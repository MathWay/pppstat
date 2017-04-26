//Parser of pppd logs. Makes stats of connections by user,ISP etc.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>

#define LOGS "/var/log/daemons/info" //here we can find pppd logs
#define PPPD_LOG "/var/log/pppd.log" //here we'll place pppd logs
#define PPPD "pppd[" //all strings from pppd
#define PPPD_START "started by" //after this we can find user name
#define ISP "remote IP address" //ISP detecting
#define EXIT "Exit." //Cases, when pppd failed to connect
#define PPPD_TIME "Connect time" //Self explaining. Now not used
#define PPPD_OUTBYTE "Sent" //Sent bytes
#define PPPD_INBYTE "received" //Received bytes
#define LOGROTATE "/etc/logrotate.d/ppp_stat" //file for logrotate. Not used

#define PATH 128
#define NAME 16
#define LINE 256

#define UNZIP "bunzip" //Eah, such a stupid way to get into archived files
#define ZIP "bzip"

struct connection //will contain info about every SUCCSESSFUL connection
{
    char iscon; // to detect, is this connection, or tail of one after normalization
    char user[NAME]; //user name
    char isp[NAME]; //ISP IP				<<--
    struct tm *start; //when begin
    struct tm *end; //when end
    long dur; //duration of connection in secs
    long inbyte;
    long outbyte;
    struct connection *next;
};

struct cons //small stat on unsuccsessful connections
{
    int total;
    int failed;
    int killed;
} cstat = {0};

