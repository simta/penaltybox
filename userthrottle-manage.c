#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>

#include <urcl.h>

#include "yasl.h"

#include "penaltybox.h"

int main( int ac, char *av[] )
{
    char        *redis_host = "127.0.0.1";
    int         redis_port = 6379;
    int		c, err = 0;
    long long   threshold = 0;
    extern int	optind;
    char        *prefix = USERTHROTTLE_PREFIX;
    char        *uniqname = NULL;
    yastr       key;
    yastr       value;
    URCL        *urc;

    while (( c = getopt( ac, av, "h:l:p:P:u:" )) != -1 ) {
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

        case 'u':
            uniqname = optarg;
            break;

	case '?':
	default:
	    err++;
	    break;
	}
    }

    if ( ac - optind != 0 ) {
	err++;
    }

    if ( uniqname == NULL ) {
        err++;
    }

    if ( err ) {
	fprintf( stderr,
                "usage: %s [ -h redis-host ] [ -p redis-port ] [ -l limit ] "
                "-u uniqname\n",
                av[ 0 ] );
	exit( 1 );
    }

    if (( urc = urcl_connect( redis_host, redis_port )) == NULL ) {
        fprintf( stderr, "Unable to connect to %s:%d\n",
                redis_host, redis_port );
        exit( 1 );
    }

    key = yaslcatlen( yaslauto( prefix ), ".userconf", 9 );
    value = yaslfromlonglong( threshold );

    if ( urcl_hset( urc, key, uniqname, value )) {
        printf( "userconf insert failed: <%s> <%lld>\n",
                uniqname, threshold );
        exit( 1 );
    }

    printf( "userconf insert succeeded: <%s> <%lld>\n", uniqname, threshold );
    exit( 0 );
}
