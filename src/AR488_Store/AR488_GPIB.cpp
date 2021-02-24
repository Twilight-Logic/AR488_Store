#include <Arduino.h>
//#include <SD.h>
#include "AR488_Config.h"
#include "AR488_GPIB.h"

/***** AR488_GPIB.cpp, ver. 0.04.01, 23/02/2021 *****/


/****** Process status values *****/
#define OK 0
#define ERR 1

/***** Control characters *****/
#define ESC  0x1B   // the USB escape char
#define CR   0xD    // Carriage return
#define LF   0xA    // Newline/linefeed
#define PLUS 0x2B   // '+' character


/***** Serial/debug port *****/
#ifdef AR_CDC_SERIAL
  extern Serial_ *arSerial;
  #ifndef DB_SERIAL_PORT
    extern Serial_ *dbSerial;
  #endif
#endif
#ifdef AR_HW_SERIAL
  extern HardwareSerial *arSerial;
  #ifndef DB_SERIAL_PORT
    extern HardwareSerial *dbSerial;
  #endif
#endif
#ifdef AR_SW_SERIAL
  #include <SoftwareSerial.h>
//  SoftwareSerial swArSerial(AR_SW_SERIAL_RX, AR_SW_SERIAL_TX);
  extern SoftwareSerial *arSerial;
  #ifndef DB_SERIAL_PORT
    extern SoftwareSerial *dbSerial;
  #endif
#endif


/***** Debug Port *****/
#ifdef DB_SERIAL_PORT
  #ifdef DB_CDC_SERIAL
    extern Serial_ *dbSerial;
  #endif
  #ifdef DB_HW_SERIAL
    extern HardwareSerial *dbSerial;
  #endif
  // Note: SoftwareSerial support conflicts with PCINT support
  #ifdef DB_SW_SERIAL
    #include <SoftwareSerial.h>
//    SoftwareSerial swDbSerial(DB_SW_SERIAL_RX, DB_SW_SERIAL_TX);
    extern SoftwareSerial *dbSerial;
  #endif
#endif


extern union AR488conf AR488;

extern bool isVerb;
extern bool isRO;
extern bool dataBufferFull;
extern bool rEoi;
extern bool rEbt;
extern bool isATN;
extern uint8_t tranBrk;
extern uint8_t lnRdy;
extern uint8_t eByte;
extern uint8_t cstate;


/***************************************/
/***** GPIB DATA HANDLING ROUTINES *****/
/***************************************/


/***** Detect ATN state *****/
/*
 * When interrupts are being used the state is automatically flagged when
 * the ATN interrupt is triggered. Where the interrupt cannot be used the
 * state of the ATN line needs to be checked.
 */
bool isAtnAsserted() {
#ifdef USE_INTERRUPTS
  if (isATN) return true;
#else
  // ATN is LOW when asserted
  if (digitalRead(ATN) == LOW) return true;
#endif
  return false;
}


/*****  Send a single byte GPIB command *****/
/*
bool gpibSendCmd(uint8_t cmdByte) {

  bool stat = false;

  // Set lines for command and assert ATN
  setGpibControls(CCMS);

  // Send the command
  stat = gpibWriteByte(cmdByte);
  if (stat && isVerb) {
    arSerial->print(F("gpibSendCmd: failed to send command "));
    arSerial->print(cmdByte, HEX);
    arSerial->println(F(" to device"));
  }

  // Return to controller idle state
  //  setGpibControls(CIDS);
  // NOTE: this breaks serial poll

  return stat ? ERR : OK;
}
*/

/***** Send the status byte *****/
void gpibSendStatus() {
  // Have been addressed and polled so send the status byte
  if (isVerb) {
    arSerial->print(F("Sending status byte: "));
    arSerial->println(AR488.stat);
  };
  setGpibControls(DTAS);
  gpibWriteByte(AR488.stat);
  setGpibControls(DIDS);
}