const char month[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

void extract_logs(char *, char *); //extract logs fron info
struct connection * parse_logs(void); //parse extracted logs
void show_cons(struct connection *); //show connections - for debug
void pack(char *); //unzip archive
void unpack(char *); //zip archive
struct tm * str2tm(char *); //transform useless date string to convenient structure 
void norm_cons(struct connection *); //divide connections, is they spread on few days
long tm2sec(struct tm *); //convert time in tm to seconds
void mkstat(struct connection *, char sep_user, char sep_isp); //make prestatistics
//char * get_cur_date(int, struct tm *);
//int inc_day(char * day);

int main(int argc, char *argv[] )
{
	struct connection *head; //start of list of cons
 	char file[PATH];
  	int i;
	char user = 0, isp = 1;

	for (i = 5; i > 0; i--) 
	{
		*file = '\0';
		sprintf(file, "%s.%d", LOGS, i);
		unpack(file);
		if ( i == 5 )
			extract_logs(file, "w");
		else
			extract_logs(file, "a");
		pack(file);
	}
	extract_logs(LOGS, "a");
	head = parse_logs();
//	show_cons(head);
	norm_cons(head);
//	show_cons(head);
	mkstat(head, user, isp);
	show_cons(head);
}

void extract_logs(char *logfile, char *wmode)
{
    FILE *log, *pppdlog;
    char line[LINE], msg[PATH];

    if ( (log = fopen(logfile, "r")) == NULL )
    {
	strcpy(msg, "open ");
	strcat(msg, logfile);
	perror(msg);
	exit(1);
    }
    
    if ( (pppdlog = fopen(PPPD_LOG, wmode)) == NULL )
    {
	strcpy(msg, "open ");
	strcat(msg, PPPD_LOG);
	perror(msg);
	exit(1);
    }
    
    while ( fgets(line, LINE-1, log) != NULL )
	if ( strstr(line, PPPD) != NULL )
	    if ( fputs(line, pppdlog) == EOF )
	    {
		strcpy(msg, "write ");
		strcat(msg, PPPD_LOG);
		perror(msg);
		exit(2);
	    }
    
    fclose(log);
    fclose(pppdlog);
}

struct connection * parse_logs()
{
    FILE *pppdlog;
    char msg[PATH], line[LINE], *p;
    struct connection *tmp, *head, *current;
    char started = 0, connected = 0;
    
    struct tm *date;
    
    head = tmp = (struct connection *) calloc( 1, sizeof(struct connection) );
    current = NULL;
    
    if ( (pppdlog = fopen(PPPD_LOG, "r")) == NULL )
    {
	strcpy(msg, "open ");
	strcat(msg, PPPD_LOG);
	perror(msg);
	exit(1);
    }
    
    while ( fgets(line, LINE-1, pppdlog) != NULL )
    {
        if ( (p = strstr(line, PPPD_START)) != NULL )
    	{
	    if ( !started )
		started = 1;
	    else
	    {	
		//fprintf(stderr, "warning: pppd was abnormally terminated on %s\n", tmp->start);
		connected = 0;
		cstat.killed++;
	    }
	    tmp->iscon = 1;
	    cstat.total++;
	    p += strlen(PPPD_START) + 1;
	    strncpy(tmp->user, strtok(p, " ,"), NAME-1);
	    continue;
	}

	if ( started )
	{
	    if ( !connected )
    	    {
		if ( (p = strstr(line, ISP)) != NULL )
		{
		    connected = 1;
		    p += strlen(ISP) + 1;
		    strncpy(tmp->isp, strtok(p, " ,\n"), NAME-1);
		    tmp->start = str2tm(line);
		    //strncpy(tmp->start, line, NAME-1);
		    continue;
		}
		else if ( strstr(line, EXIT) != NULL )
		{
		    started = 0;
		    //fprintf(stderr, "pppd failed to connect on %s\n", strncpy(tmp->start, line, NAME-1));
		    cstat.failed++;
		    continue;
		}
	    }
	    else
	    {
	    	if ( (strstr(line, PPPD_TIME)) != NULL )
		{
		    tmp->end = str2tm(line);
		    //strncpy(tmp->end, line, NAME-1);
		    continue;
		}
		else if ( (p = strstr(line, PPPD_OUTBYTE)) != NULL )
		{
		    p += strlen(PPPD_OUTBYTE) + 1;
		    tmp->outbyte = atol( p );
		    p = strstr(line, PPPD_INBYTE);
		    p += strlen(PPPD_INBYTE) + 1;
		    tmp->inbyte = atol( p );
		    
		    if ( head != tmp )
			current->next = tmp;
		    current = tmp;
		    tmp = (struct connection *) calloc( 1, sizeof(struct connection) );
		    connected = started = 0;
		    continue;
		}
	    }
	    
	}    
    }
    
    if ( current != tmp )
	free(tmp);
    fclose(pppdlog);
			    		
    return (head);
}

void show_cons(struct connection *top)
{
    char str[LINE];

    while ( top )
    {
	printf("\nuser:\t%s\n", top->user);
	printf("ISP:\t%s\n", top->isp);
	strftime(str, LINE, "%b %e %T", top->start);
	printf("start:\t%s\n", str);
	str[0] = '\0';
	strftime(str, LINE, "%b %e %T", top->end);
	printf("end:\t%s\n", str);
	printf("duration:\t%ld secs\n", top->dur);
	printf("out:\t%ld\n", top->outbyte);
	printf("in:\t%ld\n", top->inbyte);
	printf("is con\t%d\n", top->iscon);
	top = top->next;
    }
    printf("\ntotal starts of pppd: %d\n", cstat.total);
    printf("failed to connect %d times\n", cstat.failed);
    printf("pppd was %d times killed\n", cstat.killed);
}

void pack(char *file)
{
    int pid, stat_loc;

    pid = fork();
    if (pid == -1)
    {
    	perror("fork");
	exit(3);
    }
    else if (pid == 0)
    {
	execlp(ZIP, ZIP, "-q", file, NULL );
	perror("exec");
	exit(4);
    }
    else
	wait(&stat_loc);
}

void unpack(char *file)
{
    char path[PATH];
    int pid, stat_loc;

    strcpy(path, file);
    strcat(path, ".bz2");
    pid = fork();
    if (pid == -1)
    {
    	perror("fork");
    	exit(3);
    }
    else if (pid == 0)
    {
    	execlp(UNZIP, UNZIP, "-q", path, NULL );
	perror("exec");
	exit(4);
    }
    else
	wait(&stat_loc);
}

struct tm * str2tm(char *str)
{
    struct tm *tme;
    int i;
    char *p;

    tme = (struct tm *)calloc(1, sizeof(struct tm));

    for (i = 0; i < 12; i++)
	if ( !strncmp(str, month[i], 3) ) 
	{    
	    tme->tm_mon = i;
	    break;
	}
    if ( i != tme->tm_mon )
    {
	fprintf(stderr, "Error: wrong date type in str2tm\n");    
	exit(3);
    }

    p = str + 4;
    tme->tm_mday = atoi( p );
    
    p += 3;
    tme->tm_hour = atoi( p );
    
    p += 3;
    tme->tm_min = atoi( p );
    
    p+= 3;
    tme->tm_sec = atoi( p );
    
    return (tme);
}

void norm_cons(struct connection *top)
{
    struct connection *tmp, *prev = NULL;
    long a, b, c, d;
    double r;
    char str[NAME];

    while ( top )
    {	if ( top->start->tm_mday != top->end->tm_mday)
	{
	    tmp = (struct connection *) calloc( 1, sizeof(struct connection) );
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
	    top->inbyte = lrint( (1 - r) * top->inbyte );
	    top->outbyte = lrint( (1 - r) * top->outbyte );
	}
	top->dur = tm2sec(top->end) - tm2sec(top->start);
	if (top->dur < 0)
	{
	    strftime(str, LINE, "%b %e %T", top->start);
	    fprintf(stderr, "There was a mistake in your logs:\npppd session started on %s ", str);
	    str[0] = '\0';
	    strftime(str, LINE, "%b %e %T", top->end);
	    fprintf(stderr, "and closed on %s.\n", str);
	    fprintf(stderr, "I'll delete this connection 'cause of its useless.\nUse ntpd ;)\n");
	    prev->next = top->next;
	    free(top);
	    top = prev;
	}
	prev = top;
	top = top->next;
    }
}

long tm2sec(struct tm *tme)
{
    return ( tme->tm_sec + tme->tm_min * 60 + tme->tm_hour * 3600 );
}

void mkstat(struct connection *head, char sep_user, char sep_isp)
{
    struct connection *day, *prev;
    
    while ( head )
    {
	day = head->next;
	if ( !day )
	    break;
	prev = head;
	
	while ( day->start->tm_mday == head->start->tm_mday )
	{
	    if ( (!strcmp(head->user, day->user) || !sep_user) \
		&& (!strcmp(head->isp, day->isp) || !sep_isp) )
	    {
		head->inbyte += day->inbyte;
		head->outbyte += day->outbyte;
		head->dur += day->dur;
		head->iscon += day->iscon;
		
		prev->next = day->next;
		free(day);
		day = prev;
	    }
	    prev = day;
	    day = day->next;
	    if ( !day )
		break;
	}
	head = head->next;
    }
}
/*
char * get_cur_date(int i, struct tm * tme)
{
    char *s;
    time_t t;
    struct tm *ptm;

    s = (char *)malloc(NAME);
    t = time( NULL );
    t += i*24*60*60;
    tme = ptm = localtime( &t );
    strftime(s, NAME -1, "%b %e", ptm );
    return s;
}

char * add_day( int i, struct tm* tme)
{
    char *s;
    time_t t;
    struct tm *ptm;
    
    s = (char *)malloc(NAME);
    t = mktime( tme );
    t += i*24*60*60;
    tme = ptm = localtime( &t );
    strftime(s, NAME -1, "%b %e", ptm );
    return s;
}

int inc_day(char * day)
{
    char * mon;
    int dy;
    
    mon = strtok(day, " \t");

    if (dy < 32)
    	dy++;
    else
    {
	dy = 1;
	if (!strcmp(mon, "Jan")) strcpy(mon, "Feb");
	if (!strcmp(mon, "Feb")) strcpy(mon, "Mar");
	if (!strcmp(mon, "Mar")) strcpy(mon, "Apr");
	if (!strcmp(mon, "Apr")) strcpy(mon, "May");
	if (!strcmp(mon, "May")) strcpy(mon, "Jun");
	if (!strcmp(mon, "Jun")) strcpy(mon, "Jul");
	if (!strcmp(mon, "Jul")) strcpy(mon, "Aug");
	if (!strcmp(mon, "Aug")) strcpy(mon, "Sep");
	if (!strcmp(mon, "Sep")) strcpy(mon, "Oct");
	if (!strcmp(mon, "Oct")) strcpy(mon, "Nov");
	if (!strcmp(mon, "Nov")) strcpy(mon, "Dec");
	if (!strcmp(mon, "Dec")) strcpy(mon, "Jan");
    }

    sprintf(day, "%s %d", mon, dy);
    return (dy == 1)? 1 : 0;
}
*/
