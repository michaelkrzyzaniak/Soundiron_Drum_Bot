#include "Mighty_Zap.h"
#include <Arduino.h>

/*--------------------------------------------------------------------------------------------*/
int HALF_DUPLEX_DIRECTION_OUTPUT = HIGH;
int HALF_DUPLEX_DIRECTION_INPUT  = LOW;
HardwareSerial* zap_serial = NULL;
int zap_half_duplex_direction_pin;

/*--------------------------------------------------------------------------------------------*/
typedef enum zap_instruction_enum
{
  ZAP_INSTRUCTION_PING          = 0xF1,
  ZAP_INSTRUCTION_READ_DATA     = 0xF2,
  ZAP_INSTRUCTION_WRITE_DATA    = 0xF3,
  ZAP_INSTRUCTION_PREPARE_DATA  = 0xF4,
  ZAP_INSTRUCTION_DO_PREPARED   = 0xF5,
  ZAP_INSTRUCTION_RESET         = 0xF6,
  ZAP_INSTRUCTION_REBOOT        = 0xF8,
  ZAP_INSTRUCTION_SYNC_WRITE    = 0x73,
}zap_instruction_t;

/*--------------------------------------------------------------------------------------------*/
zap_error_t _zap_set            (uint8_t id, zap_register_t reg, float  val, int timeout_ms, bool do_now);
zap_error_t _zap_set_raw_byte   (uint8_t id, zap_register_t reg, uint8_t  value, int timeout_ms, bool do_now);
zap_error_t _zap_set_raw_word   (uint8_t id, zap_register_t reg, uint16_t value, int timeout_ms,   bool do_now);
zap_error_t _zap_set_raw_page   (uint8_t id, zap_register_t reg, uint8_t  values[], int num_bytes, int timeout_ms, bool do_now);
zap_error_t zap_get_response    (uint8_t id, uint8_t result[], int result_size, int timeout_ms);
zap_error_t zap_send_instruction(uint8_t id, zap_instruction_t instruction,
                                     uint8_t parameters[], uint8_t num_parameters, 
                                     uint8_t result[], int num_results, int timeout_ms);

/*--------------------------------------------------------------------------------------------*/ 
void zap_init(HardwareSerial* serial_port, int half_duplex_direction_pin, int transmit_high, uint32_t baud)
{
    HALF_DUPLEX_DIRECTION_OUTPUT = !!transmit_high;
    HALF_DUPLEX_DIRECTION_INPUT  = !transmit_high;

  zap_serial = serial_port;
  zap_half_duplex_direction_pin = half_duplex_direction_pin;
  zap_serial->begin(baud);
  
  pinMode(half_duplex_direction_pin, OUTPUT);
  digitalWrite(half_duplex_direction_pin, HALF_DUPLEX_DIRECTION_INPUT);
}

/*--------------------------------------------------------------------------------------------*/
void zap_set_interface_baud_rate(uint32_t baud)
{
  zap_serial->begin(baud);
}
/*--------------------------------------------------------------------------------------------*/
bool zap_register_is_word(zap_register_t reg)
{
  bool result = false;
  
  switch(reg)
    {
      case ZAP_REGISTER_MODEL_NUMBER:
      case ZAP_REGISTER_MIN_POSITION:
      case ZAP_REGISTER_MAX_POSITION:
      case ZAP_REGISTER_MAX_FORCE:
      case ZAP_REGISTER_CALIBRATED_MIN_POSITION:
      case ZAP_REGISTER_CALIBRATED_MAX_POSITION:
      case ZAP_REGISTER_CALIBRATED_CENTER_POSITION:
      case ZAP_REGISTER_MIN_POSITION_PW:
      case ZAP_REGISTER_MAX_POSITION_PW:
      case ZAP_REGISTER_CENTER_POSITION_PW:
      case ZAP_REGISTER_CENTER_DIFFERNCE:
      case ZAP_REGISTER_PUNCH_INITIAL_VALUE:
      case ZAP_REGISTER_GOAL_POSITION:
      case ZAP_REGISTER_GOAL_SPEED:
      case ZAP_REGISTER_GOAL_CURRENT:
      case ZAP_REGISTER_PRESENT_POSITION:
      case ZAP_REGISTER_PRESENT_SPEED:
      case ZAP_REGISTER_PRESENT_FORCE:
      case ZAP_REGISTER_PUNCH:
        result = true;
        break;
      default: break;
    }

  return result;
}
                                     
