// main header

#ifndef MAIN_PPPSTAT_H
#define MAIN_PPPSTAT_H

#ifdef HAVE_CONFIG_H
//#include <config.h>
#endif

#include <locale.h>
#include <libintl.h>
#define _(String) gettext (String) 

#include <time.h>
#include <stdio.h>

#include "conf.h"

#define NAME 16
#define PERIODS 8 //how many periods with different costs may have one ISP
#define LINE 256
#define PATH 128

struct connection //will contain info about every SUCCSESSFUL connection
{
    int iscon; // to detect, is this connection, or tail of one after normalization
    char user[NAME]; //user name
    char isp[NAME]; //ISP IP				<<--
    struct tm *start; //when begin
    struct tm *end; //when end
    long dur; //duration of connection in secs
    long pppd_dur; //that, reported by pppd
    long inbyte;
    long outbyte;
    struct connection *next;
};

struct period //here we have cost for certain period
{
    char from;
    char till;
    float cost;
};

struct isp //info about ISP modem pools to determine ISP
{
    char *name;
    char *ip[NAME];
    struct period *price[PERIODS];
    struct isp *next;
};

struct flags //I found that there are too many flags to pass, so I made a structure;
{
    unsigned int user : 1;
    unsigned int isp : 1;
    unsigned int apart : 1;
    unsigned int mounth : 1;
    unsigned int human : 1;
    unsigned int debug : 1;
};

struct cons //small stat on unsuccsessful connections
{
    long total;
    int failed;
    int killed;
}; 

extern struct cons cstat;

extern struct flags fl;

#endif
