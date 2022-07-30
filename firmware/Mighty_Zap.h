/*!
 */

#ifndef MIGHTY_ZAP
#define MIGHTY_ZAP

#include <HardwareSerial.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*--------------------------------------------------------------------------------------------*/
#define ZAP_BROADCAST_ID               0xFE
#define ZAP_STANDALONE_ID              0x00 //servo will respond to any ID
#define ZAP_DEFAULT_ID                 ZAP_STANDALONE_ID

//the only supported baud rates, must reboot after changing
#define ZAP_BAUD_115200                115200
#define ZAP_BAUD_57600                 57600
#define ZAP_BAUD_38600                 38600
#define ZAP_BAUD_19200                 19200
#define ZAP_BAUD_9600                  9600
#define ZAP_DEFAULT_BAUD               ZAP_BAUD_57600

//the only supported resolutions
#define ZAP_RESOLUTION_4096            4096
#define ZAP_RESOLUTION_2048            2048
#define ZAP_RESOLUTION_1024            1024
#define ZAP_RESOLUTION_256             256

/*--------------------------------------------------------------------------------------------*/
typedef enum zap_error_bit_mask_enum
{
  /* defined by datasheet */
  ZAP_ERROR_VOLTAGE                  = 0x0001,
  ZAP_ERROR_POSITION                 = 0x0002,
  ZAP_ERROR_RANGE                    = 0x0004,
  ZAP_ERROR_CHECKSUM                 = 0x0008,
  ZAP_ERROR_OVERLOAD                 = 0x0010,
  ZAP_ERROR_INSTRUCTION              = 0x0020,

  /* defined by this module */
  ZAP_ERROR_TIMEOUT                  = 0x0040,
  ZAP_ERROR_INVALID_RESPONSE         = 0x0080,
  
  /* raw value, not bit mask */
  ZAP_NO_ERROR                       = 0x0000,
}zap_error_t;

