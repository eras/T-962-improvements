/*
 * eeprom.c - I2C EEPROM interface for T-962 reflow controller
 *
 * Copyright (C) 2014 Werner Johansson, wj@unifiedengineering.se
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LPC214x.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "t962.h"
#include "eeprom.h"
#include "i2c.h"

#define EEADDR (0x50<<1)

void EEPROM_Init( void ) {
#define DUMP_EEPROM
#ifdef DUMP_EEPROM
	uint8_t dumpbuf[256];
	EEPROM_Read(dumpbuf, 0, sizeof(dumpbuf));
	printf("\nEEPROM contents:\n");
	for(int foo=0; foo < sizeof(dumpbuf); foo++) {
		printf(" 0x%02x",dumpbuf[foo]);
	}
#endif
	// No init needed at this point, maybe detect the actual presence some day
}

int32_t EEPROM_Read(uint8_t* dest, uint32_t startpos, uint32_t len) {
	int32_t retval=0;
	if(startpos<256 && dest && len && len<=256) {
		uint8_t offset=(uint8_t)startpos;
		retval = I2C_Xfer(EEADDR, &offset, 1, 0); // Set address pointer to startpos
		if(!retval) retval = I2C_Xfer(EEADDR | 1, dest, len, 1); // Read requested data
	}
	return retval;
}

int32_t EEPROM_Write(uint32_t startdestpos, uint8_t* src, uint32_t len) {
	int32_t retval=0;
	if(startdestpos<256 && len && len<=256) {
		uint8_t tmpbuf[9];
		uint8_t i = startdestpos;
		while(len) {
			uint32_t loopcnt=0;
			uint8_t startoffset = i & 0x07;
			uint8_t maxcopysize = 8 - startoffset;
			uint8_t bytestocopy = (len>maxcopysize)?maxcopysize:len; // up to 8 bytes at a time depending on alignment
			tmpbuf[0] = i;
			memcpy(tmpbuf+1, src, bytestocopy);
			retval = I2C_Xfer(EEADDR, tmpbuf, bytestocopy + 1, 1); // Set address pointer and provide up to 8 bytes of data for page write
			if(!retval) {
				do {
					retval = I2C_Xfer(EEADDR, tmpbuf, 1, 1); // Dummy write to poll timed write cycle completion
					loopcnt++;
				} while(retval && loopcnt<400); // 5ms max write cycle. 200kHz bus freq & 10 bits per poll makes this a 20ms timeout
				if(retval) {
					printf("\nTimeout getting ACK from EEPROM during write!");
					break;
				}
				len -= bytestocopy;
				i += bytestocopy;
				src += bytestocopy;
			} else {
				printf("\nFailed to write to EEPROM!");
				retval = -2;
				break;
			}
		}
	} else {
		printf("\nInvalid EEPROM addressing");
		retval = -3;
	}
	return retval;
}

