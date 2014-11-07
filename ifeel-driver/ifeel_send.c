#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>                  /* For exit to be declared */
#include "ifeel.h"

int main(int argc, char* argv[]) {

	int ifeel;
	
	struct ifeel_command command;
	
	if (argc != 4) {
		printf("useage = %s <strength> <delay> <count>\n", argv[0]);
		exit(0);
	}
	
	command.strength = atoi(argv[1]);
	command.delay = atoi(argv[2]);
	command.count = atoi(argv[3]);

	if ((ifeel = open("/dev/input/ifeel0", O_RDWR | O_NONBLOCK, 0)) <= 0) {
		printf("ERROR %s\n", strerror(errno));
		return 0;
	}
	
	if (ioctl(ifeel, USB_IFEEL_BUZZ_IOCTL, &command) < 0) {
		printf("ERROR %s\n", strerror(errno));
		return 0;
	}
	
	return 0;
};
