// Common definitions and structures

#include "main.h"

extern void show_isp(struct isp *); // shows the list
extern void show_cons(struct connection *); //show connections - for debug
extern void show_stat(struct connection *, struct flags *); //print stats with
//or without users/isp
extern void usage(void);//prints usage and exit
extern void version(void);//version and exit
