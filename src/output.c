// Deals with output

#include "main.h"
#include "output.h"

void sec2tm(struct tm *); //normalize time in structure
char *humanize_size(long size); //make big size look pretty

void show_cons(struct connection *top)
{
    char str[LINE];

    while ( top )
    {
	printf(_("\nuser:\t%s\n"), top->user);
	printf(_("ISP:\t%s\n"), top->isp);
	strftime(str, LINE, "%b %e %T", top->start);
	printf(_("start:\t%s\n"), str);
	str[0] = '\0';
	strftime(str, LINE, "%b %e %T", top->end);
	printf(_("end:\t%s\n"), str);
	printf(_("pd_dur:\t%ld secs\n"), top->pppd_dur);
	printf(_("dur:\t%ld secs\n"), top->dur);
	printf(_("out:\t%ld\n"), top->outbyte);
	printf(_("in:\t%ld\n"), top->inbyte);
	printf(_("is con\t%d\n"), top->iscon);
	top = top->next;
    }
    printf(_("\ntotal starts of pppd: %d\n"), cstat.total);
    printf(_("failed to connect %d times\n"), cstat.failed);
    printf(_("pppd was %d times killed\n"), cstat.killed);
}

void show_stat(struct connection *top, struct flags *f)
{
    char str[LINE], str1[16], i;
    
    
    printf("\n%-10s ", _("period"));
    if ( f->user ) printf("%-8s ", _("user"));
    if ( f->isp ) printf("%-15s ", _("ISP"));
    printf("%11s %11s %10s %9s\n", _("in, bytes"),_("out, bytes"),_("time"),_("connects"));
    
    for( i = 0; i < 80; i++ ) putchar('-');
    putchar('\n');
    
    while ( top )
    {
        strftime(str, LINE, "%b %e", top->start);
	if ( f->mounth )
	{
	    if (top->iscon == 1)
		top->end = top->start->tm_mday;
	    sprintf(str1, "-%d", top->end);
	    strcat( str, str1 );
	}
	
	printf("%-10s ", str);
	if ( f->user ) printf("%-8s ", top->user);
	if ( f->isp ) printf("%-15s ", top->isp);
	if (f->human )
	{
	    printf("%11s %11s ", humanize_size(top->inbyte), humanize_size(top->outbyte));
	    	    
	    top->start->tm_sec = top->dur;
	    
	    sec2tm(top->start);
	    strftime(str, LINE, "%T", top->start);
	    printf("%10s ", str);
	}
	else
	{
	    printf("%11ld ", top->inbyte);
	    printf("%11ld ", top->outbyte);
	    printf("%10ld ", top->dur);
	}
    	printf("%9d\n", top->iscon);	
	top = top->next;
    }
    
    for( i = 0; i < 80; i++ ) putchar('-');
    
    printf(_("\ntotal starts of pppd: %d\n"), cstat.total);
    printf(_("failed to connect %d times\n"), cstat.failed);
    printf(_("pppd was %d times killed\n\n"), cstat.killed);
}




void usage(void)
{
    fprintf(stderr, _("\nPPPstat version %s. http://pppstat.sourceforge.net\n"), VERSION);
    fprintf(stderr, _("Distributed under GPL.\n\n"));
    fprintf(stderr, _("Usage:\n\n"));
    fprintf(stderr, _("pppstat [option]\n\n"));
    fprintf(stderr, _("-u, --user\tcounts all connections for each user per period\n"));
    fprintf(stderr, _("-i, --isp\tcounts all connections for each ISP per period\n"));
    fprintf(stderr, _("-m, --month\tdefines period as mounth, otherwise as day\n"));
    fprintf(stderr, _("-h, --human\tmakes human-readable output\n"));
    fprintf(stderr, _("-d, --debug\tdebug mode, VERY verbose, be carefull, curently SEGFAULT ;)\n"));
    fprintf(stderr, _("-v, --version\tprogram verson\n"));
    fprintf(stderr, _("-H, --help\tthis screen\n"));
    putc('\n', stderr);
    exit(4);
}

void version(void)
{
    fprintf(stderr, _("PPPstat versoin %s\n"), VERSION);
    exit(4);
}

void show_isp(struct isp *top)
{
    int i = 0;
    
    while (top)
    {
	printf("\n%s\n", top->name);
	while ( top->ip[i] )
	    printf("%s\n", top->ip[i++]);
	i = 0;
	while ( top->price[i] )
	{
	    printf("from %d till %d costs %f\n", top->price[i]->from, \
						    top->price[i]->till, \
						    top->price[i]->cost);
	    i++;
	}
	i = 0;
	top = top->next;
    }
}

void sec2tm(struct tm *tme)
{
    tme->tm_min = tme->tm_sec / 60;
    tme->tm_sec = tme->tm_sec % 60;
    
    tme->tm_hour = tme->tm_min / 60;
    tme->tm_min = tme->tm_min % 60;
}

char *humanize_size(long size)
{
    char i = 0;
    char suffix[] = {'\0', 'k', 'M', 'G', 'T'};
    char *s;
    
    s = (char *)calloc(1, NAME);
    
    while ( size > 1024 )
    {	
	size = ( size + 512 ) >> 10;
	if ( ++i == 4 ) break;
    }
    
    sprintf(s, "%ld%c", size, suffix[i]);
    
    return s;
}