/*--------------------------------------------------------------------------------------------*/
typedef enum zap_register_enum
{
  /* EEPROM */
  ZAP_REGISTER_MODEL_NUMBER            = 0x00,
  //ZAP_REGISTER_MODEL_NUMBER_L        = 0x00,
  //ZAP_REGISTER_MODEL_NUMBER_H        = 0x01,
  ZAP_REGISTER_FIRMWARE_VERSION        = 0x02,
  ZAP_REGISTER_ID                      = 0x03,
  ZAP_REGISTER_BAUD_RATE               = 0x04,
  ZAP_REGISTER_RETURN_DELAY_TIME       = 0x05,
  ZAP_REGISTER_MIN_POSITION            = 0x06,
  //ZAP_REGISTER_MIN_POSITION_L        = 0x06,
  //ZAP_REGISTER_MIN_POSITION_H        = 0x07,
  ZAP_REGISTER_MAX_POSITION            = 0x08,
  //ZAP_REGISTER_MAX_POSITION_L        = 0x08,
  //ZAP_REGISTER_MAX_POSITION_H        = 0x09,
  ZAP_REGISTER_PROTOCOL_TYPE           = 0x0A,
  ZAP_REGISTER_MAX_VOLTAGE             = 0x0D,
  ZAP_REGISTER_MAX_FORCE               = 0x0E,
  //ZAP_REGISTER_MAX_TORQUE_L          = 0x0E,
  //ZAP_REGISTER_MAX_TORQUE_H          = 0x0F,
  ZAP_REGISTER_RETURN_LEVEL            = 0x10,
  ZAP_REGISTER_LED_ERROR               = 0x11,
  ZAP_REGISTER_SHUTDOWN_ERROR          = 0x12,
  //ZAP_REGISTER_TEMPERATURE             = 0x13,
  ZAP_REGISTER_RESOLUTION_FACTOR       = 0x16,

  ZAP_REGISTER_CALIBRATED_MIN_POSITION        = 0x18,
  //ZAP_REGISTER_CALIBRATED_MIN_POSITION_L    = 0x18,
  //ZAP_REGISTER_CALIBRATED_MIN_POSITION_H    = 0x19,
  ZAP_REGISTER_CALIBRATED_MAX_POSITION        = 0x1A, 
  //ZAP_REGISTER_CALIBRATED_MAX_POSITION_L    = 0x1A,
  //ZAP_REGISTER_CALIBRATED_MAX_POSITION_H    = 0x1B,
  ZAP_REGISTER_CALIBRATED_CENTER_POSITION     = 0x1C,
  //ZAP_REGISTER_CALIBRATED_CENTER_POSITION_L = 0x1C,
  //ZAP_REGISTER_CALIBRATED_CENTER_POSITION_H = 0x1D,
  
  ZAP_REGISTER_ACCEL_RATE              = 0x21,
  ZAP_REGISTER_DECEL_RATE              = 0x22,
  ZAP_REGISTER_D_GAIN                  = 0x25,
  ZAP_REGISTER_I_GAIN                  = 0x26,
  ZAP_REGISTER_P_GAIN                  = 0x27,

  ZAP_REGISTER_MIN_POSITION_PW         = 0x28,
  //ZAP_REGISTER_MIN_POSITION_PW_L     = 0x28,
  //ZAP_REGISTER_MIN_POSITION_PW_H     = 0x29,
  ZAP_REGISTER_MAX_POSITION_PW         = 0x2A,
  //ZAP_REGISTER_MAX_POSITION_L        = 0x2A,
  //ZAP_REGISTER_MAX_POSITION_H        = 0x2B,
  ZAP_REGISTER_CENTER_POSITION_PW      = 0x2C,
  //ZAP_REGISTER_CENTER_POSITION_PW_L  = 0x2C,
  //ZAP_REGISTER_CENTER_POSITION_PW_H  = 0x2D,
  ZAP_REGISTER_CENTER_DIFFERNCE        = 0x32,
  //ZAP_REGISTER_CENTER_DIFFERNCE_L    = 0x32,
  //ZAP_REGISTER_CENTER_DIFFERNCE_H    = 0x33,
  ZAP_REGISTER_PUNCH_INITIAL_VALUE     = 0x34,
  //ZAP_REGISTER_PUNCH_INITIAL_VALUE_L = 0x34,
  //ZAP_REGISTER_PUNCH_INITIAL_VALUE_H = 0x35,

  /* RAM */
  ZAP_REGISTER_FORCE_ENABLE            = 0x80,
  ZAP_REGISTER_LED_IS_ON               = 0x81,
  
  ZAP_REGISTER_START_COMPLIANCE_MARGIN_RETRACTING   = 0x82,
  ZAP_REGISTER_START_COMPLIANCE_MARGIN_EXTENDING    = 0x83,

  ZAP_REGISTER_GOAL_POSITION           = 0x86,
  //ZAP_REGISTER_GOAL_POSITION_L       = 0x86,
  //ZAP_REGISTER_GOAL_POSITION_H       = 0x87,
  ZAP_REGISTER_GOAL_SPEED              = 0x88,
  //ZAP_REGISTER_GOAL_SPEED_L          = 0x88,
  //ZAP_REGISTER_GOAL_SPEED_H          = 0x89,
  ZAP_REGISTER_GOAL_CURRENT            = 0x8A,
  //ZAP_REGISTER_GOAL_CURRENT_L        = 0x8A
  //ZAP_REGISTER_GOAL_CURRENT_H        = 0x8B
  ZAP_REGISTER_PRESENT_POSITION        = 0x8C,
  //ZAP_REGISTER_PRESENT_POSITION_L    = 0x8C,
  //ZAP_REGISTER_PRESENT_POSITION_H    = 0x8D,
  ZAP_REGISTER_PRESENT_FORCE           = 0x8E,
  //ZAP_REGISTER_PRESENT_FORCE_L       = 0x8E,
  //ZAP_REGISTER_PRESENT_FORCE_H       = 0x8F,
  ZAP_REGISTER_PRESENT_SPEED           = 0x90,
  //ZAP_REGISTER_PRESENT_SPEED_L       = 0x90,
  //ZAP_REGISTER_PRESENT_SPEED_H       = 0x91,
  ZAP_REGISTER_PRESENT_VOLTAGE         = 0x92,
 
  
  ZAP_REGISTER_PENDING_INSTRUCTION     = 0x94,
  ZAP_REGISTER_IS_MOVING               = 0x96,
  ZAP_REGISTER_LOCK_EEPROM             = 0x97,
  ZAP_REGISTER_PUNCH                   = 0x98,
  //ZAP_REGISTER_PUNCH_L               = 0x98,
  //ZAP_REGISTER_PUNCH_H               = 0x99,
  ZAP_REGISTER_END_COMPLIANCE_MARGIN   = 0x9A,
}zap_register_t;

