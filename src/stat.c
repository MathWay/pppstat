// Stats functions

#include "main.h"
#include "stat.h"

long tm2sec(struct tm *); //convert time in tm to seconds
void statcpy(struct connection *dest, struct connection *source);//copy stat structure

void norm_cons(struct connection *top)
{
    struct connection *tmp, *prev = NULL;
    long a, b, c, d;
    double r;
    char str[NAME];
    char unpunct = 0;

    while ( top )
    {	if ( top->start->tm_mday != top->end->tm_mday)
	{
	    tmp = (struct connection *)calloc( 1, sizeof(struct connection) );
	    tmp->start = (struct tm *)calloc(1, sizeof(struct tm));
	    tmp->end = (struct tm *)calloc(1, sizeof(struct tm));
	    
	    tmp->start->tm_mday = tmp->end->tm_mday = top->end->tm_mday;
	    tmp->start->tm_mon = tmp->end->tm_mon = top->end->tm_mon;
	    tmp->end->tm_sec = top->end->tm_sec;
	    tmp->end->tm_min = top->end->tm_min;
	    tmp->end->tm_hour = top->end->tm_hour;
	    tmp->next = top->next;
	    strcpy(tmp->user, top->user);
	    strcpy(tmp->isp, top->isp);
	    
	    top->end->tm_sec = 59;
	    top->end->tm_min = 59;
	    top->end->tm_hour = 23;
	    top->end->tm_mday = top->start->tm_mday;
	    top->end->tm_mon = top->start->tm_mon;
	    top->next = tmp;
//I can't find better way to split transfered bytes, then to make a simple proportion	    
	    a = tm2sec(top->start);
	    b = tm2sec(top->end);
	    c = tm2sec(tmp->start);
	    d = tm2sec(tmp->end);
	    
	    r = (double)(d - c) / ( (d - c) + (b - a) );
	    
	    tmp->inbyte = lrint( r * top->inbyte );
	    tmp->outbyte = lrint( r * top->outbyte );
	    tmp->pppd_dur = lrint( r * top->pppd_dur );
	    
	    top->inbyte = lrint( (1 - r) * top->inbyte );
	    top->outbyte = lrint( (1 - r) * top->outbyte );
	    top->pppd_dur = lrint( (1 - r) * top->pppd_dur );
	}
	
	top->dur = tm2sec(top->end) - tm2sec(top->start);
	if ( labs(top->dur - top->pppd_dur) > 3)
	{
	    if ( !unpunct )
	    {
		fprintf(stderr, _("\nYour logs aren't punctual. I'll use time, reported by pppd, this may cause 3sec deviation.\n"));
		fprintf(stderr, _("Use ntpd ;)\n\n"));
		unpunct = 1;
	    }
	    if( (top->dur = top->pppd_dur) < 0 )
	    {
		top->dur = 0;
		fprintf(stderr, _("Some connects have negative time. Making it 0. \n"));
	    }
	}
	prev = top;
	top = top->next;
    }
}

long tm2sec(struct tm *tme)
{
    return ( tme->tm_sec + tme->tm_min * 60 + tme->tm_hour * 3600 );
}

struct connection *mkstat(struct connection *head, struct flags *f)
{
    struct connection *day, *prev, *top, *cur, *tmp;
    
    if ( f->apart )
	cur = top = (struct connection *)calloc(1, sizeof(struct connection));
    else
	top = head;
	
    while ( head )
    {
	if ( f->apart )
	    statcpy(cur, head);
	
	day = head->next;
	if ( !day )
	    break;
	prev = head;
	
	while ( f->mounth ? (day->start->tm_mon == head->start->tm_mon) :\
			    (day->start->tm_mday == head->start->tm_mday) )
	{
	    if ( (!strcmp(head->user, day->user) || !f->user) \
		&& (!strcmp(head->isp, day->isp) || !f->isp) )
	    {
		if ( f->apart )
		{
		    cur->inbyte += day->inbyte;
		    cur->outbyte += day->outbyte;
		    cur->dur += day->dur;
		    cur->iscon += day->iscon;
		    
		    cur->end = day->start->tm_mday;
		}
		else
		{
		    head->inbyte += day->inbyte;
		    head->outbyte += day->outbyte;
		    head->dur += day->dur;
		    head->iscon += day->iscon;
		    
		    head->end = day->start->tm_mday;
		}
		
		prev->next = day->next;
		free(day);
		day = prev;
	    }
	    prev = day;
	    day = day->next;
	    if ( !day )
		break;
	}
	if ( f->apart )
	{
	    tmp = cur;
	    cur->next = (struct connection *)calloc(1, sizeof(struct connection));
	    cur = cur->next;
	}

	head = head->next;
	
    }
    
    if ( f->apart )
    {    
	tmp->next = NULL;
	free(cur);
	return ( top );
    }
}

void statcpy(struct connection *dest, struct connection *source)
{
    strcpy(dest->user, source->user);
    strcpy(dest->isp, source->isp);
    dest->iscon = source->iscon;
    dest->inbyte = source->inbyte;
    dest->outbyte = source->outbyte;
    dest->dur = source->dur;
    dest->pppd_dur = source->pppd_dur;
    
    dest->start = (struct tm *)calloc(1, sizeof(struct tm));
    dest->start->tm_mon = source->start->tm_mon;
    dest->start->tm_mday = source->start->tm_mday;
}
