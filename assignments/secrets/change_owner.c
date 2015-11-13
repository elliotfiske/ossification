#include <stdio.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

char *msg = "Hello, secret keeper!";

int main(int argc, char *argv[]) {
	int fd = open("/dev/Secret", O_WRONLY);
	int res, uid;

	printf("Opening... fd=%d\n",fd);
	res = write(fd, msg, strlen(msg));
	printf("Writing... res=%d\n",res);

	/* try grant */
	if ( argc > 1 && 0 != (uid = atoi(argv[1]))) {
		if ( res = ioctl(fd, SSGRANT, &uid) ) {
			perror("ioctl");
		}
		printf("Trying to change owner to %d...res=%d\n", uid, res);
	}	
	res = close(fd);

	return 0;
}

