#ifndef LED
#define LED 1

typedef enum led_color_enum
{
  LED_ORANGE_BUILTIN = 0,
  LED_RED,
  LED_YELLOW,
  LED_GREEN,
  LED_NUM_LEDS,
}led_color_t;

typedef enum led_state_enum
{
  LED_OFF = 0,
  LED_ON,
}led_state_t;

void led_init(int orange_pin, int red_pin, int yellow_pin, int green_pin);
void led_set (led_color_t led, led_state_t state);

#endif //LED
