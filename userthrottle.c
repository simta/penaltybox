
/* userthrottle
 *
 * 	A variation on the "penaltybox" concept, this program keys
 * 	on the authenticated user rather than the source IP.  See
 *	pb.c.
 *
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
    int             retval = MESSAGE_ACCEPT;
    long long       nrcpts;
    long long       threshold = USERTHROTTLE_DEFAULT;
    long long       total;
    long long       user_threshold;
    extern int	    optind;
    char            *user_config;
    char            *chksum;
    char            *prefix = USERTHROTTLE_PREFIX;
    char            *uniqname;
    yastr           key;
    URCL            *urc;

    while (( c = getopt( ac, av, "h:l:p:P:" )) != -1 ) {
	switch ( c ) {
	case 'h':
	    redis_host = optarg;
	    break;

        case 'l':
            errno = 0;
            threshold = strtoll( optarg, NULL, 10 );
            if ( errno ) {
                err++;
            }
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
	fprintf( stderr,
                "usage: %s [ -h redis-host ] [ -p redis-port ] [ -l limit ]\n",
                av[ 0 ] );
	exit( retval );
    }

    if (( urc = urcl_connect( redis_host, redis_port )) == NULL ) {
        fprintf( stderr, "Unable to connect to %s:%d\n",
                redis_host, redis_port );
        exit( MESSAGE_ACCEPT );
    }

    if (( chksum = getenv( "SIMTA_BODY_CHECKSUM" )) == NULL ) {
	fprintf( stderr, "SIMTA_BODY_CHECKSUM not set\n" );
	exit( retval );
    }

    if (( uniqname = getenv( "SIMTA_AUTH_ID" )) == NULL ) {
	fprintf( stderr, "SIMTA_AUTH_ID not set\n" );
	exit( retval );
    }

    if ( getenv( "SIMTA_NRCPTS" ) == NULL ) {
	fprintf( stderr, "SIMTA_NRCPTS not set\n" );
	exit( retval );
    }

    nrcpts = strtoll( getenv( "SIMTA_NRCPTS" ), NULL, 10 );

    key = yaslcatlen( yaslauto( prefix ), ":userconf", 9 );

    if (( user_config = urcl_hget( urc, key, uniqname ))) {
        errno = 0;
        user_threshold = strtoll( user_config, NULL, 10 );
        if ( errno == 0 ) {
            if ( user_threshold < 0 ) {
                threshold *= llabs( user_threshold );
            } else {
                threshold = user_threshold;
            }
        } else {
            /* Something went wrong, go unlimited */
            threshold = 0;
        }
    }

    key = yaslcatprintf( yaslcpy( key, prefix ), ":user:%s", uniqname );

    total = urcl_incrby( urc, key, nrcpts );
    if ( total == nrcpts ) {
        /* This is a new key, set it to expire in 24 hours */
        urcl_expire( urc, key, 86400 );
    }

    if (( threshold > 0 ) && ( total >= threshold )) {
        retval = MESSAGE_JAIL;
    }

    exit( retval );
}

