/*
 * Copyright (c) Regents of The University of Michigan
 * See COPYING.
 */

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include <urcl.h>

#include "yasl.h"

#include "penaltybox.h"

static yastr pb_ip_subnet(const char *, int, int);

int
main(int ac, char *av[]) {
    char *      redis_host = "127.0.0.1";
    int         redis_port = 6379;
    char *      ip;
    char *      cksum;
    char *      from;
    char *      prefix = PENALTYBOX_PREFIX;
    char *      reason;
    char *      subnet;
    yastr       key, ht_key;
    char        nowstr[ 42 ];
    int         c, err = 0;
    long long   ht_threshold = 0;
    struct tm   tm;
    time_t      then, now;
    double      timediff;
    extern int  optind;
    urclHandle *urc;
    redisReply *res;

    while ((c = getopt(ac, av, "h:H:p:P:")) != -1) {
        switch (c) {
        case 'h':
            redis_host = optarg;
            break;
        case 'H':
            errno = 0;
            ht_threshold = strtoll(optarg, NULL, 10);
            if (errno != 0) {
                err++;
            }
            break;
        case 'p':
            errno = 0;
            redis_port = strtoll(optarg, NULL, 10);
            if (errno != 0) {
                err++;
            }
            break;
        case 'P':
            prefix = optarg;
            break;
        case '?':
        default:
            err++;
            break;
        }
    }

    if (ac - optind != 1) {
        err++;
    }

    if (err) {
        fprintf(stderr,
                "usage: %s [ -h redis-host ] [ -p redis-port ] reason\n",
                av[ 0 ]);
        exit(MESSAGE_ACCEPT);
    }

    reason = av[ optind ];

    if ((cksum = getenv("SIMTA_CHECKSUM")) == NULL) {
        fprintf(stderr, "SIMTA_CHECKSUM not set\n");
        exit(MESSAGE_ACCEPT);
    }
    if ((ip = getenv("SIMTA_REMOTE_IP")) == NULL) {
        fprintf(stderr, "SIMTA_REMOTE_IP not set\n");
        exit(MESSAGE_ACCEPT);
    }
    if ((from = getenv("SIMTA_SMTP_MAIL_FROM")) == NULL) {
        fprintf(stderr, "SIMTA_SMTP_MAIL_FROM not set\n");
        exit(MESSAGE_ACCEPT);
    }

    if ((urc = urcl_connect(redis_host, redis_port)) == NULL) {
        fprintf(stderr, "Unable to connect to %s:%d\n", redis_host, redis_port);
        exit(MESSAGE_ACCEPT);
    }

    ht_key = yaslcatprintf(yaslauto(prefix), ":hattrick:%s", ip);

    if (ht_threshold > 0) {
        if (((res = urcl_command(urc, ht_key, "GET %s", ht_key)) != NULL) &&
                (res->type == REDIS_REPLY_STRING)) {
            if (strtoll(res->str, NULL, 10) >= ht_threshold) {
                urcl_command(urc, ht_key, "INCR %s", ht_key);
                printf("PenaltyBox: No Penalty: [%s] <%s> %s\n", ip, from,
                        reason);
                exit(MESSAGE_ACCEPT);
            }
        }
    }
    now = time(NULL);
    strftime(nowstr, 42, "%FT%TZ", gmtime(&now));

    subnet = pb_ip_subnet(ip, 24, 64);

    key = yaslcatprintf(yaslauto(prefix), ":record:%s", cksum);

    res = urcl_command(urc, key, "HGET %s %s", key, subnet);
    if (((res = urcl_command(urc, key, "HGET %s %s", key, subnet)) != NULL) &&
            (res->type == REDIS_REPLY_STRING)) {
        memset(&tm, 0, sizeof(struct tm));
        strptime(res->str, "%FT%TZ", &tm);
        setenv("TZ", "", 1);
        tzset();
        then = mktime(&tm);
        if ((timediff = difftime(now, then)) > 300) {
            urcl_command(urc, key, "DEL %s", key);
            if (ht_threshold > 0) {
                if (((res = urcl_command(urc, ht_key, "INCR %s", ht_key)) !=
                            NULL) &&
                        (res->type == REDIS_REPLY_INTEGER) &&
                        (res->integer == 1)) {
                    urcl_command(urc, ht_key, "EXPIRE %s 14400", ht_key);
                }
            }
            printf("PenaltyBox: Accept: %.0fs [%s] <%s> %s\n", timediff, ip,
                    from, reason);
            exit(MESSAGE_ACCEPT);
        } else {
            printf("PenaltyBox: Window: %.0fs [%s] <%s> %s\n", timediff, ip,
                    from, reason);
            exit(MESSAGE_TEMPFAIL);
        }
    }

    if ((res = urcl_command(urc, key, "HSET %s %s %s", key, subnet, nowstr)) ==
                    NULL ||
            (res->type != REDIS_REPLY_INTEGER)) {
        printf("PenaltyBox: Accept: 0s [%s] <%s> database error\n", ip, from);
        exit(MESSAGE_ACCEPT);
    }

    /* expire after three days */
    urcl_command(urc, key, "EXPIRE %s 259200", key);
    printf("PenaltyBox: Record: [%s] <%s> %s\n", ip, from, reason);
    exit(MESSAGE_TEMPFAIL);
}

yastr
pb_ip_subnet(const char *ip, int cidr4, int cidr6) {
    int                  rc;
    int                  bytes;
    int                  bits;
    struct addrinfo *    ai;
    struct addrinfo      hints;
    struct sockaddr_in6 *addr;
    char                 buf[ INET6_ADDRSTRLEN ];

    if (cidr4 == 0 && cidr6 == 0) {
        goto error;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    if ((rc = getaddrinfo(ip, NULL, &hints, &ai)) != 0) {
        goto error;
    }

    if (ai->ai_addr->sa_family == AF_INET) {
        ((struct sockaddr_in *)(ai->ai_addr))->sin_addr.s_addr &=
                htonl((0xFFFFFFFF << (32 - cidr4)));
    } else {
        addr = (struct sockaddr_in6 *)(ai->ai_addr);
        bytes = cidr6 / 8;
        bits = cidr6 % 8;
        if (bits > 0) {
            /* mask out partial byte */
            addr->sin6_addr.s6_addr[ bytes ] &= (0xFF << (8 - bits));
            bytes++;
        }
        if (bytes < 16) {
            /* zero out whole bytes */
            memset(addr->sin6_addr.s6_addr + bytes, 0, 16 - bytes);
        }
    }

    if ((rc = getnameinfo(ai->ai_addr,
                 ((ai->ai_addr->sa_family == AF_INET6)
                                 ? sizeof(struct sockaddr_in6)
                                 : sizeof(struct sockaddr_in)),
                 buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST)) != 0) {
        goto error;
    }

    return yaslauto(buf);

error:
    return yaslauto(ip);
}
