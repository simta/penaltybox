
LDFLAGS=-Wl,-R/usr/lib/mysql -L/usr/lib/mysql -lmysqlclient -lz -lcrypt -lnsl -lm -L/usr/lib -lssl -lcrypto
CFLAGS=-I/usr/include/mysql

CC=gcc -w $(CFLAGS) $(LDFLAGS)  

all:	userthrottle penaltybox

userthrottle:	simta_mysql.h ut.c
	$(CC) -o userthrottle ut.c simta_mysql.h

penaltybox:	simta_mysql.h pb.c
	$(CC) -o penaltybox pb.c simta_mysql.h
