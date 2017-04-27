//Parser of pppd logs. Makes stats of connections by user,ISP etc.

#define VERSION "0.4.1"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <getopt.h>

#define LOGS "/var/log/daemons/info" //here we can find pppd logs
#define PPPD_LOG "/var/log/pppd.log" //here we'll place pppd logs
#define PPPD "pppd[" //all strings from pppd
#define PPPD_START "started by" //after this we can find user name
#define ISP "remote IP address" //ISP detecting
#define TIME "Connect time" //time from pppd
#define EXIT "Exit." //Cases, when pppd failed to connect
#define PPPD_TIME "Connect time" //Self explaining. 
#define PPPD_OUTBYTE "Sent" //Sent bytes
#define PPPD_INBYTE "received" //Received bytes
#define LOGROTATE "/etc/logrotate.d/ppp_stat" //file for logrotate. Not used
#define CFG "/usr/local/etc/pppstat.conf" //here are located ISP configs
#define ISPNAME "name="// line in CFG, from which we'll get ISP name 
#define IP "IP="// line in CFG, from which we'll get ISP IP 
#define FROM "from " // line in CFG, from which we'll get ISP costs

#define PATH 128
#define NAME 16
#define LINE 256


#define UNZIP "bunzip" //Eah, such a stupid way to get into archived files
#define ZIP "bzip"

#define DEBUG
#undef DEBUG

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

struct cons //small stat on unsuccsessful connections
{
    long total;
    int failed;
    int killed;
} cstat = {0, 0, 0};

struct flags //I found that there are too many flags to pass, so I made a structure;
{
    unsigned int user : 1;
    unsigned int isp : 1;
    unsigned int apart : 1;
    unsigned int mounth : 1;
    unsigned int human : 1;
};

struct period 
{
    char from;
    char till;
    float cost;
};

struct isp //info about ISP modem pools to determine ISP
{
    char *name;
    char *ip[NAME];
    struct period *price[NAME];
    struct isp *next;
};


