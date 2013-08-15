
/* userthrottle
 *
 * 	A variation on the "penaltybox" concept, this program keys
 * 	on the authenticated user rather than the source IP.  See
 *	pb.c.
 *
 */

#include "simta_mysql.h"
#include <sys/utsname.h>

main( int ac, char *av[] )
{
    MYSQL	mysql;
    MYSQL_RES	*result;
    MYSQL_ROW	row;
    char	*uniqname, *chksum, *host, *timestamp, *nrcpts;
    char	query[ 1024 ];
    int		c, err = 0, len, rc;
    extern int	optind;
    struct utsname *ub;

	/* To Do:  read a configuration file which contains an array of
	 * (interval, limit) vectors, and exit with MESSAGE_TEMPFAIL or
	 * possibly MESSAGE_JAIL if the authenticated user is over any
	 * of the quotas.
	 */

    while (( c = getopt( ac, av, "f:" )) != -1 ) {
	switch ( c ) {
	case 'f' :
	    config = optarg;
	    break;

	case '?' :
	default :
	    err++;
	    break;
	}
    }

    if ( ac - optind != 0 ) {
	err++;
    }

    if ( err ) {
	fprintf( stderr, "usage: %s [ -f mysql-config ]\n", av[ 0 ] );
	exit( MESSAGE_ACCEPT );
    }


    if (( chksum = getenv( "SIMTA_CHECKSUM" )) == NULL ) {
	fprintf( stderr, "SIMTA_CHECKSUM not set\n" );
	exit( MESSAGE_ACCEPT );
    }

    if (( uniqname = getenv( "SIMTA_AUTH_ID" )) == NULL ) {
	fprintf( stderr, "SIMTA_AUTH_ID not set\n" );
	exit( MESSAGE_ACCEPT );
    }

    /* Assumes SIMTA_NRCPTS is an integer, may we should use atoi(3)? */
    if (( nrcpts = getenv( "SIMTA_NRCPTS" )) == NULL ) {
	fprintf( stderr, "SIMTA_NRCPTS not set\n" );
	exit( MESSAGE_ACCEPT );
    }

    if ( ! ( ub = malloc( sizeof( struct utsname ) ) ) ) {
	fprintf( stderr, "malloc fail\n" );
	exit( MESSAGE_ACCEPT );
    }

    rc = uname( ub );

    if ( rc ) {
	fprintf( stderr, "uname fail %d\n", rc );
	exit( MESSAGE_ACCEPT );
    }

    host = ub->nodename;

    if ( mysql_init( &mysql ) == NULL ) {
	perror( "mysql_init" );
	exit( MESSAGE_ACCEPT );
    }

    if ( mysql_options( &mysql, MYSQL_READ_DEFAULT_FILE, config ) != 0 ) {
	fprintf( stderr, "mysql_options %s failed\n", config );
	goto DONE;
    }

    if ( mysql_real_connect( &mysql, NULL, NULL, NULL, NULL, 0, NULL, 0 )
	    == NULL ) {
	fprintf( stderr, "MySQL connection failed: %s\n",
		mysql_error( &mysql ));
	goto DONE;
    }

    len = snprintf( query, sizeof( query ),
    	    "INSERT INTO ut VALUES ('%s', '%s', '%s', DEFAULT, %s)",
    	    uniqname, chksum, host, nrcpts );

    if ( len >= sizeof( query )) {
	fprintf( stderr, "INSERT too long! %d\n", len );
	goto DONE;
    }

    rc = mysql_real_query( &mysql, query, len );

    if ( rc != 0 ) {
	printf( "UserThrottle insert record fail: <%s> <%s> <%s> <%s>\n",
	    uniqname, chksum, host, timestamp );
	goto DONE;
    }

    /* just insert the record for now, later versions might do a query here.
     * See "Todo", above.
     */

    DONE:
    mysql_close( &mysql );
    exit( MESSAGE_ACCEPT );
}

