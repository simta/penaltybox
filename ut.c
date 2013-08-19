
/* userthrottle
 *
 * 	A variation on the "penaltybox" concept, this program keys
 * 	on the authenticated user rather than the source IP.  See
 *	pb.c.
 *
 */

#include <inttypes.h>
#include <libgen.h>
#include <string.h>
#include <strings.h>
#include <sys/utsname.h>

#include "simta_mysql.h"

int update_user( MYSQL mysql, char *uniqname, intmax_t threshold ) {
    char    query[ 1024 ];
    int len, rc;

    len = snprintf( query, sizeof( query ),
        "INSERT INTO `user_config` (`uniqname`, `threshold`) VALUES ('%s', %jd) ON DUPLICATE KEY UPDATE `threshold` = %jd",
        uniqname, threshold, threshold );

    if ( len >= sizeof( query )) {
        fprintf( stderr, "INSERT too long! %d\n", len );
        return 1;
    }

    if (( rc = mysql_real_query( &mysql, query, len )) != 0 ) {
        printf( "SPL record insert failed: <%s> <%jd>\n",
            uniqname, threshold );
        return 1;
    }

    return 0;
}

int userthrottle( MYSQL mysql )
{
    MYSQL_RES   *result;
    MYSQL_ROW   row;
    char        *uniqname, *chksum, *host;
    char        query[ 1024 ];
    intmax_t    nrcpts, count, threshold;
    int         len, rc, retval = MESSAGE_ACCEPT;
    struct utsname *ub;
    
    if (( chksum = getenv( "SIMTA_CHECKSUM" )) == NULL ) {
	fprintf( stderr, "SIMTA_CHECKSUM not set\n" );
	return( retval );
    }

    if (( uniqname = getenv( "SIMTA_AUTH_ID" )) == NULL ) {
	fprintf( stderr, "SIMTA_AUTH_ID not set\n" );
	return( retval );
    }

    if ( getenv( "SIMTA_NRCPTS" ) == NULL ) {
	fprintf( stderr, "SIMTA_NRCPTS not set\n" );
	return( retval );
    }

    nrcpts = strtoimax( getenv( "SIMTA_NRCPTS" ), NULL, 10 );

    if ( ! ( ub = malloc( sizeof( struct utsname ) ) ) ) {
	fprintf( stderr, "malloc fail\n" );
	return( retval );
    }

    rc = uname( ub );

    if (( rc = uname( ub ))) {
	fprintf( stderr, "uname fail %d\n", rc );
	exit( retval );
    }

    host = ub->nodename;

    len = snprintf( query, sizeof( query ),
            "SELECT `threshold` FROM `user_config` WHERE `uniqname` = '%s' AND `enabled` = 1",
            uniqname );

    if ( len >= sizeof( query )) {
        fprintf( stderr, "SELECT too long! %d\n", len );
        return( retval );
    }

    if (( rc = mysql_real_query( &mysql, query, len )) != 0 ) {
        fprintf( stderr, "SELECT failed: %s\n", mysql_error( &mysql ));
        return( retval );
    }

    if (( result = mysql_store_result( &mysql )) == NULL ) {
        fprintf( stderr, "mysql_store_result failed: %s\n",
            mysql_error( &mysql ));
        return( retval );
    }

    if ( mysql_num_rows( result ) == 0 ) {
        printf( "SPL: User %s is not authorized to use this service\n",
            uniqname );
        retval = MESSAGE_REJECT;
        goto DONE;
    }

    if (( row = mysql_fetch_row( result )) == NULL ) {
        fprintf( stderr, "mysql_fetch_row failed: %s\n",
            mysql_error( &mysql ));
        goto DONE;
    }

    threshold = strtoimax( row [ 0 ], NULL, 10 );
    mysql_free_result( result );

    len = snprintf( query, sizeof( query ),
    	    "INSERT INTO `ut` (`uniqname`, `chksum`, `host`, `received`, `nrcpts`) VALUES ('%s', '%s', '%s', DEFAULT, %jd)",
    	    uniqname, chksum, host, nrcpts );

    if ( len >= sizeof( query )) {
	fprintf( stderr, "INSERT too long! %d\n", len );
	return( retval );
    }

    if (( rc = mysql_real_query( &mysql, query, len )) != 0 ) {
	printf( "UserThrottle insert record fail: <%s> <%s> <%s> <%jd>\n",
	    uniqname, chksum, host, nrcpts );
	return( retval );
    }

    len = snprintf( query, sizeof( query ),
            "SELECT SUM(`nrcpts`) FROM `ut` WHERE `uniqname` = '%s' AND `received` > TIMESTAMPADD(MINUTE, -1440, NOW())",
            uniqname );

    if ( len >= sizeof( query )) {
        fprintf( stderr, "SELECT too long! %d\n", len );
        return( retval );
    }

    if (( rc = mysql_real_query( &mysql, query, len )) != 0 ) {
        fprintf( stderr, "SELECT failed: %s\n", mysql_error( &mysql ));
        return( retval );
    }

    if (( result = mysql_store_result( &mysql )) == NULL ) {
        fprintf( stderr, "mysql_store_result failed: %s\n",
            mysql_error( &mysql ));
        return( retval );
    }

    count = strtoimax( row [ 0 ], NULL, 10 );
    if ( count > threshold ) {
        printf( "UserThrottle: %s goes vroom and ting, %jd is more than %jd\n",
            uniqname, count, threshold);
        /* retval = MESSAGE_JAIL; */
    }

    DONE:
    mysql_free_result( result );
    return( retval );
}

int main( int ac, char *av[] )
{
    MYSQL	mysql;
    int		c, err = 0;
    int         retval = MESSAGE_ACCEPT;
    intmax_t    threshold = 250;
    extern int	optind;
    char        *binname, *binbase;
    char        *uniqname;

    while (( c = getopt( ac, av, "f:l:u:" )) != -1 ) {
	switch ( c ) {
	case 'f' :
	    config = optarg;
	    break;

        case 'l' :
            threshold = strtoimax( optarg, NULL, 10 );
            break;

        case 'u' :
            uniqname = optarg;
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
	exit( retval );
    }

    if ( mysql_init( &mysql ) == NULL ) {
	perror( "mysql_init" );
	exit( retval );
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

    binname = strdup( av[ 0 ] );
    binbase = basename( binname );

    if ( strcasecmp( binbase, "userthrottle" ) == 0 ) {
        retval = userthrottle( mysql );
    }
    else if ( strcasecmp( binbase, "add-spl" ) == 0 ) {
        retval = update_user( mysql, uniqname, threshold );
    }
    else {
        fprintf( stderr,
            "I don't know what to do when I'm called as %s\n",
            binbase );
    }

    DONE:
    mysql_close( &mysql );
    exit( retval );
}


