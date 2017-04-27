//Parser of pppd logs. Makes stats of connections by user,ISP etc.

#include <getopt.h>

#include "main.h"
#include "stat.h"
#include "output.h"
#include "filework.h"

struct flags fl = {0, 0, 0, 0, 0, 0};

int main(int argc, char *argv[] )
{
    struct connection *head, *top; //start of list of cons
    char ch;
    
    int opt_idx = 0;    
    static struct option long_opt[] = {
	{"month", 0, 0, 'm'},	
	{"user", 0, 0, 'u'},
	{"isp", 0, 0, 'i'},
	{"human", 0, 0, 'h'},
	{"debug", 0, 0, 'd'},
	{"help", 0, 0, 'H'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
    };
    
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    bindtextdomain(PACKAGE,LOCALEDIR);
    textdomain(PACKAGE);
    
//getting command line

    if ( argc > 1 )
    {
	while ( (ch = getopt_long(argc, argv, "muihdv", long_opt, &opt_idx)) != -1 )	
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
		case 'd':
		    fl.debug = 1;
		    break;
		case 'H':
		    usage();
		    break;
		case 'v':
		    version();
		    break;
		default:
		    fprintf(stderr, _("Bad option %c\n"), ch);
		    usage();
	    }
    }
    else
	usage();		

//extracting logs from archives

    extract_logs();
    
//lets roll!!    

    head = parse_logs();
    
    if ( fl.debug )
    { 
	show_isp( get_isp() );
	printf(_("\nSTAGE 1 - collected all connections:\n"));
	show_cons(head);
    }

    norm_cons(head);
    
    if ( fl.debug )
    {
        printf(_("\nSTAGE 2 - normalize connections:\n"));
	show_cons(head);
    }

    top = mkstat(head, &fl);


    if ( fl.debug )
    {
	printf(_("\nSTAGE 3 - counted specified stats (user=%d, isp=%d)per day:\n"), fl.user, fl.isp);
        show_cons(top);
	fprintf(stderr, _("STAGE 3 finished!\n"));
    }
    
    show_stat(head, &fl);	

    exit(0);
}