/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_factory_reset    (uint8_t id, int timeout_ms)
{
  return zap_send_instruction(id, ZAP_INSTRUCTION_RESET, NULL, 0, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_ping             (uint8_t id, int timeout_ms)
{
  return zap_send_instruction(id, ZAP_INSTRUCTION_PING, NULL, 0, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_reboot             (uint8_t id, int timeout_ms)
{
  return zap_send_instruction(id, ZAP_INSTRUCTION_REBOOT, NULL, 0, NULL, 0, timeout_ms);
}


/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_get              (uint8_t id, zap_register_t reg, float* result, int timeout_ms)
{
  zap_error_t error;
  bool is_word = zap_register_is_word(reg);
  
  if(is_word)
    {
      uint16_t raw;
      error = zap_get_raw_word(id, reg, &raw, timeout_ms);
      *result = zap_raw_to_anything(reg, (int)raw);
    }
  else
    {
      uint8_t raw;
      error = zap_get_raw_byte(id, reg, &raw, timeout_ms);
      *result = zap_raw_to_anything(reg, (int)raw);
    }
    
  return error;
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t _zap_set              (uint8_t id, zap_register_t reg, float  val, int timeout_ms, bool do_now)
{
  zap_error_t error = ZAP_ERROR_RANGE;
  bool is_word = zap_register_is_word(reg);
  int  raw = zap_anything_to_raw(reg, val);
  
  if(is_word)
    {
      if((raw >= 0) && (raw <= 0xFFFF))
        error = _zap_set_raw_word(id, reg, (uint16_t)raw, timeout_ms, do_now);
    }
  else
    {
      if((raw >= 0) && (raw <= 0xFF))
        error = _zap_set_raw_byte(id, reg, (uint8_t)raw, timeout_ms, do_now);
    }

  return error;
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set              (uint8_t id, zap_register_t reg, float  val, int timeout_ms)
{  
  return _zap_set(id, reg, val, timeout_ms, true);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set_multiple     (uint8_t ids[], zap_register_t start_reg, float values[], int num_ids, int num_values_per_zap)
{  
  int bytes_per_value[num_values_per_zap];
  int* b = bytes_per_value;
  int bytes_per_zap = 0;
  int i, j, k;
  int addr = (int)start_reg;
  
  uint8_t  raw_bytes[num_ids * 2]; //worst case scenario
  uint8_t* r = raw_bytes;
  int      raw_anything;
  
  for(i=0; i<num_values_per_zap; i++)
    {
      *b = (zap_register_is_word((zap_register_t)addr)) ? 2 : 1;
      bytes_per_zap += *b;
      addr += *b++;
    }
  
  for(i=0; i<num_ids; i++)
    {
      addr = start_reg;
      b = bytes_per_value;
      for(j=0; j<num_values_per_zap; j++)
        {
          raw_anything = zap_anything_to_raw((zap_register_t)addr,  *values++);

          for(k=0; k<*b; k++)
            {
              *r++ = (raw_anything & 0xFF);
              raw_anything >>= 8;
            }
          addr += *b++;
        }
    }
  
  return zap_set_multiple_raw(ids, start_reg, raw_bytes, num_ids, bytes_per_zap);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_prepare          (uint8_t id, zap_register_t reg, float  val, int timeout_ms)
{  
  return _zap_set(id, reg, val, timeout_ms, false);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_do_prepared      (uint8_t id, int timeout_ms)
{
  return zap_send_instruction(id, ZAP_INSTRUCTION_DO_PREPARED, NULL, 0, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
char* zap_errors_to_string  (zap_error_t error)
{
  static const char* const_error_strings[] /*PROGMEM*/ = 
    {
      "voltage",
      "position",
      "range",
      "checksum",
      "overload",
      "instruction",
      "timeout",
      "invalid_response",
    };
    
  static char result[100];
  char* r = result;
  
  if(error != ZAP_NO_ERROR)
    {
      int i;
      for(i=0; i<8; i++)
        if ((1 << i) & error)
          {
            //this saves over 1K of code space
            //r += sprintf(r, "%s ", const_error_strings[i]);
            const char* s = const_error_strings[i];
            while(*s != '\0')
              *r++ = *s++;
            *r++ = ' ';
          }
    }
    
  *r = '\0';
  return result;
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_get_raw_byte     (uint8_t id, zap_register_t reg, uint8_t*  result, int timeout_ms)
{
  uint8_t params[2] = {reg, 1};
  return zap_send_instruction(id, ZAP_INSTRUCTION_READ_DATA, params, 2, result, 1, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_get_raw_word     (uint8_t id, zap_register_t reg, uint16_t* result, int timeout_ms)
{
  uint8_t params [2] = {reg, 2};
  uint8_t results[2] = {0, 0};
  zap_error_t error;

  error = zap_send_instruction(id, ZAP_INSTRUCTION_READ_DATA, params, 2, results, 2, timeout_ms);
  *result = ((uint16_t)results[1] << 8) | ((uint16_t)results[0]);
  
  return error;
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_get_raw_page     (uint8_t id, zap_register_t reg, uint8_t   result[], int num_bytes, int timeout_ms)
{
  uint8_t params [2] = {reg, num_bytes};
  return zap_send_instruction(id, ZAP_INSTRUCTION_READ_DATA, params, 2, result, num_bytes, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t _zap_set_raw_byte     (uint8_t id, zap_register_t reg, uint8_t  value, int timeout_ms,  bool do_now)
{
  zap_instruction_t instruction = (do_now) ? ZAP_INSTRUCTION_WRITE_DATA : ZAP_INSTRUCTION_PREPARE_DATA;
  uint8_t params[2] = {reg, value};
  return zap_send_instruction(id, instruction, params, 2, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t _zap_set_raw_word     (uint8_t id, zap_register_t reg, uint16_t value, int timeout_ms,  bool do_now)
{
  zap_instruction_t instruction = (do_now) ? ZAP_INSTRUCTION_WRITE_DATA : ZAP_INSTRUCTION_PREPARE_DATA;
  uint8_t params[3] = {reg, (value & 0xFF), (value >> 8)};
  return zap_send_instruction(id, instruction, params, 3, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t _zap_set_raw_page     (uint8_t id, zap_register_t reg, uint8_t  values[], int num_bytes, int timeout_ms, bool do_now)
{
  zap_instruction_t instruction = (do_now) ? ZAP_INSTRUCTION_WRITE_DATA : ZAP_INSTRUCTION_PREPARE_DATA;
  uint8_t params[num_bytes + 1];
  uint8_t *p = params;
  int n = num_bytes;
  *p++ = reg;
  while(n-- < 0)
    *p++ = *values++;
  
  return zap_send_instruction(id, instruction, params, num_bytes, NULL, 0, timeout_ms);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set_raw_byte     (uint8_t id, zap_register_t reg, uint8_t   value,    int timeout_ms)
{
  return _zap_set_raw_byte(id, reg, value, timeout_ms, true);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set_raw_word     (uint8_t id, zap_register_t reg, uint16_t  value,    int timeout_ms)
{
  return _zap_set_raw_word(id, reg, value, timeout_ms, true);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set_raw_page     (uint8_t id, zap_register_t reg, uint8_t   values[], int num_bytes, int timeout_ms)
{
  return _zap_set_raw_page(id, reg, values, num_bytes, timeout_ms, true);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_prepare_raw_byte (uint8_t id, zap_register_t reg, uint8_t   value,    int timeout_ms)
{
  return _zap_set_raw_byte(id, reg, value, timeout_ms, false);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_prepare_raw_word (uint8_t id, zap_register_t reg, uint16_t  value,    int timeout_ms)
{
  return _zap_set_raw_word(id, reg, value, timeout_ms, false);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_prepare_raw_page (uint8_t id, zap_register_t reg, uint8_t   values[], int num_bytes, int timeout_ms)
{
  return _zap_set_raw_page(id, reg, values, num_bytes, timeout_ms, false);
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_set_multiple_raw(uint8_t ids[], zap_register_t start_reg, uint8_t bytes[], int num_ids, int bytes_per_zap)
{
  int i, j;
  uint8_t num_params = num_ids * (bytes_per_zap + 1) + 2;
  uint8_t params[num_params];
  uint8_t *p = params;
  
  *p++ = start_reg;
  *p++ = bytes_per_zap;
  
  for(i=0; i<num_ids; i++)
    {   
      *p++ = *ids++;
      for(j=0; j<bytes_per_zap; j++)
        *p++ = *bytes++;
    }    
    
  return zap_send_instruction(ZAP_BROADCAST_ID, ZAP_INSTRUCTION_SYNC_WRITE, params, num_params, NULL, 0, 0);
}

/*--------------------------------------------------------------------------------------------*/
void* zap_get_conversion_function_for_register(zap_register_t reg, bool to_raw)
{
  int (*result)() = NULL;
  
  switch(reg)
    {
      case ZAP_REGISTER_BAUD_RATE:
        result = (to_raw) ? 
          (int (*)()) zap_baud_bps_to_raw:
          (int (*)()) zap_raw_to_baud_bps;
        break;
                  
      case ZAP_REGISTER_MAX_VOLTAGE:
      case ZAP_REGISTER_PRESENT_VOLTAGE:
        result = (to_raw) ?
          (int (*)()) zap_volts_to_raw:
          (int (*)()) zap_raw_to_volts;
        break;
        
      case ZAP_REGISTER_MAX_FORCE:
      case ZAP_REGISTER_PRESENT_FORCE:
        result = (to_raw) ?
          (int (*)()) zap_force_percentage_to_raw:
          (int (*)()) zap_raw_to_force_percentage;
        break;

      case ZAP_REGISTER_RESOLUTION_FACTOR:
        result = (to_raw) ?
          (int (*)()) zap_resolution_to_raw:
          (int (*)()) zap_raw_to_resolution;
        break;

      default:
        result = NULL;
      break;
    }

  return (void*)result;
}

/*--------------------------------------------------------------------------------------------*/
int      zap_anything_to_raw (zap_register_t reg, float anything)
{
  int (*conversion)(float) = (int (*)(float))zap_get_conversion_function_for_register(reg, true);
  return (conversion == NULL) ? (int)(anything + 0.5) : conversion(anything);
}

/*--------------------------------------------------------------------------------------------*/
float    zap_raw_to_anything (zap_register_t reg, int raw)
{
  float (*conversion)(int) = (float (*)(int))zap_get_conversion_function_for_register(reg, false);
  return (conversion == NULL) ? (float) raw : conversion(raw);
}

/*--------------------------------------------------------------------------------------------*/
int  zap_baud_bps_to_raw(float baud_bps)
{
  int raw;
  switch((int)baud_bps)
  {
    case ZAP_BAUD_115200:
      raw = 0x10;
      break;
    case ZAP_BAUD_57600:
      raw = 0x20;
      break;
    case ZAP_BAUD_38600:
      raw = 0x30;
      break;
    case ZAP_BAUD_19200:
      raw = 0x40;
      break;
    case ZAP_BAUD_9600:
      raw = 0x80;
      break;
    default:
      raw = ZAP_BAUD_115200;
  }
  return raw;
}

/*--------------------------------------------------------------------------------------------*/
float    zap_raw_to_baud_bps(int raw )
{
  float result;
  switch(raw)
  {
    case 0x10:
      result = 115200;
      break;
    case 0x20:
      result = 57600;
      break;
    case 0x30:
      result = 38600;
      break;
    case 0x40:
      result = 19200;
      break;
    case 0x80:
      result = 9600;
      break;
    default:
      result = raw;
  }
  return result;
}

/*--------------------------------------------------------------------------------------------*/
int   zap_force_percentage_to_raw (float percent)
{
  int result;
  int is_counter_clockwise = (percent < 0);
  if(is_counter_clockwise) percent *= -1;
  result = percent * 10.23 + 0.5;
  if(is_counter_clockwise) result |= 1024;
  
  return result;
}
/*--------------------------------------------------------------------------------------------*/
float zap_raw_to_force_percentage (int raw)
{
  float result;
  int is_negative = (raw >= 0x400); //B10000000000
  raw &= 0x3FF;                     //B01111111111
  result = raw * (100.0 / 1023.0);
  if(is_negative) result *= -1;
  
  return result;
}

/*--------------------------------------------------------------------------------------------*/
int   zap_resolution_to_raw (float resolution)
{
  int result = -1;
  
  result = (int)(0.5 + (resolution / 4096));
  
  if(result < 1) result = 1;
  if(result > 0) result = 0;
  
  return result;
}
/*--------------------------------------------------------------------------------------------*/
//returns power of two, maximum position will be one less.
float zap_raw_to_resolution (int raw)
{
  float result = -1;

  if((raw >= 1) && (raw <= 4))
    result = 4096 / raw;
    
  return result;
}

/*--------------------------------------------------------------------------------------------*/
int   zap_volts_to_raw             (float volts)  {return volts * 10;}
/*--------------------------------------------------------------------------------------------*/
float zap_raw_to_volts             (int raw)      {return raw * 0.1;}


/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_send_instruction(uint8_t id, zap_instruction_t instruction,
                                     uint8_t parameters[], uint8_t num_parameters, 
                                     uint8_t result[], int num_results, int timeout_ms)
{

  int data_size = num_parameters + 7;
  uint8_t data[data_size];
  uint8_t *d = data;
  uint8_t checksum;
  zap_error_t error = ZAP_NO_ERROR;
 
  //could check instruction and number of parameters here;

  *d++ = 0xFF;
  *d++ = 0xFF;
  *d++ = 0xFF;
  *d++ = id;
  *d++ = num_parameters + 2;
  *d++ = (uint8_t) instruction;

  checksum = data[3] + data[4] + data[5];
  
  while(num_parameters-- > 0)
    {
      checksum += *parameters;
      *d++ = *parameters++;
    }

  *d++ = ~checksum;
  
  digitalWrite(zap_half_duplex_direction_pin, HALF_DUPLEX_DIRECTION_OUTPUT);
  //Delay was necessary when on solderless breadboard, but not on custom PCB (probably stray capacitance)
  //delayMicroseconds(10);
  
  while(zap_serial->available()) zap_serial->read(); //remove junk from buffer (there shouldn't be any)
  zap_serial->write(data, data_size);

  //Datasheet says to disable and enable interrupts here but the given reason seems
  //unnecessary, and without interrupts, flush won't know that the last byte has been transmitted
  //zap_serial->write(data, data_size-1);
  //cli();
  //zap_serial->write(data[data_size - 1]);
  zap_serial->flush(); //this waits for all data to be transmitted (does not flush input buffer)
  digitalWrite(zap_half_duplex_direction_pin, HALF_DUPLEX_DIRECTION_INPUT);
  //sei();
 
  if((timeout_ms > 0) && (id != ZAP_BROADCAST_ID))
    error = zap_get_response(id, result, num_results, timeout_ms);

  return error;
}

/*--------------------------------------------------------------------------------------------*/
zap_error_t zap_get_response    (uint8_t id, uint8_t result[], int result_size, int timeout_ms)
{
  int data_size = result_size + 7;
  uint8_t data[data_size];
  uint8_t* d = data;
  int i;
  zap_error_t error = ZAP_NO_ERROR;
  uint8_t checksum;
  timeout_ms *= 100;

  while ((d - data) < data_size)
    {
      if(zap_serial->available())
        *d++ = zap_serial->read();
      else if(timeout_ms-- > 0)
        delayMicroseconds(10);
      else
        break;
    }
  
  if((d-data) != data_size) 
    return ZAP_ERROR_TIMEOUT;

  if((data[0] != 0xFF)          || 
     (data[1] != 0xFF)          ||
     (data[2] != 0xFF)          ||
     (data[3] != id  )          ||
     (data[4] != data_size - 5)  )
    return ZAP_ERROR_INVALID_RESPONSE;
  
  error = (zap_error_t)data[5];

  checksum = data[3] + data[4] + data[5];
  d = data + 6;
  //while(result_size-- > 0)
  for(i=0; i<result_size; i++)
    {
      checksum += *d;
      *result++ = *d++;
    }
  checksum = ~checksum;
  
  if(*d != checksum)
    error = (zap_error_t) (error | ZAP_ERROR_CHECKSUM);

  return error;
}
