#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

typedef unsigned char byte;

#include "types.h"

void usage(char* cmdname);
int open_port(char* device);
int ping_pong(int fd);
int light_command(int fd, char *argv[]);
int set_opts(char *argv[], struct Command *cmd);


int main(int argc, char *argv[]) {
	int fd = -1;

	int result = 0;
	if (argc < 4) {
		usage(argv[0]);
	} else {
		fd = open_port(argv[1]);
		if (fd == -1) {
			printf("Could not open device: %s\n", argv[1]);
			return 1;
		}
		if (ping_pong(fd) != 0) {
			printf("Could not communicate with device: %s\n", argv[1]);
			close(fd);
			return 1;
		}

		if (strcmp(argv[2], "light") == 0 && argc >= 5) {
			if (strcmp(argv[4], "pwm") == 0 && argc < 6) {
				printf("Invalid number of parameters for light pwm\n");
				usage(argv[0]);
				close(fd);
				return 1;
			}

			result = light_command(fd, &argv[3]);
		} else if (strcmp(argv[2], "all") == 0) {

			result = all_command(fd, &argv[3]);

		} else if (strcmp(argv[2], "settings") == 0 && strcmp(argv[3], "read") == 0) {
			
		} else if (strcmp(argv[2], "settings") == 0 && strcmp(argv[3], "write") == 0) {
			
		} else {

			printf("Invalid command\n\n");
			usage(argv[0]);
		}
	}

	if (fd != -1) {
		close(fd);
	}

	return result;
}

void usage(char* cmdname) {
	printf("Usage:\n");
	printf("\t%s <device> light 1|2|3 on|off|pwm [pwm value]\n", cmdname);
	printf("\t%s <device> all on|off|pwm|clear [pwm value]\n", cmdname);
	printf("\t%s <device> settings read [readable]\n", cmdname);
	printf("\t%s <device> settings write <hex values>\n", cmdname);
}

int light_command(int fd, char *argv[]) {
	int light = atoi(argv[0]) - 1;

	if (light > 2 || light < 0) {
		fprintf(stderr, "light can only be 1-3\n");
		return -1;
	}

	struct Command cmd = { HEAD, 0, 0, 0 };
	cmd.light = (char)light;

	int n = set_opts(&argv[1], &cmd);
	if (n != 0) {
		return -1;
	}

	if (cmd.cmd == CMD_ALL_CLEAR) {
		fprintf(stderr, "clear only works with 'all'");
		return -1;
	}
	cmd.cmd -= 0x10;

	n = write(fd, &cmd, 4);
	if (n != 4) {
		perror("light_command: failed to send command");
		return -1;
	}

	return 0;
}

int all_command(int fd, char *argv[]) {
	struct Command cmd = { HEAD, 0, 0, 0 };

	int n = set_opts(argv, &cmd);
	if (n != 0) {
		return -1;
	}

	n = write(fd, &cmd, 4);
	if (n != 4) {
		perror("all_command: failed to send command");
		return -1;
	}

	return 0;
}

int set_opts(char *argv[], struct Command *cmd) {
	if (strcmp(argv[0], "on") == 0) {
		cmd->cmd = CMD_ALL_ON;
	} else if (strcmp(argv[0], "off") == 0) {
		cmd->cmd = CMD_ALL_OFF;
	} else if (strcmp(argv[0], "pwm") == 0) {
		cmd->cmd = CMD_ALL_PWM;

		int pwm = atoi(argv[1]);

		if (pwm < 0 || pwm > 255) {
			fprintf(stderr, "pwm must be between 0-255");
			return -1;
		}
		cmd->pwm = (char)pwm;
	} else if (strcmp(argv[0], "clear") == 0) {
		cmd->cmd = CMD_ALL_CLEAR;
	} else {
		fprintf(stderr, "invalid command");
		return -1;
	}

	return 0;
}

int open_port(char* device) {
	int fd = open(device, O_RDWR | O_NOCTTY);
	if (fd == -1) {
		perror("open_port: Unable to open serial device");
	} else {
		fcntl(fd, F_SETFL, 0);

		struct termios options;
		tcgetattr(fd, &options);

		cfsetispeed(&options, B115200);
		cfsetospeed(&options, B115200);

	    options.c_cflag &= ~PARENB;
	    options.c_cflag &= ~CSTOPB;
	    options.c_cflag &= ~CSIZE;
	    options.c_cflag |= CS8;
	    // no flow control
	    options.c_cflag &= ~CRTSCTS;

	    options.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines
	    options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

	    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	    options.c_oflag &= ~OPOST; // make raw

	    options.c_cc[VMIN] = 0;
	    options.c_cc[VTIME] = 10;

		tcsetattr(fd, TCSANOW, &options);

	    if( tcsetattr(fd, TCSAFLUSH, &options) < 0) {
	        perror("open_port: failed to set options");
	        close(fd);
	        return -1;
	    }

		return fd;
	}
}

int ping_pong(int fd) {
	char pong[1];
	char ping[] = { HEAD, CMD_PING, 0, 0 };
	int n;

	n = write(fd, ping, 4);
	if (n != 4) {
		perror("ping_pong: failed to send ping");
		return -1;
	}

	n = read(fd, pong, 1);
	if (n != 1) {
		perror("ping_pong: failed to read pong");
		return -1;
	}

	if (pong[0] != CMD_PONG) {
		printf("ping_pong: data received was not CMD_PONG\n");
		return -1;
	}

	return 0;
}
