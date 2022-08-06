#include "Striker.h"
#include "Electromagnet.h"
#include "LEDs.h"
#include "Mighty_Zap.h"
#include "Robot_Communication.h"

#include <Arduino.h>

#define  STRIKER_TIMER_INTERVAL             50 // ms
#define  STRIKER_TIMEOUT_TIME             10000 // ms
#define  STRIKER_WAITING_SILENCE_TIME     1000 // ms
#define  STRIKER_RELEASE_TIME              100 // ms

typedef enum striker_state_enum
{
  STRIKER_IDLE               = 0,
  STRIKER_EXTENDING             ,
  STRIKER_RETRACTING            , 
  STRIKER_EXTENDING_FORCE_MODE  ,
  STRIKER_RETRACTING_FORCE_MODE ,
  STRIKER_WAITING_SILENCE       ,
  STRIKER_RELEASING             ,
  STRIKER_EXTENDING_5_SECS      ,
  STRIKER_WAITING_5_SECS        , 
}striker_state_t;

striker_state_t striker_state            = STRIKER_IDLE;
int             striker_zap_max_position = 4000; //
int             striker_zap_id           = 0; // any will respond to id 0;
float           pending_retract_position = 0;
float           pending_retract_force    = 0;
float           striker_max_force        = 0;
float           striker_filtered_force   = 0;
int             striker_clear_next_force = 0;
int             striker_state_timer      = 0;
int             striker_stream_force     = 0;
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
float striker_scalef(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*----------------------------------------------------------------*/
float striker_scale_force_in(float force_percent)
{
  return striker_scalef(force_percent, 0, 100, 10, 50);
}

/*----------------------------------------------------------------*/
float striker_scale_force_out(float force_percent)
{
  return striker_scalef(force_percent, 10, 50, 0, 100);
}

/*----------------------------------------------------------------*/
float striker_force_mode_update_filtered_force()
{
  float force;
  zap_error_t error;
  
  error = zap_get(striker_zap_id, ZAP_REGISTER_PRESENT_FORCE, &force, 5);
  if(error != ZAP_NO_ERROR)
    striker_kill(5);
   else
     {
       if(striker_clear_next_force) 
         striker_filtered_force = force;
       else
         striker_filtered_force = striker_filtered_force * 0.7 + force * 0.3;
     }
     
  striker_clear_next_force = 0;
}

/*----------------------------------------------------------------*/
void striker_run_loop()
{
  float result;
  zap_error_t error;

  if(striker_state == STRIKER_EXTENDING_5_SECS)
  {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        return striker_kill(1);
      
      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        return striker_kill(1); 

      if(result == 0)
        {
          electromagnet_on(-1.0);
          led_set (LED_RED, LED_ON);
          striker_state_timer = 0;
          striker_state = STRIKER_WAITING_5_SECS;
        }
  }

  if(striker_state == STRIKER_WAITING_5_SECS)
  {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        return striker_kill(0);
  }
  
  else if((striker_state == STRIKER_EXTENDING) || ((striker_state == STRIKER_EXTENDING_FORCE_MODE)))
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        return striker_kill(1);
      
      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        return striker_kill(1);
        
      if(result == 0)
        {
          electromagnet_on(1.0);
          led_set (LED_YELLOW, LED_ON);
          zap_set (striker_zap_id, ZAP_REGISTER_GOAL_POSITION, pending_retract_position, 0);
          striker_state_timer = 0;
          striker_state = (striker_state==STRIKER_EXTENDING) ? STRIKER_RETRACTING : STRIKER_RETRACTING_FORCE_MODE;
          striker_max_force = 0;
        }
    }

  else if(striker_state == STRIKER_RETRACTING)
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        return striker_kill(1);
      
      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        return striker_kill(1);
 
      if(result == 0)
        {
          striker_state_timer = 0;
          striker_state = STRIKER_WAITING_SILENCE;
        }
    }

  else if(striker_state == STRIKER_RETRACTING_FORCE_MODE)
    {
      striker_state_timer += STRIKER_TIMER_INTERVAL;
      if(striker_state_timer >= STRIKER_TIMEOUT_TIME)
        return striker_kill(1);

      if(striker_state_timer < 200) //millisecs. wait for motor currrent to stabalize
        {striker_clear_next_force = 1; return;}

       striker_force_mode_update_filtered_force();
       if(striker_stream_force)
         robot_send_message(robot_reply_streaming_force, striker_scale_force_out(striker_filtered_force));

      if(striker_filtered_force > striker_max_force)
        striker_max_force = striker_filtered_force;

      error = zap_get(striker_zap_id, ZAP_REGISTER_IS_MOVING, &result, 5);
      if(error != ZAP_NO_ERROR)
        return striker_kill(1);

      if(result == 0)
        return striker_kill(1);
    
      if(striker_filtered_force < (0.7 * striker_max_force))
        return striker_kill(1);
        
      if(striker_filtered_force >= pending_retract_force)
         {
           error = zap_get(striker_zap_id, ZAP_REGISTER_PRESENT_POSITION, &result, 5);
           if(error != ZAP_NO_ERROR)
             return striker_kill(1);
           error = zap_set(striker_zap_id, ZAP_REGISTER_GOAL_POSITION, result, 0);
           if(error != ZAP_NO_ERROR)
             return striker_kill(1);
           else
             {
               striker_state_timer = 0;
               striker_state = STRIKER_WAITING_SILENCE;  
             }
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
        striker_kill(0);
    }
}

