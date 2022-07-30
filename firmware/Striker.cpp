#include "Striker.h"
#include "Electromagnet.h"
#include "LEDs.h"
#include "Mighty_Zap.h"

#include <Arduino.h>

#define  STRIKER_TIMER_INTERVAL             50 // ms
#define  STRIKER_TIMEOUT_TIME             3000 // ms
#define  STRIKER_WAITING_SILENCE_TIME     1000 // ms
#define  STRIKER_RELEASE_TIME              100 // ms

typedef enum striker_state_enum
{
  STRIKER_IDLE           = 0,
  STRIKER_EXTENDING         ,
  STRIKER_RETRACTING        ,
  STRIKER_WAITING_SILENCE   , 
  STRIKER_RELEASING         , 
}striker_state_t;

striker_state_t striker_state            = STRIKER_IDLE;
int             striker_zap_max_position = 4000; //
int             striker_zap_id           = 0; // any will respond to id 0;
float           pending_retract_position = 0;
int             striker_state_timer      = 0;
IntervalTimer   striker_timer;

void striker_run_loop();

/*----------------------------------------------------------------*/
int striker_init  (int zap_id /*ignore this*/)
{
  float result;
  
  striker_zap_id = zap_id;
  
  zap_error_t error = zap_get(striker_zap_id, ZAP_REGISTER_CALIBRATED_MAX_POSITION, &result, 20);
  if(error != ZAP_NO_ERROR)
    {
      led_set (LED_RED, LED_ON);
      return 0;
    }

  led_set (LED_RED, LED_OFF);
  striker_zap_max_position = (int)result;
  striker_timer.begin(striker_run_loop, STRIKER_TIMER_INTERVAL * 1000 /*microsecs*/);

  return 1;
}


/*----------------------------------------------------------------*/
void striker_run_loop()
{
  float result;
  zap_error_t error;
  
  if(striker_state == STRIKER_EXTENDING)
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        striker_kill();
      
      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        striker_kill();
        
      if(result == 0)
        {
          electromagnet_on(1.0);
          led_set (LED_YELLOW, LED_ON);
          zap_set (striker_zap_id, ZAP_REGISTER_GOAL_POSITION, pending_retract_position, 0);
          striker_state_timer = 0;
          striker_state = STRIKER_RETRACTING;
        }
    }

  else if(striker_state == STRIKER_RETRACTING)
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        striker_kill();
      
      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        striker_kill();
        
      if(result == 0)
        {
          striker_state_timer = 0;
          striker_state = STRIKER_WAITING_SILENCE;
        }
    }

  else if(striker_state == STRIKER_WAITING_SILENCE)
    {
       striker_state_timer += STRIKER_TIMER_INTERVAL;
       if(striker_state_timer >= STRIKER_WAITING_SILENCE_TIME)
         {
           electromagnet_on(-1.0); 
           led_set (LED_YELLOW, LED_OFF);
           led_set (LED_RED, LED_ON);
           striker_state_timer = 0;
           striker_state = STRIKER_RELEASING;
         }
    }

   else if(striker_state == STRIKER_RELEASING)
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_RELEASE_TIME)
        striker_kill();
    }
}

void striker_strike(float height /* 0 ~ 1*/)
{ 
  if(striker_is_busy())
    return;

  height = (height > 1) ? 1 : (height < 0) ? 0 : height;
  height *= striker_zap_max_position;
  height = striker_zap_max_position - height;
  pending_retract_position = height;
  zap_set (striker_zap_id, ZAP_REGISTER_GOAL_SPEED   , 1023, 0);
  zap_set (striker_zap_id, ZAP_REGISTER_GOAL_POSITION, striker_zap_max_position, 0);
  striker_state = STRIKER_EXTENDING;
  
  /*
   *   height = (height > 1) ? 1 : (height < 0) ? 0 : height;
  height *= striker_zap_max_position;
  //zap_set (striker_zap_id, ZAP_REGISTER_GOAL_POSITION, height, 0);
  zap_error_t zap_get              (striker_zap_id, ZAP_REGISTER_IS_MOVING, float* result, int timeout_ms);
   */
}


/*----------------------------------------------------------------*/
void striker_strike_force_mode(float force)
{
  
}


/*----------------------------------------------------------------*/
int striker_is_busy()
{
  return striker_state != STRIKER_IDLE;
}

/*----------------------------------------------------------------*/
void striker_kill()
{
  striker_state = STRIKER_IDLE;
  striker_state_timer = 0;
  electromagnet_off();
  led_set (LED_YELLOW, LED_OFF);
  led_set (LED_RED, LED_OFF);
  zap_set(striker_zap_id, ZAP_REGISTER_FORCE_ENABLE, 0, 0);
}
