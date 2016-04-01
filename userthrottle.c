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

int main( int ac, char *av[] )
{
    char            *redis_host = "127.0.0.1";
    int             redis_port = 6379;
    int		    c;
    int             err = 0;
    long long       nrcpts;
    long long       total;
    extern int	    optind;
    char            *endptr;
    char            *chksum;
    char            *prefix = USERTHROTTLE_PREFIX;
    char            *uniqname;
    char            *env_nrcpts;
    yastr           key;
    urclHandle      *urc;
    redisReply      *res;

    while (( c = getopt( ac, av, "h:p:P:" )) != -1 ) {
	switch ( c ) {
	case 'h':
	    redis_host = optarg;
	    break;

        case 'p':
            errno = 0;
            redis_port = strtoll( optarg, NULL, 10 );
            if ( errno ) {
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

    if (( ac - optind ) != 0 ) {
	err++;
    }

    if ( err ) {
	fprintf( stderr, "usage: %s [ -h redis-host ] [ -p redis-port ] "
                "[ -P redis-prefix ]\n",
                av[ 0 ] );
	exit( 1 );
    }

    if (( urc = urcl_connect( redis_host, redis_port )) == NULL ) {
        fprintf( stderr, "Unable to connect to %s:%d\n",
                redis_host, redis_port );
        exit( 1 );
    }

    if (( chksum = getenv( "SIMTA_BODY_CHECKSUM" )) == NULL ) {
	fprintf( stderr, "SIMTA_BODY_CHECKSUM not set\n" );
	exit( 1 );
    }

    if (( uniqname = getenv( "SIMTA_AUTH_ID" )) == NULL ) {
	fprintf( stderr, "SIMTA_AUTH_ID not set\n" );
	exit( 1 );
    }

    if (( env_nrcpts = getenv( "SIMTA_NRCPTS" )) == NULL ) {
	fprintf( stderr, "SIMTA_NRCPTS not set\n" );
	exit( 1 );
    }

    errno = 0;
    nrcpts = strtoll( env_nrcpts, &endptr, 10 );
    if (( errno != 0 ) || ( *endptr != '\0' )) {
        fprintf( stderr, "SIMTA_NRCPTS invalid\n" );
        exit( 1 );
    }

    key = yaslcatprintf( yaslauto( prefix ), ":user:%s", uniqname );

    if ((( res = urcl_command( urc, key, "INCRBY %s %s", key,
            env_nrcpts )) == NULL ) || ( res->type != REDIS_REPLY_INTEGER )) {
        fprintf( stderr, "INCRBY on %s failed\n", key );
        exit( 1 );
    }

    total = res->integer;
    if ( total == nrcpts ) {
        /* This is a new key, set it to expire in 24 hours */
        urcl_command( urc, key, "EXPIRE %s 86400", key );
    }

    printf( "%lld\n", total );

    exit( 0 );
}