void        zap_init            (HardwareSerial* serial_port, int half_duplex_direction_pin, int transmit_high, uint32_t baud);
void        zap_set_interface_baud_rate(uint32_t baud);


zap_error_t zap_ping             (uint8_t id, int timeout_ms);
zap_error_t zap_get              (uint8_t id, zap_register_t reg, float* result, int timeout_ms);
zap_error_t zap_set              (uint8_t id, zap_register_t reg, float  val,    int timeout_ms);
zap_error_t zap_prepare          (uint8_t id, zap_register_t reg, float  val,    int timeout_ms);
zap_error_t zap_do_prepared      (uint8_t id, int timeout_ms);
zap_error_t zap_factory_reset    (uint8_t id, int timeout_ms);
zap_error_t zap_reboot           (uint8_t id, int timeout_ms);
zap_error_t zap_set_multiple     (uint8_t ids[], zap_register_t start_reg, float values[], int num_ids, int num_values_per_zap);
char*       zap_errors_to_string (zap_error_t error);


zap_error_t zap_get_raw_byte     (uint8_t id, zap_register_t reg, uint8_t*  result,   int timeout_ms);
zap_error_t zap_get_raw_word     (uint8_t id, zap_register_t reg, uint16_t* result,   int timeout_ms);
zap_error_t zap_get_raw_page     (uint8_t id, zap_register_t reg, uint8_t   result[], int num_bytes, int timeout_ms);

zap_error_t zap_set_raw_byte     (uint8_t id, zap_register_t reg, uint8_t   value,    int timeout_ms);
zap_error_t zap_set_raw_word     (uint8_t id, zap_register_t reg, uint16_t  value,    int timeout_ms);
zap_error_t zap_set_raw_page     (uint8_t id, zap_register_t reg, uint8_t   values[], int num_bytes, int timeout_ms);

zap_error_t zap_prepare_raw_byte (uint8_t id, zap_register_t reg, uint8_t   value,    int timeout_ms);
zap_error_t zap_prepare_raw_word (uint8_t id, zap_register_t reg, uint16_t  value,    int timeout_ms);
zap_error_t zap_prepare_raw_page (uint8_t id, zap_register_t reg, uint8_t   values[], int num_bytes, int timeout_ms);

zap_error_t zap_set_multiple_raw(uint8_t ids[], zap_register_t start_reg, uint8_t values[], int num_ids, int num_bytes_per_zap);
zap_error_t zap_set_positions_and_speeds_raw(uint8_t ids[], zap_register_t start_reg, uint8_t values[], int num_ids, int num_bytes_per_zap);

int   zap_anything_to_raw          (zap_register_t reg, float anything);
float zap_raw_to_anything          (zap_register_t reg, int raw);

/*    0 ~ 2M, 2.25M, 2.5M or 3M. unususal resolution... */
int   zap_baud_bps_to_raw          (float baud);
float zap_raw_to_baud_bps          (int  raw);

/*    0 ~ 100 percent of motor maximum. */
int   zap_force_percentage_to_raw (float percent);
float zap_raw_to_force_percentage (int raw);

/*    5 ~ 25 V, resolution 0.1 V */
int   zap_volts_to_raw             (float volts);
float zap_raw_to_volts             (int raw);

int   zap_resolution_to_raw        (float volts);
float zap_raw_to_resolution        (int raw);

#ifdef __cplusplus
}
#endif

#endif //MIGHTY_ZAP