/*----------------------------------------------------------------*/
void striker_generic_start_action()
{
  striker_state_timer = 0;
  led_set (LED_YELLOW, LED_OFF);
  led_set (LED_RED, LED_OFF);
  zap_set (striker_zap_id, ZAP_REGISTER_GOAL_SPEED   , 1023, 0);
  zap_set (striker_zap_id, ZAP_REGISTER_GOAL_POSITION, striker_zap_max_position, 0);
}

/*----------------------------------------------------------------*/
void striker_lower_and_wait_with_magnet_off_5_seconds()
{
  if(striker_is_busy())
    return;

  striker_generic_start_action();
  striker_state = STRIKER_EXTENDING_5_SECS; 
}

/*----------------------------------------------------------------*/
void striker_strike(float height /* 0 ~ 1*/)
{ 
  if(striker_is_busy())
    return;

  height = (height > 1) ? 1 : (height < 0) ? 0 : height;
  height *= striker_zap_max_position;
  height = striker_zap_max_position - height;
  pending_retract_position = height;

  striker_generic_start_action();
  striker_state = STRIKER_EXTENDING;
}


/*----------------------------------------------------------------*/
void striker_strike_force_mode(float force /*0 ~ 1*/)
{
  if(striker_is_busy())
    return;

  force = (force > 1) ? 1 : (force < 0) ? 0 : force;
  force *= 100; //percent of motor maximum
  pending_retract_force = striker_scale_force_in(force);
  pending_retract_position = 0;

  striker_generic_start_action();
  striker_state = STRIKER_EXTENDING_FORCE_MODE;
}

/*----------------------------------------------------------------*/
void striker_set_should_stream_motor_force(int should_stream)
{
  striker_stream_force = should_stream;
}

/*----------------------------------------------------------------*/
int striker_is_busy()
{
  return striker_state != STRIKER_IDLE;
}

/*----------------------------------------------------------------*/
void striker_kill(int error)
{
  striker_state = STRIKER_IDLE;
  electromagnet_off();
  zap_set(striker_zap_id, ZAP_REGISTER_FORCE_ENABLE, 0, 0);

  //led_set (LED_RED   , (error & 0x01) ? LED_ON : LED_OFF);
  //led_set (LED_YELLOW, (error & 0x02) ? LED_ON : LED_OFF);
  //led_set (LED_GREEN , (error & 0x04) ? LED_ON : LED_OFF);
  
  if(error)
    {
      
      led_set (LED_YELLOW, LED_ON);
      led_set (LED_RED   , LED_ON);
    }
  else
    {
      led_set (LED_YELLOW, LED_OFF);
      led_set (LED_RED   , LED_OFF);
    }
}
