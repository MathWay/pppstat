// Functions to work with files

#define PPPD_LOG "/var/log/pppd.log" //here we'll place pppd logs

#define SYSLOG_CFG "/etc/syslog.conf" //syslog config file
#define INFO_1 "*.info"
#define INFO_2 "*.=info" //two possible rules for info messages in SYSLOG_CFG
#define LOGS "/var/log/messages" //here we can find pppd logs
#define LOGROTATE "/etc/logrotate.d/syslogd" //logrotate rule file for syslogd

#define PPPD "pppd[" //all strings from pppd
#define PPPD_START "started by" //after this we can find user name
#define ISP "remote IP address" //ISP detecting
#define EXIT "Exit." //Cases, when pppd failed to connect
#define PPPD_TIME "Connect time" //Self explaining. 
#define PPPD_OUTBYTE "Sent" //Sent bytes
#define PPPD_INBYTE "received" //Received bytes

#define ISPNAME "name="// line in PPPSTAT_CFG, from which we'll get ISP name 
#define IP "IP="// line in PPPSTAT_CFG, from which we'll get ISP IP 
#define FROM "from " // line in PPPSTAT_CFG, from which we'll get ISP costs

#include "filework.h"
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void pack(char *); //zip archive
void unpack(char *); //unzip archive
char *check_ip(char *, struct isp *); //checks, whether there is some ISP whith given IP
struct tm *str2tm(char *); //transform useless date string to convenient structure 
int file_exists_p(const char *filename); //Self-explanatory

char *find_log(char * ext, int *num); //find, where log file "messages" is located, 
			//what packer is used and number of rotations.

