#include "Electromagnet.h"
#include <Arduino.h>

#define ELECTROMAGNET_PWM_RESOLUTION 8
#define ELECTROMAGNET_PWM_MAX ((1 << (ELECTROMAGNET_PWM_RESOLUTION)) - 1)

int electromagnet_vin_pin;
int electromagnet_standby_pin;
int electromagnet_in_1_pin;
int electromagnet_in_2_pin;
int electromagnet_pwm_pin;

/*----------------------------------------------------------------*/
void electromagnet_init(int vin_pin, int standby_pin, int in_1_pin, int in_2_pin, int pwm_pin)
{
  electromagnet_vin_pin     = vin_pin;
  electromagnet_standby_pin = standby_pin;
  electromagnet_in_1_pin    = in_1_pin;
  electromagnet_in_2_pin    = in_2_pin;
  electromagnet_pwm_pin     = pwm_pin; 

  analogWriteResolution(ELECTROMAGNET_PWM_RESOLUTION);
  
  pinMode(electromagnet_vin_pin    , OUTPUT);
  pinMode(electromagnet_standby_pin, OUTPUT);
  pinMode(electromagnet_in_1_pin   , OUTPUT);
  pinMode(electromagnet_in_2_pin   , OUTPUT);
  //pinMode(electromagnet_pwm_pin  , OUTPUT);

  digitalWrite(electromagnet_vin_pin    , HIGH);
  digitalWrite(electromagnet_standby_pin, LOW);
  digitalWrite(electromagnet_in_1_pin   , LOW);
  digitalWrite(electromagnet_in_2_pin   , LOW);
  analogWrite (electromagnet_pwm_pin    , 0);
}

/*----------------------------------------------------------------*/
void electromagnet_on (float strength)
{
  int is_forward = strength >= 0;
  strength = (strength < -1) ? -1 : (strength > 1) ? 1 : strength;
  strength = fabs(strength);

  int pwm_strength = round(strength * ELECTROMAGNET_PWM_MAX);

  int in1 = (is_forward) ? HIGH : LOW;
  int in2 = (is_forward) ? LOW  : HIGH;
  
  digitalWrite(electromagnet_standby_pin, HIGH);
  digitalWrite(electromagnet_in_1_pin   , in1);
  digitalWrite(electromagnet_in_2_pin   , in2);
  analogWrite (electromagnet_pwm_pin    , pwm_strength);
}

/*----------------------------------------------------------------*/
void electromagnet_off()
{
  digitalWrite(electromagnet_standby_pin, LOW);
  digitalWrite(electromagnet_in_1_pin   , LOW);
  digitalWrite(electromagnet_in_2_pin   , LOW);
  analogWrite (electromagnet_pwm_pin    , 0);
}