const char month[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

void extract_logs(void); //extract logs fron info
struct connection * parse_logs(void); //parse extracted logs

#ifdef DEBUG
void show_cons(struct connection *); //show connections - for debug
#endif

void pack(char *); //zip archive
void unpack(char *); //unzip archive
struct tm *str2tm(char *); //transform useless date string to convenient structure 
void norm_cons(struct connection *); //divide connections, is they spread on few days
long tm2sec(struct tm *); //convert time in tm to seconds
void sec2tm(struct tm *); //normalize time in structure
struct connection *mkstat(struct connection *, struct flags *); //make prestatistics
void show_stat(struct connection *, struct flags *); //print stats with
//or without users/isp
void statcpy(struct connection *dest, struct connection *source);//copy stat structure
void usage(void);//prints usage and exit
struct isp *get_isp(void); //get info about ISP modem pools from CFG file

#ifdef DEBUG
void show_isp(struct isp *); // shows the list
#endif

char *check_ip(char *, struct isp *); //checks, whether there is some ISP whith given IP

int main(int argc, char *argv[] )
{
    struct connection *head, *top; //start of list of cons
    struct flags fl = {0, 0, 0, 0, 0};
    char ch;
    
//getting command line

    if ( argc > 1 )
    {
	while ( (ch = getopt(argc, argv, "muih")) != -1 )	
    	    switch ( ch )
	    {
		case 'm':
		    fl.mounth = 1;
		    break;
		case 'u':
		    fl.user = 1;
		    break;
		case 'i':
		    fl.isp = 1;
		    break;
		case 'h':
		    fl.human = 1;
		    break;
		default:
		    fprintf(stderr, "Bad option %c", ch);
		    usage();
	    }
    }
    else
	usage();		
    
//extracting logs from archives

    extract_logs();
    
//lets roll!!    

    head = parse_logs();
    
#ifdef DEBUG
    showisp( getisp() );
    printf("\nSTAGE 1 - collected all connections:\n");
    show_cons(head);
#endif

    norm_cons(head);
    
#ifdef DEBUG
    printf("\nSTAGE 2 - normalize connections:\n");
    show_cons(head);
#endif

    top = mkstat(head, &fl);

#ifdef DEBUG
    printf("\nSTAGE 3 - counted specified stats (user=%d, isp=%d)per day:\n", fl.user, fl.isp);
    show_cons(top);
    fprintf(stderr, "STAGE 3 finished!\n");
#endif

    show_stat(head, &fl);	
    
    
}

void extract_logs(void)
{
    FILE *log, *pppdlog;
    char line[LINE], msg[LINE] = {0};
    char file[PATH];
    int i, reach = 0;
    
    if ( (pppdlog = fopen(PPPD_LOG, "r+")) == NULL )
    {
	if ( errno == ENOENT )
	    pppdlog = fopen(PPPD_LOG, "a");
	else
	{
	    strcpy(msg, "open ");
	    strcat(msg, PPPD_LOG);
	    perror(msg);
	    exit(1);
	}
    }
    else
	while ( fgets(line, LINE - 1, pppdlog) )
	    strcpy(msg, line);
    
    for (i = 5; i > 0; i--) 
    {
    	*file = '\0';
	sprintf(file, "%s.%d", LOGS, i);
	
	unpack(file);
	    
	if ( (log = fopen(file, "r")) == NULL )
	    if ( errno == ENOENT )
		continue;
	    else
	    {
		strcpy(msg, "open ");
		strcat(msg, file);
		perror(msg);
		exit(1);
	    }
    
	fprintf(stderr, "Extracting from %s ...\n", file);

	while ( fgets(line, LINE-1, log) != NULL )
	    if ( strstr(line, PPPD) != NULL )
	        if ( !msg[0] || reach || !strcmp(line, msg)  )
		{	
		    if ( !reach && msg[0] )
		    {
		        reach = 1;
		        continue;
		    }
		    
		    if ( fputs(line, pppdlog) == EOF )
		    {
		        strcpy(msg, "write ");
		        strcat(msg, PPPD_LOG);
		        perror(msg);
		        exit(2);
		    }
		}
    
	fclose(log);
	
	pack(file);
    }
    
    *file = '\0';
    sprintf(file, "%s", LOGS);
    
    if ( (log = fopen(file, "r")) == NULL )
    {
        strcpy(msg, "open ");
        strcat(msg, file);
        perror(msg);
        exit(1);
    }
    
    fprintf(stderr, "Extracting from %s ...\n", file);
    
    while ( fgets(line, LINE-1, log) != NULL )
	if ( strstr(line, PPPD) != NULL )
	    if ( !msg[0] || reach || !strcmp(line, msg)  )
	    {	
	        if ( !reach && msg[0] )
	        {
		    reach = 1;
		    continue;
	    	}
		    
		if ( fputs(line, pppdlog) == EOF )
		{
		    strcpy(msg, "write ");
		    strcat(msg, PPPD_LOG);
		    perror(msg);
		    exit(2);
		}
	    }
    
    fclose(log);
	
    fclose(pppdlog);
}

struct connection * parse_logs(void)
{
    FILE *pppdlog;
    char msg[PATH], line[LINE], *p;
    struct connection *tmp, *head, *current;
    char started = 0, connected = 0;
    
    struct tm *date;
    
    struct isp *isp_list;

    isp_list = get_isp();
    
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
		    if ( p = check_ip(tmp->isp, isp_list) )
			strncpy(tmp->isp, p, NAME-1);
		    tmp->start = str2tm(line);
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
	    	if ( ( (p = strstr(line, PPPD_TIME)) ) != NULL )
		{
		    tmp->pppd_dur = (long)( 60 * atof(p + strlen(PPPD_TIME)) );
		    tmp->end = str2tm(line);
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

#ifdef DEBUG
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
	printf("pd_dur:\t%ld secs\n", top->pppd_dur);
	printf("dur:\t%ld secs\n", top->dur);
	printf("out:\t%ld\n", top->outbyte);
	printf("in:\t%ld\n", top->inbyte);
	printf("is con\t%d\n", top->iscon);
	top = top->next;
    }
    printf("\ntotal starts of pppd: %d\n", cstat.total);
    printf("failed to connect %d times\n", cstat.failed);
    printf("pppd was %d times killed\n", cstat.killed);
}
#endif

void show_stat(struct connection *top, struct flags *f)
{
    char str[LINE], str1[16];
    
    while ( top )
    {
        strftime(str, LINE, "%b %e", top->start);
	if ( f->mounth )
	{
	    if (top->iscon == 1)
		top->end = top->start->tm_mday;
	    sprintf(str1, " - %d", top->end);
	    strcat( str, str1 );
	}
	printf("\nPeriod:\t\t%s\n", str);
	if ( f->user ) printf("user:\t\t%s\n", top->user);
	if ( f->isp ) printf("ISP:\t\t%s\n", top->isp);
	if (f->human )
	{
	    printf("in:\t\t%.2f %cbytes\n", (float)top->inbyte / \
		    (1024 * (f->mounth ? 1024 : 1)), f->mounth ? 'M' : 'k' );
	    printf("out:\t\t%.2f %cbytes\n", (float)top->outbyte / \
		    (1024 * (f->mounth ? 1024 : 1)), f->mounth ? 'M' : 'k' );
		    
	    top->start->tm_sec = top->dur;
	    
	    sec2tm(top->start);
	    strftime(str, LINE, "%T", top->start);
	    printf("time:\t\t%s\n", str);
	}
	else
	{
	    printf("in:\t\t%ld bytes\n", top->inbyte);
	    printf("out:\t\t%ld bytes\n", top->outbyte);
	    printf("time:\t\t%ld secs\n", top->dur);
	}
	printf("connects:\t%d\n", top->iscon);	
	top = top->next;
    }
    printf("\ntotal starts of pppd: %d\n", cstat.total);
    printf("failed to connect %d times\n", cstat.failed);
    printf("pppd was %d times killed\n\n", cstat.killed);
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
    int pid, stat_loc;
    char path[PATH];
    
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

struct tm *str2tm(char *str)
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
		fprintf(stderr, "\nYour logs aren't punctual. I'll use time, reported by pppd, this may cause 3sec deviation.\n");
		fprintf(stderr, "Use ntpd ;)\n");
		unpunct = 1;
	    }
	    if( (top->dur = top->pppd_dur) < 0 )
	    {
		top->dur = 0;
		fprintf(stderr, "Some connects have negative time. Making it 0. \n");
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

void sec2tm(struct tm *tme)
{
    tme->tm_min = tme->tm_sec / 60;
    tme->tm_sec = tme->tm_sec % 60;
    
    tme->tm_hour = tme->tm_min / 60;
    tme->tm_min = tme->tm_min % 60;
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

void usage(void)
{
    fprintf(stderr, "\nPPPstat version %s. http://pppstat.sourceforge.net\n", VERSION);
    fprintf(stderr, "Distributed under GPL.\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "pppstat [-u] [-i] [-m] [-h]\n");
    fprintf(stderr, "-u\tcounts all connections for each user per period\n");
    fprintf(stderr, "-i\tcounts all connections for each ISP per period\n");
    fprintf(stderr, "-m\tdefines period as mounth, otherwise as day\n");
    fprintf(stderr, "-h\tmakes human-readable output\n");
    putc('\n', stderr);
    exit(4);
}

struct isp *get_isp(void)
{
    FILE *cfg;
    char line[LINE], *p, *p1;
    struct isp *head, *isp = NULL;
    int len, i, j;
    
    if ( (cfg = fopen(CFG, "r")) == NULL )
    {
	strcpy(line, "open ");
	strcat(line, CFG);
	perror(line);
	exit(1);
    }
    
    while ( fgets(line, LINE-1, cfg) != NULL  )
    {
	if ( line[0] == '#' ) continue;
	if ( !(p = strstr(line, ISPNAME)) && !isp ) continue;
	if ( p1 = strchr(line, '#') ) *p1 = '\0';
	if ( p )
	{
	    if (!isp)
		head = isp = (struct isp *)calloc( 1, sizeof(struct isp) );
	    else
		isp = isp->next = (struct isp *)calloc( 1, sizeof(struct isp) );
		
	    p += strlen(ISPNAME);
	    
	    if ( p1 = strchr(line, ';') ) 
		len = p1 - p;
	    else
	    {
		if ( p1 = strchr(p, ' ') ) *p1 = '\0';
		len = strlen(p);
	    }	    
	    
	    isp->name = (char *)calloc( 1, len );
	    strncpy(isp->name, p, len);
	    
	    i = j = 0;
	}
	else if ( p = strstr(line, IP) )
	{
	    p += strlen(IP);
	    if ( p1 = strchr(line, ';') ) 
		len = p1 - p;
	    else
	    {
		if ( p1 = strchr(p, ' ') ) *p1 = '\0';
		len = strlen(p);
	    }
	    	
	    isp->ip[i] = (char *)calloc( 1, len );
	    strncpy(isp->ip[i], p, len);
	    i++;
	}
	else if ( p = strstr(line, FROM) )
	{
	    isp->price[j] = (struct period *)calloc( 1, sizeof(struct period) );
	    
	    sscanf(p, "from %d till %d costs %f", &(isp->price[j]->from), \
						    &(isp->price[j]->till), \
						    &(isp->price[j]->cost));
	    j++;
	}
    }
    fclose (cfg);
    
    return (head);
}		

char *check_ip(char *ip, struct isp *top)
{
    int i;

    while ( top )
    {	
	i = 0;
	
	while ( top->ip[i] )
	    if ( !strcmp(ip, top->ip[i++]) ) 
		return top->name;
		
	top = top->next;
    }
    
    return NULL;
}

#ifdef DEBUG
void show_isp(struct isp *top)
{
    int i = 0;
    
    while (top)
    {
	printf("\n%s\n", top->name);
	while ( top->ip[i] )
	    printf("%s\n", top->ip[i++]);
	i = 0;
	top = top->next;
    }
}
#endif
