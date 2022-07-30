#include "MMap.h"

/*----------------------------------------------------------------*/
void mmap_init()
{
  int magic_johnson = mmap_read (MMAP_MAGIC_NUMBER_ADDR, 2);
  if(magic_johnson != MMAP_MAGIC_NUMBER)
    {
      mmap_write(MMAP_MAGIC_NUMBER_ADDR, MMAP_MAGIC_NUMBER, 2);
      mmap_write(MMAP_ZAP_SERIAL_BAUD   , 57600, 4);
    }
}

/*----------------------------------------------------------------*/
int  mmap_read (mmap_address address, int num_bytes)
{
  if((address + num_bytes) >= 1024)
    return 0;
  if(address < 0)
    return 0;
 
  int result = 0;
  int n = num_bytes;
  
  while(n > 0)
    {
      int temp = EEPROM.read(address);
      temp <<= 8 * (num_bytes-n);
      result |= temp;
      address = (mmap_address)((int)address + 1);
      --n;
    }
    
   int shift = 8*(4 - num_bytes);
   result <<= shift;
   result >>= shift;
    
   return result;
}

/*----------------------------------------------------------------*/
void  mmap_write(mmap_address address, int value, int num_bytes)
{
  if((address + num_bytes) >= 1024)
    return;
  if(address < 0)
    return;
  
  while(num_bytes > 0)
    {
      EEPROM.write(address, value & 0xFF);
      value >>= 8;
      address = (mmap_address)((int)address + 1);
      --num_bytes;
    }
}

/*----------------------------------------------------------------*/
int  mmap_read_plucker (mmap_address plucker_0_address, int plucker, int num_bytes)
{
  return mmap_read((mmap_address)((int)plucker_0_address+(num_bytes*plucker)), num_bytes);
}

/*----------------------------------------------------------------*/
void mmap_write_plucker(mmap_address plucker_0_address, int plucker, int val, int num_bytes)
{
  mmap_write((mmap_address)((int)plucker_0_address+(num_bytes*plucker)), val, num_bytes);
}
