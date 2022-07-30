#include "LEDs.h"
#include <Arduino.h>

#define ELECTROMAGNET_PWM_RESOLUTION 8
#define ELECTROMAGNET_PWM_MAX ((1 << (ELECTROMAGNET_PWM_RESOLUTION)) - 1)

int led_pins[LED_NUM_LEDS];

/*----------------------------------------------------------------*/
void led_init(int orange_pin, int red_pin, int yellow_pin, int green_pin)
{
  led_pins[LED_ORANGE_BUILTIN] = orange_pin;
  led_pins[LED_RED]            = red_pin;
  led_pins[LED_YELLOW]         = yellow_pin;
  led_pins[LED_GREEN]          = green_pin;

  int i;
  for(i=0; i<LED_NUM_LEDS; i++)
    {
      pinMode(led_pins[i], OUTPUT);
      led_set ((led_color_t)i, LED_OFF);
    }
}

/*----------------------------------------------------------------*/
void led_set (led_color_t led, led_state_t state)
{
  if((led < 0) || (led >= LED_NUM_LEDS))
    return;

  int val = (state == LED_OFF) ? LOW : HIGH;
  digitalWrite(led_pins[led], val);
}
