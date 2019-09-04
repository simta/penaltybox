/*
 * Copyright (c) Regents of The University of Michigan
 * See COPYING.
 */

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>

#include <urcl.h>

#include "yasl.h"

#include "penaltybox.h"

int
main(int ac, char *av[]) {
    char *      redis_host = "127.0.0.1";
    int         redis_port = 6379;
    int         c;
    int         err = 0;
    long long   score;
    long long   total;
    extern int  optind;
    char *      endptr;
    char *      prefix = USERTHROTTLE_PREFIX;
    yastr       buf;
    char *      env_buf;
    yastr       uniqname;
    yastr       mailfrom;
    yastr       hfrom = NULL;
    yastr       domain = NULL;
    yastr       key;
    urclHandle *urc;
    redisReply *res;

    while ((c = getopt(ac, av, "d:h:p:P:")) != -1) {
        switch (c) {
        case 'd':
            domain = yaslauto(optarg);
            yasltolower(domain);
            break;

        case 'h':
            redis_host = optarg;
            break;

        case 'p':
            errno = 0;
            redis_port = strtoll(optarg, NULL, 10);
            if (errno) {
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

    if ((ac - optind) != 0) {
        err++;
    }

    if (err) {
        fprintf(stderr,
                "usage: %s [ -h redis-host ] [ -p redis-port ] "
                "[ -P redis-prefix ] [ -d expected-domain ]\n",
                av[ 0 ]);
        exit(1);
    }

    if ((env_buf = getenv("SIMTA_AUTH_ID")) == NULL) {
        fprintf(stderr, "SIMTA_AUTH_ID not set\n");
        exit(1);
    }
    uniqname = yaslauto(env_buf);
    yasltolower(uniqname);

    if ((env_buf = getenv("SIMTA_SMTP_MAIL_FROM")) == NULL) {
        fprintf(stderr, "SIMTA_SMTP_MAIL_FROM not set\n");
        exit(1);
    }
    mailfrom = yaslauto(env_buf);
    yasltolower(mailfrom);

    if ((env_buf = getenv("SIMTA_HEADER_FROM")) != NULL) {
        hfrom = yaslauto(env_buf);
        yasltolower(hfrom);
    }

    if ((env_buf = getenv("SIMTA_NRCPTS")) == NULL) {
        fprintf(stderr, "SIMTA_NRCPTS not set\n");
        exit(1);
    }

    errno = 0;
    score = strtoll(env_buf, &endptr, 10);
    if ((errno != 0) || (*endptr != '\0')) {
        fprintf(stderr, "SIMTA_NRCPTS invalid\n");
        exit(1);
    }

    if ((urc = urcl_connect(redis_host, redis_port)) == NULL) {
        fprintf(stderr, "Unable to connect to %s:%d\n", redis_host, redis_port);
        exit(1);
    }

    /* Adjust the score if the sender doesn't match the authenticated user */
    buf = yasldup(mailfrom);
    yaslrange(buf, 0, yasllen(uniqname) - 1);
    if (yaslcmp(buf, uniqname) != 0) {
        score *= 2;
    }

    /* Adjust the score if the addresses don't match */
    if (hfrom && (yaslcmp(mailfrom, hfrom) != 0)) {
        score *= 2;
    }

    /* Adjust the score if it's from a different domain */
    if (domain) {
        buf = yasldup(mailfrom);
        yaslrange(buf, 0 - (yasllen(domain)), -1);
        if (yaslcmp(buf, domain) != 0) {
            score *= 2;
        }
    }

    /* Adjust the score if the authuser has sent from multiple addresses */
    key = yaslcatprintf(
            yaslauto(prefix), ":user:%s:rfc5321.mailfrom", uniqname);

    if (((res = urcl_command(
                  urc, key, "HINCRBY %s %s %s", key, mailfrom, "1")) == NULL) ||
            (res->type != REDIS_REPLY_INTEGER)) {
        fprintf(stderr, "HINCRBY on %s %s failed\n", key, mailfrom);
        exit(1);
    }
    /* This hash never expires if it's active. */
    urcl_command(urc, key, "EXPIRE %s 86400", key);

    if (((res = urcl_command(urc, key, "HLEN %s", key)) == NULL) ||
            (res->type != REDIS_REPLY_INTEGER)) {
        fprintf(stderr, "HLEN on %s failed\n", key);
        exit(1);
    }

    if (res->integer > 1) {
        score *= (res->integer * (res->integer - 1));
    }

    key = yaslcatprintf(yaslauto(prefix), ":user:%s", uniqname);
    if (((res = urcl_command(urc, key, "INCRBY %s %s", key,
                  yaslfromlonglong(score))) == NULL) ||
            (res->type != REDIS_REPLY_INTEGER)) {
        fprintf(stderr, "INCRBY on %s failed\n", key);
        exit(1);
    }

    total = res->integer;
    if (total == score) {
        /* This is a new key, set it to expire in 24 hours */
        urcl_command(urc, key, "EXPIRE %s 86400", key);
    }

    printf("%lld\n", total);

    exit(0);
}
