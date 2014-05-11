#include <EEPROM.h>

#include "/home/nathan/sketchbook/DeskLights/types.h"

/* Nate an Kat's light control */

void updateLights();

// nate1, nate2, kat
const int ledPins[] = {
  11, 10, 9};
const int swPins[] = {
  12, 7, 4};
const unsigned long timeout = 400;

byte swState[] = {
  SWITCH_UNKNOWN, SWITCH_UNKNOWN, SWITCH_UNKNOWN};
unsigned long swTime[2];

struct Settings settings = DEFAULT_SETTINGS;

void saveSettings() {
  EEPROM.write(0, HEAD);

  for (unsigned int x = 0; x < sizeof(Settings); x++) {
    EEPROM.write(x+1, ((byte *) &settings)[x]);
  }
}
void loadSettings() {
  //dont load if not set
  if (EEPROM.read(0) == HEAD) {
    for (unsigned int x = 0; x < sizeof(Settings); x++) {
      ((byte *) &settings)[x] = EEPROM.read(x+1);
    }
  }
}
void clearAll() {
  for (int x =0; x < 10; x++) {
    EEPROM.write(x, 0xFF);
    
    if (x < 3) {
      settings.pwm[x] = PWM_FULL;
      settings.lights[x] = LIGHT_OFF;
    }
  }
}

void setup() {
  // put your setup code here, to run once:

  for (unsigned char x = 0; x < 3; x++) {
    analogWrite(ledPins[x], PWM_OFF);
    pinMode(ledPins[x], OUTPUT);
    pinMode(swPins[x], INPUT);
  }

  loadSettings();
  Serial.begin(115200);
  Serial.write("setup\n");

}

int nswState(int idx, byte state) {
  if (state == SWITCH_ON && swState[idx] != SWITCH_ON) {
  	Serial.write("nate btn down\n");
    swState[idx] = SWITCH_ON;

    swTime[idx] = millis();	
  } 
  else if (state == SWITCH_OFF && swState[idx] != SWITCH_OFF) {
  Serial.write("nate btn up\n");
    swState[idx] = SWITCH_OFF;

    unsigned long time = millis() - swTime[idx];
    if (time > 5000) {
      settings.pwm[0] = PWM_FULL;

      settings.lights[0] = LIGHT_ON;
      updateLights();
      delay(100);
      settings.lights[0] = LIGHT_OFF;
      updateLights();
      delay(100);
      settings.lights[0] = LIGHT_ON;
      updateLights();
      delay(100);
      settings.lights[0] = LIGHT_OFF;
      updateLights();
      delay(100);
      settings.lights[0] = LIGHT_ON;
      updateLights();
      delay(100);
      settings.lights[0] = LIGHT_OFF;
      updateLights();
      clearAll();			
    } else if (time > timeout) {
      settings.lights[idx] = ~settings.lights[idx];
    } else if (time > 50) {
      byte newState = ~(settings.lights[0] | settings.lights[1]);
      settings.lights[0] = newState;
      settings.lights[1] = newState;
    } else {
    	return 0;
    }

    return 1;
  }
  return 0;
}

int kswState(byte state) {
  if (swState[2] != state) {
    settings.lights[2] = (state == SWITCH_ON) ? LIGHT_ON : LIGHT_OFF;
    swState[2] = state;
    return 1;
  }

  return 0;
}

void updateLights() {
  Serial.write("updateLights()\n");
  for (int x = 0; x < 3; x++) {
    if (settings.lights[x] == LIGHT_ON) {
      analogWrite(ledPins[x], settings.pwm[x]);
    } 
    else {
      analogWrite(ledPins[x], PWM_OFF);
    }
  }
  saveSettings();
}

void loop() {
  int update = 0;
  // put your main code here, to run repeatedly:
  for (int x = 0; x < 2; x++) {
    update += nswState(x, (LOW == digitalRead(swPins[x])) ? SWITCH_ON : SWITCH_OFF);
  }
  update += kswState((LOW == digitalRead(swPins[2])) ? SWITCH_ON : SWITCH_OFF);

  if (update > 0) {
    updateLights();
  }
}
void serialEvent() {
  struct Command command;
  int len = Serial.readBytes(((char *) &command), 4);

  if (len < 4 || command.head != HEAD) return;
  if (command.light > 2) return;

  switch (command.cmd) {
  case CMD_PING:
    Serial.write(CMD_PONG);
    break;
  case CMD_ALL_ON:
    for (int x = 0; x < 3; x++) {
      settings.lights[x] = LIGHT_ON;
    }
    break;
  case CMD_ALL_OFF:
    for (int x = 0; x < 3; x++) {
      settings.lights[x] = LIGHT_OFF;
    }
    break;
  case CMD_ALL_PWM:
    for (int x = 0; x < 3; x++) {
      settings.pwm[x] = command.pwm;
    }
    break;
  case CMD_ALL_CLEAR:
    clearAll();
    break;
  case CMD_ON:
    settings.lights[command.light] = LIGHT_ON;
    break;
  case CMD_OFF:
    settings.lights[command.light] = LIGHT_OFF;
    break;
  case CMD_PWM:
    settings.pwm[command.light] = command.pwm;
    break;
  case CMD_SETTINGS_READ:
    Serial.write(((byte *) &settings), sizeof(Settings));
    Serial.flush();
    break;
  case CMD_SETTINGS_WRITE:
    struct Settings newSettings;
    len = Serial.readBytes(((char *) &newSettings), sizeof(Settings));

    if (len < sizeof(Settings)) return;

    settings = newSettings;
    break;
  default:
    return;
  }

  updateLights();
}



