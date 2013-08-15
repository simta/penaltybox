
/*  pb.h  */

#include <mysql.h>
#include <mysqld_error.h>
#define __USE_XOPEN
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/*
 * These programs will be exec'd by our standard shell script called by
 * simta for filtering, so the exit values must obey those that simta
 * uses for content filters.
 *
 * The content filter return code is a bitfield.  The first line of text
 * returned from the content filter will be logged and displayed to the
 * SMTP client.
 * 
 */

/* These codes are mail filter return values, see simta.h */
#define MESSAGE_ACCEPT                  0
#define MESSAGE_TEMPFAIL                (1<<0)
#define MESSAGE_REJECT                  (1<<1)
#define MESSAGE_DELETE                  (1<<2)
#define MESSAGE_DISCONNECT              (1<<3)
#define MESSAGE_TARPIT                  (1<<4)
#define MESSAGE_JAIL                    (1<<5)
#define MESSAGE_BOUNCE                  (1<<6)

/*
 * On most errors, we will "accept" the message.  During initial
 * insertion into the database, we will "tempfail" the message.
 *
 */


#define _PATH_MYSQL_CONFIG	"./my-pb.conf"
char	*config = _PATH_MYSQL_CONFIG;

