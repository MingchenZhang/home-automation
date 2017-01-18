
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include "radio_level1_rf24Radio.h"
#include "radio_level1_control.h"
#include "radio_level1_broadcastProcess.h"
#include "radio_level1_forwarding.h"
#include "radio_level1_localReceive.h"
#include "radio_level1_init.h"
#include "util_debug.h"

static void displayHelp(){
	printf("run example: sudo ./radio_level1 localhost 5000\n");
	printf(	"param: -i: do not take input from stdin\n"\
			"param: -c: specify a channel for rf24 module\n");
}

int main(int argc, char ** argv, char **envp){
	int no_stdin = 0;
	channelUsed = 126;
	
	int opt;
	while((opt = getopt(argc, argv, "hic:")) != -1) {
       switch(opt) {
		case 'h':
			displayHelp();
			exit(EXIT_SUCCESS);
		case 'i':
			no_stdin = 1;
			break;
		case 'c':
			channelUsed = atoi(optarg);
			if(channelUsed < 0 || channelUsed > 127){
				displayHelp();
				printf("-c channel should be in range(0,128)\n");
				exit(EXIT_FAILURE);
			}
			break;
		case '?':
		default:
			displayHelp();
			exit(EXIT_FAILURE);
		}
	}
	if(optind < argc && (argc - optind) >= 2) {
        gatewayHostname = argv[optind++];
        gatewayPort = argv[optind++];
//		if((argc - optind) >= 1) userDatabase = argv[optind++];
    } else {
        displayHelp();
		exit(EXIT_FAILURE);
    }
	
	if(rf24Radio_init(0xaaaaaaaa02LL, 0xaaaaaaaa00LL, channelUsed)<0) {
		error("unable to initialize rf24 radio, exiting. \n");
		exit(EXIT_FAILURE);
	}
	broadcastProcess_init();
	forwarding_init(0xaaaaaaaa01LL);
	localReceive_init();
	
	control_loop(no_stdin);
	return -1;
}