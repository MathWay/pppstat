//Парсер логов pppd на предмет составления статистики соединений
//по пользователям, прову и т. д.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOGS "/var/log/daemons/info" //Где лежат логи демонов (и pppd)
#define PPPD_LOG "/var/log/pppd.log" //Где будет лежать статистика pppd
#define PPPD "pppd[" //all strings from pppd
#define PPPD_START "started by" //По этой строке определим юзверя
#define ISP "remote IP address" //ISP detecting
#define EXIT "Exit." //Cases, when pppd failed to connect
#define PPPD_TIME "Connect time" //А по этой - время соединения
#define PPPD_OUTBYTE "Sent" //Sent bytes
#define PPPD_INBYTE "received" //Received bytes
//#define LOGROTATE "/etc/logrotate.d/ppp_stat" //Файл настройки для logrotate

#define PATH 128
#define NAME 16
#define LINE 256
#define MAXUSR 8
//#define DATE 6

#define UNZIP "bunzip"
#define ZIP "bzip"

struct connection
{
    char *user;//[NAME];
    char *isp;//[NAME];
    char start[NAME];
    char end[NAME];
    long inbyte;
    long outbyte;
    struct connection *next;
};

void extract_logs(char *, char *);
struct connection * parse_logs(void);
void pack(char *);
void unpack(char *);
//char * get_cur_date(int, struct tm *);
//int inc_day(char * day);

int main(int argc, char *argv[] )
{
	struct connection *head;
 	char file[PATH];
  	int i;

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
		fprintf(stderr, "warning: pppd was abnormally terminated on %s\n", tmp->start);
		connected = 0;
	    }
	    p += strlen(PPPD_START) + 1;
	    /*tmp->user = */printf("\nuser:\t%s\n", strtok(p, " ,"));
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
		    /*tmp->isp = */printf("ISP:\t%s\n", strtok(p, " ,\n"));
		    strncpy(tmp->start, line, NAME-1);
		    printf("start:\t%s\n", tmp->start);
		    continue;
		}
		else if ( strstr(line, EXIT) != NULL )
		{
		    started = 0;
		    fprintf(stderr, "pppd failed to connect on %s\n", strncpy(tmp->start, line, NAME-1));
		    continue;
		}
	    }
	    else
	    {
	    	if ( (strstr(line, PPPD_TIME)) != NULL )
		{
		    strncpy(tmp->end, line, NAME-1);
		    printf("end:\t%s\n", tmp->end);
		    continue;
		}
		else if ( (p = strstr(line, PPPD_OUTBYTE)) != NULL )
		{
		    p += strlen(PPPD_OUTBYTE) + 1;
		    /*tmp->outbyte = */printf("out:\t%ld\n", atol( p ) );
		    p = strstr(line, PPPD_INBYTE);
		    p += strlen(PPPD_INBYTE) + 1;
		    /*tmp->inbyte = */printf("in:\t%ld\n", atol( p ) );
		    //current->next = tmp;
		    //current = tmp;
		    //tmp = (struct connection *) calloc( 1, sizeof(struct connection) );
		    connected = started = 0;
		    continue;
		}
	    }
	    
	}    
    }
    		    		
    return (head);
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