/***** Send a series of characters as data to the GPIB bus *****/
void gpibSendData(char *data, uint8_t dsize) {

  bool err = false;

  // If lon is turned on we cannot send data so exit
  if (isRO) return;

    // Set GPIB state for writing data
    setGpibControls(DTAS);

#ifdef DEBUG3
  dbSerial->println(F("Set write data mode."));
  dbSerial->print(F("Send->"));
#endif

  // Write the data string
  for (int i = 0; i < dsize; i++) {
    // If EOI asserting is on
    if (AR488.eoi) {
      // Send all characters
      err = gpibWriteByte(data[i]);
    } else {
      // Otherwise ignore non-escaped CR, LF and ESC
      if ((data[i] != CR) || (data[i] != LF) || (data[i] != ESC)) err = gpibWriteByte(data[i]);
    }
#ifdef DEBUG3
    dbSerial->print(data[i]);
#endif
    if (err) break;
  }

#ifdef DEBUG3
  dbSerial->println("<-End.");
#endif

  if (!err) {
    // Write terminators according to EOS setting
    // Do we need to write a CR?
    if ((AR488.eos & 0x2) == 0) {
      gpibWriteByte(CR);
#ifdef DEBUG3
      dbSerial->println(F("Appended CR"));
#endif
    }
    // Do we need to write an LF?
    if ((AR488.eos & 0x1) == 0) {
      gpibWriteByte(LF);
#ifdef DEBUG3
      dbSerial->println(F("Appended LF"));
#endif
    }
  }

  // If EOI enabled and no more data to follow then assert EOI
  if (AR488.eoi && !dataBufferFull) {
    setGpibState(0b00000000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00000000, 0b00010000);
    delayMicroseconds(40);
    setGpibState(0b00010000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00010000, 0b00010000);
#ifdef DEBUG3
    dbSerial->println(F("Asserted EOI"));
#endif
  }

  // Set control lines to idle
  setGpibControls(DIDS);

#ifdef DEBUG3
    dbSerial->println(F("<- End of send."));
#endif
 
}


/***** Receive data from the GPIB bus ****/
/*
 * Readbreak:
 * 5 - EOI detected
 * 7 - command received via serial
 */
