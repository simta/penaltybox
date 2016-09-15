/*
 * Copyright (c) Regents of The University of Michigan
 * See COPYING.
 */

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <urcl.h>

#include "yasl.h"

#include "penaltybox.h"

int main( int ac, char *av[] )
{
    char            *redis_host = "127.0.0.1";
    int             redis_port = 6379;
    char            *p;
    char            *ip;
    char            *cksum;
    char            *from;
    char            *prefix = PENALTYBOX_PREFIX;
    char            *reason;
    char            *subnet;
    yastr           key, ht_key;
    char            nowstr[ 42 ];
    int             c, err = 0;
    long long       ht_threshold = 0;
    struct tm       tm;
    time_t          then, now;
    double          timediff;
    extern int      optind;
    urclHandle      *urc;
    redisReply      *res;

    while (( c = getopt( ac, av, "h:H:p:P:" )) != -1 ) {
        switch ( c ) {
        case 'h':
            redis_host = optarg;
            break;
        case 'H':
            errno = 0;
            ht_threshold = strtoll( optarg, NULL, 10 );
            if ( errno != 0 ) {
                err++;
            }
            break;
        case 'p':
            errno = 0;
            redis_port = strtoll( optarg, NULL, 10 );
            if ( errno != 0 ) {
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

    if ( ac - optind != 1 ) {
        err++;
    }

    if ( err ) {
        fprintf( stderr,
                "usage: %s [ -h redis-host ] [ -p redis-port ] reason\n",
                av[ 0 ] );
        exit( MESSAGE_ACCEPT );
    }

    reason = av[ optind ];

    if (( cksum = getenv( "SIMTA_CHECKSUM" )) == NULL ) {
        fprintf( stderr, "SIMTA_CHECKSUM not set\n" );
        exit( MESSAGE_ACCEPT );
    }
    if (( ip = getenv( "SIMTA_REMOTE_IP" )) == NULL ) {
        fprintf( stderr, "SIMTA_REMOTE_IP not set\n" );
        exit( MESSAGE_ACCEPT );
    }
    if (( from = getenv( "SIMTA_SMTP_MAIL_FROM" )) == NULL ) {
        fprintf( stderr, "SIMTA_SMTP_MAIL_FROM not set\n" );
        exit( MESSAGE_ACCEPT );
    }

    if (( urc = urcl_connect( redis_host, redis_port )) == NULL ) {
        fprintf( stderr, "Unable to connect to %s:%d\n",
                redis_host, redis_port );
        exit( MESSAGE_ACCEPT );
    }

    ht_key = yaslcatprintf( yaslauto( prefix ), ":hattrick:%s", ip );

    if ( ht_threshold > 0 ) {
        if ((( res = urcl_command( urc, ht_key, "GET %s", ht_key )) != NULL ) &&
                ( res->type == REDIS_REPLY_STRING )) {
            if ( strtoll( res->str, NULL, 10 ) >= ht_threshold ) {
                urcl_command( urc, ht_key, "INCR %s", ht_key );
                printf( "PenaltyBox: No Penalty: [%s] <%s> %s\n",
                        ip, from, reason );
                exit( MESSAGE_ACCEPT );
            }
        }
    }
    now = time( NULL );
    strftime( nowstr, 42, "%FT%TZ", gmtime( &now ));

    /* FIXME: This should really parse the IP and do math, to make it more
     * flexible (and IPv6 ready.)
     */
    subnet = yaslauto( ip );
    if (( p = strrchr( subnet, '.' )) != NULL ) {
        yaslrange( subnet, 0, ( p - subnet ));
        subnet = yaslcat( subnet, "0" );
    }

    key = yaslcatprintf( yaslauto( prefix ), ":record:%s", cksum );

    res = urcl_command( urc, key, "HGET %s %s", key, subnet );
    if ((( res =
            urcl_command( urc, key, "HGET %s %s", key, subnet )) != NULL ) &&
            ( res->type == REDIS_REPLY_STRING )) {
        memset( &tm, 0, sizeof( struct tm ));
        strptime( res->str, "%FT%TZ", &tm );
        setenv( "TZ", "", 1 );
        tzset( );
        then = mktime( &tm );
        if (( timediff = difftime( now, then )) > 300 ) {
            urcl_command( urc, key, "DEL %s", key );
            if ( ht_threshold > 0 ) {
                if ((( res = urcl_command( urc, ht_key, "INCR %s",
                        ht_key )) != NULL ) &&
                        ( res->type == REDIS_REPLY_INTEGER ) &&
                        ( res->integer == 1 )) {
                    urcl_command( urc, ht_key, "EXPIRE %s 14400", ht_key );
                }
            }
            printf( "PenaltyBox: Accept: %.0fs [%s] <%s> %s\n",
                    timediff, ip, from, reason );
            exit( MESSAGE_ACCEPT );
        } else {
            printf( "PenaltyBox: Window: %.0fs [%s] <%s> %s\n",
                    timediff, ip, from, reason );
            exit( MESSAGE_TEMPFAIL );
        }
    }

    if (( res = urcl_command( urc, key, "HSET %s %s %s", key, subnet,
            nowstr )) == NULL || ( res->type != REDIS_REPLY_INTEGER )) {
        printf( "PenaltyBox: Accept: 0s [%s] <%s> database error\n", ip, from );
        exit( MESSAGE_ACCEPT );
    }

    /* expire after three days */
    urcl_command( urc, key, "EXPIRE %s 259200", key );
    printf( "PenaltyBox: Record: [%s] <%s> %s\n", ip, from, reason );
    exit( MESSAGE_TEMPFAIL );

}