struct cons cstat = {0, 0, 0};
const char month[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

void extract_logs(void)
{
    FILE *log, *pppdlog;
    char line[LINE], msg[LINE] = {0};
    char file[PATH], *logs;
    int i, reach = 0;
    
    if ( (pppdlog = fopen(PPPD_LOG, "r+")) == NULL )
    {
	if ( errno == ENOENT )
	    pppdlog = fopen(PPPD_LOG, "a");
	else
	{
	    strcpy(msg, _("open "));
	    strcat(msg, PPPD_LOG);
	    perror(msg);
	    exit(1);
	}
    }
    else
	while ( fgets(line, LINE - 1, pppdlog) )
	    strcpy(msg, line);
    
    // Research about sysloging
        
    logs = find_log(file, &i);
    
    for (i = 5; i >= 0; i--) 
    {
    	*file = '\0';
	sprintf(file, "%s.%d", logs, i);
	
	unpack(file);

	if ( !file_exists_p(file) )
	    continue;
	    
	if ( (log = fopen(file, "r")) == NULL )
	{
	    strcpy(msg, _("open "));
	    strcat(msg, file);
	    perror(msg);
	    exit(1);
	}
    
	fprintf(stderr, _("Extracting from %s ...\n"), file);

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
		        strcpy(msg, _("write "));
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
        strcpy(msg, _("open "));
        strcat(msg, file);
        perror(msg);
        exit(1);
    }
    
    fprintf(stderr, _("Extracting from %s ...\n"), file);
    
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
		    strcpy(msg, _("write "));
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
	strcpy(msg, _("open "));
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
//Crazy workaround, caused by some gcc bug.
		    //puts(p + strlen(PPPD_TIME));
		    //tmp->pppd_dur = (long)( 60 * atof(p + strlen(PPPD_TIME) + 1) );
		    float t;
		    sscanf(p + strlen(PPPD_TIME), " %f", &t);
		    //printf("%f\n", t);
		    tmp->pppd_dur = (long)(60 * t);
		    //printf("%f\n", atof( p + strlen(PPPD_TIME) + 1) );
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
    
//    show_cons(head);
    			    		
    return (head);
}

static char unpacked, bzip;

void pack(char *file)
{
    int pid, stat_loc;

    if (!unpacked)
	return;

    pid = fork();
    if (pid == -1)
    {
    	perror("fork");
	exit(3);
    }
    else if (pid == 0)
    {
	if ( bzip )
	    execlp(BZIP, BZIP, "-q", file, NULL );
	else 
	    execlp(GZIP, GZIP, "-q", file, NULL );
	perror("exec");
	exit(4);
    }
    else
	wait(&stat_loc);
    bzip = unpacked = 0;
}

void unpack(char *file)
{
    int pid, stat_loc;
    char path[PATH];
    
    strcpy(path, file);
    strcat(path, ".gz");
    
    if (!file_exists_p(path))
    {
	strcpy(path, file);
	strcat(path, ".bz2");
	
	if (!file_exists_p(path))
	    return;
	
	bzip = 1;
    }
    
    pid = fork();
    if (pid == -1)
    {
    	perror("fork");
    	exit(3);
    }
    else if (pid == 0)
    {
	if ( bzip )
	    execlp(BUNZIP, BUNZIP, "-q", path, NULL );
	else 
	    execlp(GUNZIP, GUNZIP, "-q", path, NULL );
    
	perror("exec");
	exit(4);
    }
    else
	wait(&stat_loc);
	
    unpacked = 1;
}

struct isp *get_isp(void)
{
    FILE *cfg;
    char line[LINE], *p, *p1;
    struct isp *head, *isp = NULL;
    int len, i, j;
    
    if ( (cfg = fopen(PPPSTAT_CFG, "r")) == NULL )
    {
	strcpy(line, _("open "));
	strcat(line, PPPSTAT_CFG);
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
	    	
	    isp->ip[i] = (char *)calloc( 1, len + 1 );
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
	fprintf(stderr, _("Error: wrong date type in str2tm\n"));    
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

// Was stolen from wget. Thanks Hrvoje Niksic for code.

/* Does FILENAME exist?  This is quite a lousy implementation, since
   it supplies no error codes -- only a yes-or-no answer.  Thus it
   will return that a file does not exist if, e.g., the directory is
   unreadable.  I don't mind it too much currently, though.  The
   proper way should, of course, be to have a third, error state,
   other than true/false, but that would introduce uncalled-for
   additional complexity to the callers.  */
int file_exists_p(const char *filename)
{
  struct stat buf;
  
  return stat(filename, &buf) >= 0;
}

char *find_log(char *ext, int *num)
{
    FILE *cfg;
    char line[LINE], *p, *messages, flag = 0;

    // Checking in /etc/syslog.conf

    if ( (cfg = fopen(SYSLOG_CFG, "r")) == NULL )
    {
	strcpy(line, _("open "));
	strcat(line, SYSLOG_CFG);
	perror(line);
	exit(1);
    }
    
    while ( fgets(line, LINE-1, cfg) != NULL  )
    {
	if ( line[0] == '#' ) continue;
	if ( strstr(line, INFO_1) || strstr(line, INFO_2) ) flag++;
	if ( flag )
	    if ( p = strchr(line, '/') )
	    {
		messages = (char *)calloc(1, PATH);
		strcpy(messages, p);
		messages[strlen(messages) - 1] = '\0';
		
		if ( !file_exists_p(messages) )
		{
		    fprintf(stderr, _("Warning: failed to parse %s, falling to defaults. Bugrepot, please.\n"), SYSLOG_CFG);
		    strcpy(messages, LOGS);
		}
		break;
	    }
    }

    // Checking in /etc/logrotate.d

    if ( file_exists_p(LOGROTATE) )
    {	
    if ( (cfg = fopen(SYSLOG_CFG, "r")) == NULL )
	{
	    strcpy(line, _("open "));
	    strcat(line, SYSLOG_CFG);
	    perror(line);
	    exit(1);
	}
    
/*	while ( fgets(line, LINE-1, cfg) != NULL  )
	{
	    if ( line[0] == '#' ) continue;
	    
*/	
    }
    return messages;
    // Checking in /etc/cron.weekly/sysklogd



}
