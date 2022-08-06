#ifndef ROBOT_COMMUNICATION
#define ROBOT_COMMUNICATION 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include <stdint.h>
#include "MIDI_Parser.h"

/*---------------------------------------------------*/
typedef struct robot_arg_struct
{
  union robot_arg_value_union
    {
      float   f;
      int32_t i;
      char*   s;
    }value;

  char type;
}robot_arg_t;

/*--------------------------------------------------------*/
typedef enum robot_message_hash_enum
{
  robot_hash_zap_get                     = 77239656,
  robot_hash_zap_set                     = 77226364,
  robot_hash_zap_prepare                 = 3483770079,
  robot_hash_zap_do_prepared             = 117587759,
  robot_hash_zap_ping                    = 2548708014,
  robot_hash_zap_factory_reset           = 3252245920,
  robot_hash_zap_reboot                  = 768952351,
  robot_hash_zap_scan_ids                = 3614840288,
  robot_hash_zap_scan_id_baud            = 751962430,
  robot_hash_reply_zap_register          = 3423193731,
  robot_hash_reply_zap_id                = 1925841246,

  robot_hash_electromagnet_on            = 3756831744,
  robot_hash_electromagnet_off           = 3716363630,
  robot_hash_should_stream_force         = 348492820,
  
  robot_hash_note_on                     = 2186442148,
  robot_hash_note_off                    = 3433114442,
  robot_hash_all_notes_off               = 412662823,
  robot_hash_get_firmware_version        = 1616850098,
  robot_hash_reply_firmware_version      = 857564406,
  robot_hash_aok                         = 2085472399,
  robot_hash_error                       = 3342388946,
  robot_hash_reply_streaming_force       = 5858988,
}robot_message_hash_t;



/*--------------------------------------------------------*/
#define robot_cmd_zap_get                     "/zap_get %i %i"           //id, reg
#define robot_cmd_zap_set                     "/zap_set %i %i %f"        //id, reg, val
#define robot_cmd_zap_prepare                 "/zap_prepare %i %i %f"    //id, reg, val
#define robot_cmd_zap_do_prepared             "/zap_do_prepared %i"      //id
#define robot_cmd_zap_ping                    "/zap_ping %i"             //id
#define robot_cmd_zap_factory_reset           "/zap_factory_reset %i"    //id
#define robot_cmd_zap_reboot                  "/zap_reboot %i"           //id
#define robot_cmd_zap_scan_ids                "/zap_scan_ids"            //
#define robot_cmd_zap_scan_id_baud            "/zap_scan_id_baud"        //

#define robot_cmd_electromagnet_on            "/magnet_on %f"            //strength -1 ~ 1
#define robot_cmd_electromagnet_off           "/magnet_off"              //

#define robot_cmd_should_stream_force         "/stream_force %i"         //0 or 1 (false or true)

#define robot_cmd_get_firmware_version        "/get_firmware"            //
#define robot_cmd_note_on                     "/note_on %i %i"           //
#define robot_cmd_note_off                    "/note_off %i %i"          //
#define robot_cmd_all_notes_off               "/all_notes_off"           //

#define robot_reply_aok                       "/aok"
#define robot_reply_error                     "/error %s"
#define robot_reply_firmware_version          "/reply_firmware %i %i"
#define robot_reply_zap_register              "/reply_zap_reg %i %f"
#define robot_reply_zap_id                    "/reply_zap_id %i %i"    //id, baud

#define robot_reply_streaming_force           "/f %f"    //percent of motor force

/*---------------------------------------------------*/
typedef void  (*robot_message_received_callback)(void* self, char* message, robot_arg_t args[], int num_args);

/*--------------------------------------------------------*/

#if (defined __APPLE__) || (defined __linux__)
#define __ROBOT_MIDI_HOST__ 1
#endif

#if defined  __APPLE__
#define ROBOT_MIDI_DEVICE_NAME "Soundiron Bot"

#elif defined  __linux__
//cat /proc/asound/cards for a list of cards -- I guess Linux dosen't like the space in the name?
#define ROBOT_MIDI_DEVICE_NAME "hw:Bot"
#endif

#if defined __ROBOT_MIDI_HOST__
typedef   struct opaque_robot_struct Robot;
Robot*    robot_new                (robot_message_received_callback callback, void* callback_self);
Robot*    robot_destroy            (Robot* self);
void      robot_send_message       (Robot* self, const char *message, /*args*/...);
void      robot_send_raw_midi      (Robot* self, uint8_t* midi_bytes, int num_bytes);

#else //!__ROBOT_MIDI_HOST__,
void     robot_send_message       (const char *message, /*args*/...);

#endif // __ROBOT_MIDI_HOST__

//SHARED CODE
void     robot_init               (robot_message_received_callback callback, void* callback_self);
float    robot_arg_to_float       (robot_arg_t* arg);
int32_t  robot_arg_to_int         (robot_arg_t* arg);

uint32_t robot_hash_message       (char* message);
void     robot_debug_print_message(char* message, robot_arg_t args[], int num_args);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif //ROBOT_COMMUNICATION
