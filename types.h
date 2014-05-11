//serial stuff (HEAD:CMD:LIGHT:PWM)
#define HEAD 0x42

#define CMD_ON 0x10
#define CMD_OFF 0x11
#define CMD_PWM 0x12

#define CMD_ALL_ON 0x20
#define CMD_ALL_OFF 0x21
#define CMD_ALL_PWM 0x22
#define CMD_ALL_CLEAR 0x23

#define CMD_SETTINGS_READ 0x35
#define CMD_SETTINGS_WRITE 0x36

#define CMD_PING 0x41
#define CMD_PONG 0x42

#define LIGHT_ON 0xFF
#define LIGHT_OFF 0x00

#define PWM_FULL 0xFF
#define PWM_OFF 0x00

#define DEFAULT_SETTINGS { {LIGHT_OFF, LIGHT_OFF, LIGHT_OFF}, {PWM_FULL, PWM_FULL, PWM_FULL} }

#define SWITCH_UNKNOWN 0x00
#define SWITCH_ON 0x01
#define SWITCH_OFF 0x02

struct Command {
  char head;
  char cmd;
  char light;
  char pwm;
};

struct Settings {
  byte lights[3];
  byte pwm[3];
};
