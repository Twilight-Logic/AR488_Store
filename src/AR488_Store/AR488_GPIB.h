#ifndef AR488_GPIB_H
#define AR488_GPIB_H

#include <SD.h>
#include "AR488_Config.h"
#include "AR488_Layouts.h"

/***** AR488_Eeprom.cpp, ver. 0.03.03, 12/02/2021 *****/


/*********************************************/
/***** GPIB COMMAND & STATUS DEFINITIONS *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv *****/

// Universal Multiline commands (apply to all devices)

#define GC_LLO 0x11
#define GC_DCL 0x14
#define GC_PPU 0x15
#define GC_SPE 0x18
#define GC_SPD 0x19
#define GC_UNL 0x3F
#define GC_TAD 0x40
#define GC_PPE 0x60
#define GC_PPD 0x70
#define GC_UNT 0x5F
// Address commands
#define GC_LAD 0x20
// Addressed commands
#define GC_GTL 0x01
#define GC_SDC 0x04
#define GC_PPC 0x05
#define GC_GET 0x08

/***** GPIB control states *****/
// Controller mode
/*
#define CINI 0x01 // Controller idle state
#define CIDS 0x02 // Controller idle state
#define CCMS 0x03 // Controller command state
#define CTAS 0x04 // Controller talker active state
#define CLAS 0x05 // Controller listner active state
*/
// Listner/device mode
#define DINI 0x06 // Device initialise state
#define DIDS 0x07 // Device idle state
#define DLAS 0x08 // Device listener active (listening/receiving)
#define DTAS 0x09 // Device talker active (sending) state

/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** GPIB COMMAND & STATUS DEFINITIONS *****/
/*********************************************/

bool isAtnAsserted();
//bool gpibSendCmd(uint8_t cmdByte);
void gpibSendStatus();
void gpibSendData(char *data, uint8_t dsize);
bool gpibReceiveData();
bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence);
uint8_t gpibReadByte(uint8_t *db, bool *eoi);
bool gpibWriteByte(uint8_t db);
bool gpibWriteByteHandshake(uint8_t db);
//bool addrDev(uint8_t addr, bool dir);
//bool uaddrDev();
boolean Wait_on_pin_state(uint8_t state, uint8_t pin, int interval);
void setGpibControls(uint8_t state);
void gpibSendFromFile(File sdfile);
bool gpibWriteToFile(File sdfile);


#endif // AR488_GPIB_H
