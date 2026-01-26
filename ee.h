/*
 * ee.h
 *
 *  Created on: Jan 26, 2026
 *      Author: quyen
 */

#ifndef _EE_H_
#define _EE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include <stdbool.h>
#include "main.h"

/*************************************************************************************************/
/** Typedef/Struct/Enum **/
/*************************************************************************************************/

/*************************************************************************************************/
/* Handle struct of EEPROM */
typedef struct
{
  uint8_t                *data;
  uint32_t               size;
  uint32_t               page_sector_size;
  uint32_t               address;
  uint8_t                page_sector_number;
#if (defined FLASH_BANK_1) || (defined FLASH_BANK_2)
  uint8_t                bank_number;
#endif

} ee_t;

/*************************************************************************************************/
/** API Functions **/
/*************************************************************************************************/

/* Initializes the EEPROM emulation module */
bool      ee_init(void *data, uint32_t size);

/* Retrieves the capacity of the EEPROM emulation area */
uint32_t  ee_capacity(void);

/* Formats the EEPROM emulation area */
bool      ee_format(void);

/* Reads data from the EEPROM emulation area */
void      ee_read(void);

/* Writes data to the EEPROM emulation area */
bool      ee_write(void);

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif

