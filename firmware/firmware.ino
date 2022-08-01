/*
 * Language: C, C++
 * IDE: Teensyduino 1.55 (Arduino 1.8.16)
 * Board: Teensy 3.2
 * USB Type: MIDI
 * External Libraries: None
*/

#include "MMap.h"
#include "Interface.h"
#include "Mighty_Zap.h"
#include "Electromagnet.h"
#include "Robot_Communication.h"
#include "LEDs.h"
#include "Striker.h"

/* these pins are used  to give reference voltage to the level shifter*/
#define LEVEL_SHIFTER_LV_PIN          3
#define LEVEL_SHIFTER_GND_PIN         2

#define ZAP_HALF_DUPLEX_DIRECTION_PIN 5
#define ZAP_ID                        1

#define ELECTROMAGNET_VIN_PIN         14
#define ELECTROMAGNET_STANDBY_PIN     12
#define ELECTROMAGNET_B_IN_1_PIN      11
#define ELECTROMAGNET_B_IN_2_PIN      10
#define ELECTROMAGNET_B_PWM_PIN        9

#define ORANGE_BUILTIN_LED_PIN        13
#define GREEN_LED_PIN                  8
#define YELLOW_LED_PIN                 7
#define RED_LED_PIN                    6

/*------------------------------------------------------*/
void setup()
{
  //Serial.begin(115200);

  pinMode(LEVEL_SHIFTER_LV_PIN, OUTPUT);
  pinMode(LEVEL_SHIFTER_GND_PIN, OUTPUT);
  
  digitalWrite(LEVEL_SHIFTER_LV_PIN, HIGH);
  digitalWrite(LEVEL_SHIFTER_GND_PIN, LOW);

  mmap_init();
  
  int zap_serial_baud   = mmap_read(MMAP_ZAP_SERIAL_BAUD, 4);
  
  zap_init(&Serial1, ZAP_HALF_DUPLEX_DIRECTION_PIN, 1, zap_serial_baud);
  led_init(ORANGE_BUILTIN_LED_PIN, RED_LED_PIN, YELLOW_LED_PIN, GREEN_LED_PIN);
  electromagnet_init(ELECTROMAGNET_VIN_PIN, ELECTROMAGNET_STANDBY_PIN, ELECTROMAGNET_B_IN_1_PIN, ELECTROMAGNET_B_IN_2_PIN, ELECTROMAGNET_B_PWM_PIN);
  interface_init();
  
  robot_send_message("zap baud %f", (float)zap_serial_baud);
  led_set (LED_GREEN, LED_ON);

  while(!striker_init(ZAP_ID))
    {
      delay(1000);
      interface_run_loop();
      /* in case baud needs to be configured*/
    }
}


/*------------------------------------------------------*/
void loop()
{
  interface_run_loop();
}
