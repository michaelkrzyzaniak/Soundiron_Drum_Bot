#ifndef __MMAP__
#define __MMAP__

#include "Arduino.h" 
#include "EEPROM.h" 

#define MMAP_MAGIC_NUMBER 0x1237

typedef enum mmap_address_enum
{
  MMAP_MAGIC_NUMBER_ADDR       = 0, //2 bytes
  MMAP_ZAP_SERIAL_BAUD         = 6, //4 bytes

 //2k eeprom (emulated) on Teensy 3.2 
 //1k eeprom (emulated) on Teensy 4.0 
}mmap_address;

void mmap_init();
int  mmap_read (mmap_address address, int num_bytes);
void mmap_write(mmap_address address, int val, int num_bytes);

int  mmap_read_plucker (mmap_address plucker_0_address, int plucker, int num_bytes);
void mmap_write_plucker(mmap_address plucker_0_address, int plucker, int val, int num_bytes);


#endif   //__MMAP__
