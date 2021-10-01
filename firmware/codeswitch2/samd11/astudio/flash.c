/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Written by Cristian Maglie

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  modified from C++ --> C     by LeonH
*/



#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd11.h"
#include "flash.h"

static const uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
uint32_t PAGE_SIZE, PAGES, MAX_FLASH, ROW_SIZE;
const volatile void *flash_address;
uint32_t flash_size;



void flash_config(void) 
{
  PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ]; // 64 bytes (SAMD11)
  PAGES = NVMCTRL->PARAM.bit.NVMP;               // 256 pages
  MAX_FLASH = PAGE_SIZE * PAGES;                 // 16384
  ROW_SIZE = PAGE_SIZE * 4;                      // 256
  FlashStorage(my_flash_addr,Eeprom);
  flash_size = sizeof(Eeprom);
}

static inline uint32_t read_unaligned_uint32(const void *data)
{
  union {
    uint32_t u32;
    uint8_t u8[4];
  } res;
  const uint8_t *d = (const uint8_t *)data;
  res.u8[0] = d[0];
  res.u8[1] = d[1];
  res.u8[2] = d[2];
  res.u8[3] = d[3];
  return res.u32;
}

void flash_write(const uint8_t *data)
{
  const volatile void *flash_ptr = flash_address;
  uint32_t size = flash_size;
              
  // erase flash area first:
  flash_erase_sized();
    
  // Calculate data boundaries
  size = (size + 3) / 4;
  volatile uint32_t *dst_addr = (volatile uint32_t *)flash_ptr;
  const uint8_t *src_addr = data;
  
  // Disable automatic page write
  NVMCTRL->CTRLB.bit.MANW = 1;

  // Do writes in pages
  while (size) {
    // Execute "PBC" Page Buffer Clear
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
    while (NVMCTRL->INTFLAG.bit.READY == 0) { }

    // Fill page buffer
    uint32_t i;
    for (i=0; i<(PAGE_SIZE/4) && size; i++) {
      *dst_addr = read_unaligned_uint32(src_addr);
      src_addr += 4;
      dst_addr++;
      size--;
    }

    // Execute "WP" Write Page
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
    while (NVMCTRL->INTFLAG.bit.READY == 0) { }
  }
}

void flash_erase(const volatile void *flash_ptr)
{
    NVMCTRL->ADDR.reg = ((uint32_t)flash_ptr) / 2;
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
    while (!NVMCTRL->INTFLAG.bit.READY) { }
}

void flash_erase_sized(void)
{
    const volatile void *flash_ptr = flash_address;
    uint32_t size = flash_size;
    const uint8_t *ptr = (const uint8_t *)flash_ptr;
    while (size > ROW_SIZE) {
      flash_erase(ptr);
      ptr += ROW_SIZE;
      size -= ROW_SIZE;
    }
    flash_erase(ptr);
}


void flash_read(void *data)
{
    const volatile void *flash_ptr = flash_address;
    uint32_t size = flash_size;
    memcpy(data, (const void *)flash_ptr, size);
}

