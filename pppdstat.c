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
#define ISP "primary   DNS address" //ISP detecting
#define PPPD_STOP "Connect time" //А по этой - время соединения
#define LOGROTATE "/etc/logrotate.d/ppp_stat" //Файл настройки для logrotate

#define PATH 128
#define NAME 16
#define LINE 256
#define MAXUSR 8
#define DATE 6

#define UNZIP "bunzip"
#define ZIP "bzip"

struct connection
{
    float started;	
    char *user;
    char *isp;
    float time;
    struct connection *next;
};

void extract_logs(char *);

int main(int argc, char *argv[] )
{
    struct connection *head;

    extract_logs(LOGS);        
    
}

void extract_logs(char *logfile)
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
    
    if ( (pppdlog = fopen(PPPD_LOG, "w")) == NULL )
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