bool gpibReceiveData() {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  uint8_t eor = AR488.eor&7;
  int x = 0;
  bool eoiStatus;
  bool eoiDetected = false;

  // Reset transmission break flag
  tranBrk = 0;

  // Set status of EOI detection
  eoiStatus = rEoi; // Save status of rEoi flag
  if (AR488.eor==7) rEoi = true;    // Using EOI as terminator

  // Set GPIB controls to device read mode
  setGpibControls(DLAS);
  rEoi = true;  // In device mode we read with EOI by default

#ifdef DEBUG7
    dbSerial->println(F("gpibReceiveData: Start listen ->"));
    dbSerial->println(F("Before loop flags:"));
    dbSerial->print(F("TRNb: "));
    dbSerial->println(tranBrk);
    dbSerial->print(F("rEOI: "));
    dbSerial->println(rEoi);
    dbSerial->print(F("ATN:  "));
    dbSerial->println(isAtnAsserted() ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (tranBrk > 0) break;

    // ATN asserted
    if (isAtnAsserted()) break;

    // Read the next character on the GPIB bus
    r = gpibReadByte(&bytes[0], &eoiDetected);

    // When reading with amode=3 or EOI check serial input and break loop if neccessary
//    if ((AR488.amode==3) || rEoi) lnRdy = serialIn_h();
    
    // Line terminator detected (loop breaks on command being detected or data buffer full)
/*    
    if (lnRdy > 0) {
      aRead = false;  // Stop auto read
      break;
    }
*/

#ifdef DEBUG7
    if (eoiDetected) dbSerial->println(F("\r\nEOI detected."));
#endif

    // If ATN asserted then break here
    if (isAtnAsserted()) break;

#ifdef DEBUG7
    dbSerial->print(bytes[0], HEX), dbSerial->print(' ');
#else
    // Output the character to the serial port
    arSerial->print((char)bytes[0]);
#endif

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (rEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (isTerminatorDetected(bytes, eor)) break;
    }

    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG7
  dbSerial->println();
  dbSerial->println(F("After loop flags:"));
  dbSerial->print(F("ATN: "));
  dbSerial->println(isAtnAsserted());
  dbSerial->print(F("TMO:  "));
  dbSerial->println(r);
#endif

  // End of data - if verbose, report how many bytes read
  if (isVerb) {
    arSerial->print(F("Bytes read: "));
    arSerial->println(x);
  }

  // Detected that EOI has been asserted
  if (eoiDetected) {
    if (isVerb) arSerial->println(F("EOI detected!"));
    // If eot_enabled then add EOT character
    if (AR488.eot_en) arSerial->print(AR488.eot_ch);
  }

  // Return rEoi to previous state
  rEoi = eoiStatus;

  // Verbose timeout error
  if (r > 0) {
    if (isVerb && r == 1) arSerial->println(F("Timeout waiting for sender!"));
    if (isVerb && r == 2) arSerial->println(F("Timeout waiting for transfer to complete!"));
  }

  // Set device back to idle state
  setGpibControls(DIDS);

#ifdef DEBUG7
    dbSerial->println(F("<- End listen."));
#endif

  // Reset flags
//  isReading = false;
  if (tranBrk > 0) tranBrk = 0;

  if (r > 0) return ERR;

  return OK;
}


/***** Check for terminator *****/
bool isTerminatorDetected(uint8_t bytes[3], uint8_t eor_sequence){
  if (rEbt) {
    // Stop on specified <char> if appended to ++read command
    if (bytes[0] == eByte) return true;
  }else{
    // Look for specified terminator (CR+LF by default)
    switch (eor_sequence) {
      case 0:
          // CR+LF terminator
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      case 1:
          // CR only as terminator
          if (bytes[0]==CR) return true;
          break;
      case 2:
          // LF only as terminator
          if (bytes[0]==LF) return true;
          break;
      case 3:
          // No terminator (will rely on timeout)
          break;
      case 4:
          // Keithley can use LF+CR instead of CR+LF
          if (bytes[0]==CR && bytes[1]==LF) return true;
          break;
      case 5:
          // Solarton (possibly others) can also use ETX (0x03)
          if (bytes[0]==0x03) return true;
          break;
      case 6:
          // Solarton (possibly others) can also use CR+LF+ETX (0x03)
          if (bytes[0]==0x03 && bytes[1]==LF && bytes[2]==CR) return true;
          break;
      default:
          // Use CR+LF terminator by default
          if (bytes[0]==LF && bytes[1]==CR) return true;
          break;
      }
  }
  return false;
}


/***** Read a SINGLE BYTE of data from the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to read data    )
 * (- the GPIB bus must already be configured to listen )
 */
uint8_t gpibReadByte(uint8_t *db, bool *eoi) {
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse, i.e. asserted=true; unasserted=false;
  *eoi = false;

  // Unassert NRFD (we are ready for more data)
  setGpibState(0b00000100, 0b00000100, 0);

  // ATN asserted and just got unasserted - abort - we are not ready yet
  if (atnStat && (digitalRead(ATN)==HIGH)) {
    setGpibState(0b00000000, 0b00000100, 0);
    return 3;
  }

  // Wait for DAV to go LOW indicating talker has finished setting data lines..
  if (Wait_on_pin_state(LOW, DAV, AR488.rtmo))  {
    if (isVerb) arSerial->println(F("gpibReadByte: timeout waiting for DAV to go LOW"));
    setGpibState(0b00000000, 0b00000100, 0);
    // No more data for you?
    return 1;
  }

  // Assert NRFD (NOT ready - busy reading data)
  setGpibState(0b00000000, 0b00000100, 0);

  // Check for EOI signal
  if (rEoi && digitalRead(EOI) == LOW) *eoi = true;

  // read from DIO
  *db = readGpibDbus();

  // Unassert NDAC signalling data accepted
  setGpibState(0b00000010, 0b00000010, 0);

  // Wait for DAV to go HIGH indicating data no longer valid (i.e. transfer complete)
  if (Wait_on_pin_state(HIGH, DAV, AR488.rtmo))  {
    if (isVerb) arSerial->println(F("gpibReadByte: timeout waiting DAV to go HIGH"));
    return 2;
  }

  // Re-assert NDAC - handshake complete, ready to accept data again
  setGpibState(0b00000000, 0b00000010, 0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  return 0;

}


/***** Write a SINGLE BYTE onto the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to send data )
 */
bool gpibWriteByte(uint8_t db) {

  bool err;

  err = gpibWriteByteHandshake(db);

  // Unassert DAV
  setGpibState(0b00001000, 0b00001000, 0);

  // Reset the data bus
  setGpibDbus(0);

  // GPIB bus DELAY
  delayMicroseconds(AR488.tmbus);

  // Exit successfully
  return err;
}


/***** GPIB send byte handshake *****/
bool gpibWriteByteHandshake(uint8_t db) {
  
    // Wait for NDAC to go LOW (indicating that devices are at attention)
  if (Wait_on_pin_state(LOW, NDAC, AR488.rtmo)) {
    if (isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for receiver attention [NDAC asserted]"));
    return true;
  }
  // Wait for NRFD to go HIGH (indicating that receiver is ready)
  if (Wait_on_pin_state(HIGH, NRFD, AR488.rtmo))  {
    if (isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for receiver ready - [NRFD unasserted]"));
    return true;
  }

  // Place data on the bus
  setGpibDbus(db);

  // Assert DAV (data is valid - ready to collect)
  setGpibState(0b00000000, 0b00001000, 0);

  // Wait for NRFD to go LOW (receiver accepting data)
  if (Wait_on_pin_state(LOW, NRFD, AR488.rtmo))  {
    if (isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for data to be accepted - [NRFD asserted]"));
    return true;
  }

  // Wait for NDAC to go HIGH (data accepted)
  if (Wait_on_pin_state(HIGH, NDAC, AR488.rtmo))  {
    if (isVerb) arSerial->println(F("gpibWriteByte: timeout waiting for data accepted signal - [NDAC unasserted]"));
    return true;
  }

  return false;
}


/***** Untalk bus then address a device *****/
/*
 * dir: 0=listen; 1=talk;
 */
/* 
bool addrDev(uint8_t addr, bool dir) {
  uint8_t saddr = 0;
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (dir) {
    // Device to talk, controller to listen
    if (gpibSendCmd(GC_TAD + addr)) return ERR;
    if (gpibSendCmd(GC_LAD + AR488.caddr)) return ERR;
  } else {
    // Device to listen, controller to talk
    if (gpibSendCmd(GC_LAD + addr)) return ERR;
    if (gpibSendCmd(GC_TAD + AR488.caddr)) return ERR;
  }
  // Send secondary address if available
  if (AR488.saddr){
    saddr = AR488.saddr;
    AR488.saddr = 0;  // Clear the address for next "command"
    if (gpibSendCmd(saddr)) return ERR;
  }
  return OK;
}
*/

/***** Unaddress a device (untalk bus) *****/
/*
bool uaddrDev() {
  // De-bounce
  delayMicroseconds(30);
  // Utalk/unlisten
  if (gpibSendCmd(GC_UNL)) return ERR;
  if (gpibSendCmd(GC_UNT)) return ERR;
  return OK;
}
*/


/***** Send contents of a file to the GPIB bus *****/
/*
 * Assumes file has already been opened with appropriate
 * read flag:
 * FILE_READ
 * FILE_RDONLY
 * FILE_RDWR
 */
/* 
void gpibSendDataFromFile(File sdfile) {

  bool err = false;

  // If lon is turned on we cannot send data so exit
//  if (isRO) return;

  // Set GPIB state for writing data
  setGpibControls(DTAS);

  // Write the data string
//  for (int i = 0; i < dsize; i++) {
  while (sdfile.available()){
    // If EOI asserting is on
    if (AR488.eoi) {
      // Send characters
      err = gpibWriteByte(sdfile.read());
    } else {
      // Otherwise ignore non-escaped CR, LF and ESC
//      if ((data[i] != CR) || (data[i] != LF) || (data[i] != ESC)) err = gpibWriteByte(data[i]);
    }
    if (err) break;
  }

#ifdef DEBUG11
  dbSerial->println("<-End.");
#endif

  if (!err) {
    // Write terminators according to EOS setting
    // Do we need to write a CR?
    if ((AR488.eos & 0x2) == 0) {
      gpibWriteByte(CR);
#ifdef DEBUG11
      dbSerial->println(F("Appended CR"));
#endif
    }
    // Do we need to write an LF?
    if ((AR488.eos & 0x1) == 0) {
      gpibWriteByte(LF);
#ifdef DEBUG11
      dbSerial->println(F("Appended LF"));
#endif
    }
  }

  // If EOI enabled and no more data to follow then assert EOI
  if (AR488.eoi && !dataBufferFull) {
    setGpibState(0b00000000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00000000, 0b00010000);
    delayMicroseconds(40);
    setGpibState(0b00010000, 0b00010000, 0);
    //    setGpibState(0b00010000, 0b00010000, 0b00010000);
#ifdef DEBUG11
    dbSerial->println(F("Asserted EOI"));
#endif
  }

  // Set control lines to idle
  setGpibControls(DIDS);

#ifdef DEBUG11
    dbSerial->println(F("<- End of send."));
#endif
 
}
*/

/***** Write to a file from the GPIB bus *****/
/*
 * Assumes file has already been opened with the appropriate
 * write method:
 * FILE_WRITE
 * FILE_APPEND
 */
/*
bool gpibWriteToFile(File sdfile) {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  uint8_t eor = AR488.eor&7;
  int x = 0;
  bool eoiStatus;
  bool eoiDetected = false;

  // Reset transmission break flag
  tranBrk = 0;

  // Set status of EOI detection
  eoiStatus = rEoi; // Save status of rEoi flag
  if (AR488.eor==7) rEoi = true;    // Using EOI as terminator

  // Set GPIB controls to device read mode
  setGpibControls(DLAS);
  rEoi = true;  // In device mode we read with EOI by default

#ifdef DEBUG12
    dbSerial->println(F("gpibReceiveData: Start listen ->"));
    dbSerial->println(F("Before loop flags:"));
    dbSerial->print(F("TRNb: "));
    dbSerial->println(tranBrk);
    dbSerial->print(F("rEOI: "));
    dbSerial->println(rEoi);
    dbSerial->print(F("ATN:  "));
    dbSerial->println(isAtnAsserted() ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (tranBrk > 0) break;

    // ATN asserted
    if (isAtnAsserted()) break;

    // Read the next character on the GPIB bus
    r = gpibReadByte(&bytes[0], &eoiDetected);

    // When reading with amode=3 or EOI check serial input and break loop if neccessary
//    if ((AR488.amode==3) || rEoi) lnRdy = serialIn_h();
    
    // Line terminator detected (loop breaks on command being detected or data buffer full)
*/
    
/*    
    if (lnRdy > 0) {
      aRead = false;  // Stop auto read
      break;
    }
*/


/*
#ifdef DEBUG12
    if (eoiDetected) dbSerial->println(F("\r\nEOI detected."));
#endif

    // If ATN asserted then break here
    if (isAtnAsserted()) break;

#ifdef DEBUG12
    dbSerial->print(bytes[0], HEX), dbSerial->print(' ');
#else
    // Output the character to the serial port  
//    arSerial->print((char)bytes[0]);
    // Write the character to the file    
    sdfile.write(r);
#endif

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (rEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (isTerminatorDetected(bytes, eor)) break;
    }

    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG12
  dbSerial->println();
  dbSerial->println(F("After loop flags:"));
  dbSerial->print(F("ATN: "));
  dbSerial->println(isAtnAsserted());
  dbSerial->print(F("TMO:  "));
  dbSerial->println(r);
#endif

  // End of data - if verbose, report how many bytes read
  if (isVerb) {
    arSerial->print(F("Bytes read: "));
    arSerial->println(x);
  }

  // Detected that EOI has been asserted
  if (eoiDetected) {
    if (isVerb) arSerial->println(F("EOI detected!"));
    // If eot_enabled then add EOT character
    if (AR488.eot_en) arSerial->print(AR488.eot_ch);
  }

  // Return rEoi to previous state
  rEoi = eoiStatus;

  // Verbose timeout error
  if (r > 0) {
    if (isVerb && r == 1) arSerial->println(F("Timeout waiting for sender!"));
    if (isVerb && r == 2) arSerial->println(F("Timeout waiting for transfer to complete!"));
  }

  // Set device back to idle state
  setGpibControls(DIDS);

#ifdef DEBUG12
    dbSerial->println(F("<- End listen."));
#endif

  // Reset flags
//  isReading = false;
  if (tranBrk > 0) tranBrk = 0;

  if (r > 0) return ERR;

  return OK;
}
*/


/**********************************/
/*****  GPIB CONTROL ROUTINES *****/
/**********************************/


/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
boolean Wait_on_pin_state(uint8_t state, uint8_t pin, int interval) {

  unsigned long timeout = millis() + interval;
  bool atnStat = (digitalRead(ATN) ? false : true); // Set to reverse - asserted=true; unasserted=false;

  while (digitalRead(pin) != state) {
    // Check timer
    if (millis() >= timeout) return true;
    // ATN status was asserted but now unasserted so abort
    if (atnStat && (digitalRead(ATN)==HIGH)) return true;
    //    if (digitalRead(EOI)==LOW) tranBrk = 2;
  }
  return false;        // = no timeout therefore succeeded!
}

/***** Control the GPIB bus - set various GPIB states *****/
/*
 * state is a predefined state (CINI, CIDS, CCMS, CLAS, CTAS, DINI, DIDS, DLAS, DTAS);
 * Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC
 * setGpibState byte1 (databits) : State - 0=LOW, 1=HIGH/INPUT_PULLUP; Direction - 0=input, 1=output;
 * setGpibState byte2 (mask)     : 0=unaffected, 1=enabled
 * setGpibState byte3 (mode)     : 0=set pin state, 1=set pin direction
 */
void setGpibControls(uint8_t state) {

  // Switch state
  switch (state) {
/*
    // Controller states
    case CINI:  // Initialisation
      // Set pin direction
      setGpibState(0b10111000, 0b11111111, 1);
      // Set pin state
      setGpibState(0b11011111, 0b11111111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,LOW);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,HIGH);
  #endif
#endif      
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB control mode"));
#endif
      break;

    case CIDS:  // Controller idle state
      setGpibState(0b10111000, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case CCMS:  // Controller active - send commands
      setGpibState(0b10111001, 0b10011111, 1);
      setGpibState(0b01011111, 0b10011111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for sending a command"));
#endif
      break;

    case CLAS:  // Controller - read data bus
      // Set state for receiving data
      setGpibState(0b10100110, 0b10011110, 1);
      setGpibState(0b11011000, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for reading data"));
#endif
      break;

    case CTAS:  // Controller - write data bus
      setGpibState(0b10111001, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for writing data"));
#endif
      break;
*/
    /* Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC */

    // Listener states
    case DINI:  // Listner initialisation
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
  #ifdef SN7516X_DC
        digitalWrite(SN7516X_DC,HIGH);
  #endif
  #ifdef SN7516X_SC
        digitalWrite(SN7516X_SC,LOW);
  #endif
#endif      
      setGpibState(0b00000000, 0b11111111, 1);
      setGpibState(0b11111111, 0b11111111, 0);
#ifdef DEBUG2
      dbSerial->println(F("Initialised GPIB listener mode"));
#endif
      break;

    case DIDS:  // Device idle state
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
      setGpibState(0b00000000, 0b00001110, 1);
      setGpibState(0b11111111, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DLAS:  // Device listner active (actively listening - can handshake)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
      setGpibState(0b00000110, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines to idle state"));
#endif
      break;

    case DTAS:  // Device talker active (sending data)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
      setGpibState(0b00001000, 0b00001110, 1);
      setGpibState(0b11111001, 0b00001110, 0);
#ifdef DEBUG2
      dbSerial->println(F("Set GPIB lines for listening as addresed device"));
#endif
      break;
#ifdef DEBUG2

    default:
      // Should never get here!
      //      setGpibState(0b00000110, 0b10111001, 0b11111111);
      dbSerial->println(F("Unknown GPIB state requested!"));
#endif
  }

  // Save state
  cstate = state;

  // GPIB bus delay (to allow state to settle)
  delayMicroseconds(AR488.tmbus);

}
