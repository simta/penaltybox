#ifndef PENALTYBOX_H
#define PENALTYBOX_H

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/* simta mail filter return values, see simta.h in the simta source */
#define MESSAGE_ACCEPT                  0
#define MESSAGE_TEMPFAIL                (1<<0)
#define MESSAGE_REJECT                  (1<<1)
#define MESSAGE_DELETE                  (1<<2)
#define MESSAGE_DISCONNECT              (1<<3)
#define MESSAGE_TARPIT                  (1<<4)
#define MESSAGE_JAIL                    (1<<5)
#define MESSAGE_BOUNCE                  (1<<6)

#define PENALTYBOX_PREFIX               "pb"
#define USERTHROTTLE_PREFIX             "ut"
#define USERTHROTTLE_DEFAULT            250
#define IPTHROTTLE_PREFIX               "ipt"

#endif /* PENALTYBOX_H */
