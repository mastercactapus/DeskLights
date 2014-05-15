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
int read_settings(int fd, struct Settings *settings);
void print_settings(struct Settings *settings);
void print_settings_hex(struct Settings *settings);
int write_settings(int fd, char *settings);

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
			if (strcmp(argv[3], "pwm") == 0 && argc < 5) {
				printf("Invalid number of parameters for all pwm\n");
				usage(argv[0]);
				close(fd);
				return 1;
			}
			result = all_command(fd, &argv[3]);

		} else if (strcmp(argv[2], "settings") == 0 && strcmp(argv[3], "read") == 0) {
			struct Settings settings;
			int n = read_settings(fd, &settings);
			if (n != 0) {
				close(fd);
				return 1;
			}
			if (argc >= 5 && strcmp(argv[4], "hex") == 0) {
				print_settings_hex(&settings);
			} else {
				print_settings(&settings);
			}
		} else if (strcmp(argv[2], "settings") == 0 && strcmp(argv[3], "write") == 0 && argc > 4) {
			result = write_settings(fd, argv[4]);
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

int write_settings(int fd, char *settings) {
	if (strlen(settings) != sizeof(struct Settings) * 2) {
		fprintf(stderr, "Invalid length for settings");
		return -1;
	}

	struct Settings new_settings;
	int i;
	for (i=0; i < sizeof(struct Settings); i++) {
		sscanf(&settings[i*2], "%2x", &((char*)&new_settings)[i]);
	}

	printf("New Settings:\n");
	print_settings(&new_settings);

	struct Command cmd = { HEAD, CMD_SETTINGS_WRITE, 0, 0 };
	int n = write(fd, &cmd, 4) + write(fd, &new_settings, sizeof(struct Settings));

	if (n != 4 + sizeof(struct Settings)) {
		perror("write_settings: failed to update settings");
		return -1;
	}

	return 0;
}

int read_settings(int fd, struct Settings *settings) {
	struct Command cmd = { HEAD, CMD_SETTINGS_READ, 0, 0 };

	int n = write(fd, &cmd, 4);
	if (n != 4) {
		perror("light_command: failed to send command");
		return -1;
	}

	n = read(fd, settings, sizeof(struct Settings));
	if (n != sizeof(struct Settings)) {
		perror("read_settings: failed to read settings");
		return -1;
	}

	return 0;
}

void print_settings(struct Settings *settings){
	int i;
	for (i = 0; i < 3; i++) {
		printf("Light%i: %s (PWM: %u)\n", i+1, settings->lights[i] == LIGHT_ON ? "on" : "off", settings->pwm[i]);
	}
}
void print_settings_hex(struct Settings *settings){
	unsigned char *ptr = (char *)settings;

	int i;
	for (i = 0; i < sizeof(struct Settings); i++) {
		printf("%hhx", *ptr++);
	}
	printf("\n");
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
	    options.c_cc[VTIME] = 5;

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
