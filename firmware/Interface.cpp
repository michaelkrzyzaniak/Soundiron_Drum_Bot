#include <Arduino.h>
#include "Interface.h"
#include "Robot_Communication.h"
#include "Mighty_Zap.h"
#include "Electromagnet.h"
#include "Striker.h"
#include "MMAP.h"

#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 1

#define INTERFACE_SERVO_TIMEOUT 5 //ms

void interface_dispatch             (void* self, char* message, robot_arg_t args[], int num_args);
void interface_note_on_callback     (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_note_off_callback    (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_mode_change_callback (midi_channel_t chan, midi_mode_t  mode , uint8_t arg        );

/*---------------------------------------------------*/
void interface_init()
{
  midi_note_on_event_handler  = interface_note_on_callback;
  midi_note_off_event_handler = interface_note_off_callback;
  midi_mode_change_event_handler = interface_mode_change_callback;
  robot_init(interface_dispatch, NULL);
}

/*---------------------------------------------------*/
void interface_run_loop()
{
  uint32_t ui;
  while((ui = usb_midi_read_message()) != 0)
    {
      midi_parse((ui >>  8) & 0xFF);
      midi_parse((ui >> 16) & 0xFF);
      midi_parse((ui >> 24) & 0xFF);
    }
}

/*---------------------------------------------------*/
void interface_note_on_callback(midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel)
{
  float value = vel / 127.0;

  switch(pitch)
    {
      case MIDI_PITCH_C_4:
        striker_strike(value);
        break;
        
      case MIDI_PITCH_C_5:
        striker_strike_force_mode(value);
        break;

      case MIDI_PITCH_C_6:
        striker_lower_and_wait_with_magnet_off_5_seconds();
        
      default:
        break;
    }
}

/*---------------------------------------------------*/
void interface_note_off_callback(midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel)
{

}

/*-----------------------------------------------------*/
void interface_mode_change_callback (midi_channel_t chan, midi_mode_t  mode , uint8_t arg)
{
  if(mode == MIDI_MODE_ALL_SOUND_OFF)
    striker_kill(0);
}

/*-----------------------------------------------------*/
void interface_send_aok()
{ 
  robot_send_message(robot_reply_aok); 
}

/*-----------------------------------------------------*/
void interface_send_error(const char* error)
{ 
  robot_send_message(robot_reply_error, error); 
}

/*-----------------------------------------------------*/
void interface_send_zap_error(zap_error_t error)
{ 
  if(error == ZAP_NO_ERROR)
    interface_send_aok();
  else
    interface_send_error(zap_errors_to_string(error));
}

/*-----------------------------------------------------*/
void interface_dispatch(void* self, char* message, robot_arg_t args[], int num_args)
{
  switch(robot_hash_message(message)) 
    { 
      /******************************/
      case robot_hash_zap_get:
        if(num_args == 2)
          {
            float result;
            int id  = robot_arg_to_int  (&args[0]);
            int reg = robot_arg_to_int  (&args[1]);
            zap_error_t error = zap_get(id,(zap_register_t)reg, &result, INTERFACE_SERVO_TIMEOUT);
            if(error == ZAP_NO_ERROR)
              robot_send_message(robot_reply_zap_register, reg, result);
            else
              interface_send_zap_error(error);
          }
        break;
        
      /******************************/ 
      case robot_hash_zap_set:
        if(num_args == 3)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            int   reg = robot_arg_to_int  (&args[1]);
            float val = robot_arg_to_float(&args[2]);
            zap_error_t error = zap_set(id, (zap_register_t)reg, val, 0 /*INTERFACE_SERVO_TIMEOUT*/);
            interface_send_zap_error(error);
            
            if(reg == ZAP_REGISTER_BAUD_RATE)
              {
                //zap_reboot(id, INTERFACE_SERVO_TIMEOUT);
                float baud_rate = zap_raw_to_baud_bps(zap_baud_bps_to_raw(val));
                robot_send_message("baud rate %f", baud_rate);
                mmap_write(MMAP_ZAP_SERIAL_BAUD, (int)baud_rate, 4);
                zap_set_interface_baud_rate(baud_rate);
                robot_send_message("baud rate %i", mmap_read(MMAP_ZAP_SERIAL_BAUD, 4));
              }
          }
        break;
        
      /******************************/
      case robot_hash_zap_prepare:
        if(num_args == 3)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            int   reg = robot_arg_to_int  (&args[1]);
            float val = robot_arg_to_float(&args[2]);
            zap_error_t error = zap_prepare(id, (zap_register_t)reg, val, INTERFACE_SERVO_TIMEOUT);
            interface_send_zap_error(error);
          }
        break;
        
      /******************************/
      case robot_hash_zap_do_prepared:
        if(num_args == 1)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            zap_error_t error = zap_do_prepared(id, INTERFACE_SERVO_TIMEOUT);
            interface_send_zap_error(error);
          }
        break;
        
      /******************************/ 
      case robot_hash_zap_ping:
        if(num_args == 1)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            unsigned long later, now;
            now = micros();
            zap_error_t error = zap_ping(id, INTERFACE_SERVO_TIMEOUT);
            later = micros();
            robot_send_message("/microseconds %i", (int)(later-now));
            interface_send_zap_error(error);
          }
        break;

      /******************************/
      case robot_hash_zap_factory_reset:
        if(num_args == 1)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            zap_error_t error = zap_factory_reset(id, INTERFACE_SERVO_TIMEOUT);
            interface_send_zap_error(error);
            zap_set_interface_baud_rate(ZAP_DEFAULT_BAUD);
            mmap_write(MMAP_ZAP_SERIAL_BAUD, ZAP_DEFAULT_BAUD, 4);
          }
        break;


      /******************************/
      case robot_hash_zap_reboot:
        if(num_args == 1)
          {
            int   id  = robot_arg_to_int  (&args[0]);
            zap_error_t error = zap_reboot(id, INTERFACE_SERVO_TIMEOUT);
            interface_send_zap_error(error);
          }
        break;
        
      /******************************/
      case robot_hash_zap_scan_ids:
        {
          int id;
          zap_error_t error;
          for(id=0; id<=253; id++)
            {
              error = zap_ping(id, INTERFACE_SERVO_TIMEOUT);
              if(error == ZAP_NO_ERROR)
                {
                  float baud_rate = 0;
                  error = zap_get(id, ZAP_REGISTER_BAUD_RATE, &baud_rate, INTERFACE_SERVO_TIMEOUT);
                  robot_send_message(robot_reply_zap_id, id, (int)baud_rate);
                }
            }
          interface_send_aok();
        }
        break;

      /******************************/
      case robot_hash_zap_scan_id_baud:
        {
          /*
          robot_send_message("this could take up to 10 minutes ...");
          int id;
          zap_error_t error;
          for(id=0; id<=253; id++)
            {
              int raw_baud;
              for(raw_baud=0; raw_baud<=252; raw_baud++)
                {
                  float baud_rate = servo_raw_to_baud_bps(raw_baud);
                  servo_set_interface_baud_rate((int)baud_rate);
                  error = servo_ping(id, INTERFACE_SERVO_TIMEOUT);
                  if(error == SERVO_NO_ERROR)
                    {
                      robot_send_message(robot_reply_servo_id, id, (int)baud_rate);
                      mmap_write(MMAP_SERVO_SERIAL_BAUD, (int)baud_rate, 4);
                      robot_send_message(robot_reply_servo_register, SERVO_REGISTER_ID, (float)id);
                      robot_send_message(robot_reply_servo_register, SERVO_REGISTER_BAUD_RATE, baud_rate);
                      goto scan_out;
                    }
                }              
            }
          scan_out:
          interface_send_aok();
          */
        }
        break;

      /******************************/
      case robot_hash_electromagnet_on:
        if(num_args == 1)
          {
            float strength  = robot_arg_to_float(&args[0]);
            electromagnet_on(strength);
            robot_send_message(robot_reply_aok);
          }
        break;
      
      /******************************/
      case robot_hash_electromagnet_off:
        if(num_args == 0)
          {
            electromagnet_off();
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_should_stream_force:
        if(num_args == 1)
          {
            int should_stream = robot_arg_to_int(&args[0]);
            striker_set_should_stream_motor_force(should_stream);
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_note_on:
        if(num_args == 2)
          {
            int note  = robot_arg_to_int(&args[0]);
            int vel   = robot_arg_to_int(&args[1]);
            interface_note_on_callback(MIDI_CHANNEL_1, note, vel);
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_note_off:
        if(num_args == 1)
          {
            int note = robot_arg_to_int(&args[0]);
            interface_note_off_callback(MIDI_CHANNEL_1, note, 0);
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_all_notes_off:
        if(num_args == 0)
          {
            interface_mode_change_callback (MIDI_CHANNEL_1, MIDI_MODE_ALL_SOUND_OFF, 0);
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_get_firmware_version:
        if(num_args == 0)
           robot_send_message(robot_reply_firmware_version, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION); 
        break;
      
      /******************************/
      default: break;
    }
}
