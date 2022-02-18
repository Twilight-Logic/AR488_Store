#include <Arduino.h>
//#include <SD.h>
#include "AR488_Config.h"
#include "AR488_GPIBdevice.h"

/***** AR488_GPIB.cpp, ver. 0.05.65, 18/02/2022 *****/


/****** Process status values *****/
#define OK 0
#define ERR 1

/***** Control characters *****/
#define ESC  0x1B   // the USB escape char
#define CR   0xD    // Carriage return
#define LF   0xA    // Newline/linefeed
#define PLUS 0x2B   // '+' character


/***** Serial/debug port *****/
#ifdef AR_SERIAL_ENABLE
  extern Stream& dataStream;
#endif

#ifdef DB_SERIAL_ENABLE
  extern Stream& debugStream;
#endif



/********************************/
/***** GPIB CLASS FUNCTIONS *****/
/***** vvvvvvvvvvvvvvvvvvvv *****/


/********** PUBLIC FUNCTIONS **********/



/***** Class constructor *****/
GPIBbus::GPIBbus(){
  // Set default values ({'\0'} sets version string array to null)
  cfg = { 0, 0, 1, 1, 1, 0, 0, 0, 0, 1200, 0, {'\0'}, 0, {'\0'}, 0, 0 };
  cstate = 0;
  deviceAddressedState = DIDS;
}


/***** Initialise the GPIB bus *****/
void GPIBbus::begin(){
    // Stop current mode
  stop();
  delayMicroseconds(200); // Allow settling time
  // Start device mode
  cfg.cmode = 1;
  // Set GPIB control bus to device idle mode
//  setControls(DINI);
  setControls(DIDS);
  deviceAddressedState = DIDS;
  // Initialise GPIB data lines (sets to INPUT_PULLUP)
  readyGpibDbus();
}


/***** Stops active mode and brings control and data bus to inactive state *****/
void GPIBbus::stop(){
  cstate = 0;
  // Set control bus to idle state (all lines input_pullup)
  // Input_pullup
  setGpibState(0b00000000, 0b11111111, 1);
  // All lines HIGH
  setGpibState(0b11111111, 0b11111111, 0);
  // Set data bus to default state (all lines input_pullup)
  readyGpibDbus();
}


/***** Detect selected pin state *****/
bool GPIBbus::isAsserted(uint8_t gpibsig){
  bool asserted = (getGpibPinState(gpibsig)==LOW) ? true : false;
  if (gpibsig==ATN) atnStatus = asserted;   // Save ATN status
  // Use digitalRead function to get current Arduino pin state
  return asserted;
}


/***** Send the device status byte *****/
void GPIBbus::sendStatus() {
  // Have been addressed and polled so send the status byte
  if (!(cstate==DTAS)) setControls(DTAS);
  writeByte(cfg.stat, true);
  setControls(DIDS);
  // Clear the SRQ bit
  cfg.stat = cfg.stat & ~0x40;
  // De-assert the SRQ signal
  clrSrqSig();
}


/***** Set the status byte *****/
void GPIBbus::setStatus(uint8_t statusByte){
  cfg.stat = statusByte;
  if (statusByte & 0x40) {
    // If SRQ bit is set then assert the SRQ signal
    setSrqSig();
  } else {
    // If SRQ bit is NOT set then de-assert the SRQ signal
    clrSrqSig();
  }
}


/***** Send EOI signal *****/
void GPIBbus::sendEOI(){
  setGpibState(0b00010000, 0b00010000, 1);
  setGpibState(0b00000000, 0b00010000, 0);
  delayMicroseconds(40);
  setGpibState(0b00000000, 0b00010000, 1);
  setGpibState(0b00010000, 0b00010000, 0);
}


/***** Receive data from the GPIB bus ****/
/*
 * Readbreak:
 * 7 - command received via serial
 */

bool GPIBbus::receiveData(Stream& dataStream, bool detectEoi, bool detectEndByte, uint8_t endByte) {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  uint8_t eor = cfg.eor&7;
  int x = 0;
  bool readWithEoi = false;
  bool eoiDetected = false;

  endByte = endByte;  // meaningless but defeats vcompiler warning!

  // Reset transmission break flag
  txBreak = 0;

  // EOI detection required ?
  if (cfg.eoi || detectEoi || (cfg.eor==7)) readWithEoi = true;    // Use EOI as terminator

  // Set GPIB controls for reading in Device mode
  setControls(DLAS);
  
#ifdef DEBUG_GPIBbus_READ
  debugStream.println(F("gpibReceiveData: Start listen ->"));
  debugStream.println(F("Before loop flags:"));
  debugStream.print(F("TRNb: "));
  debugStream.println(txBreak);
  debugStream.print(F("rEOI: "));
  debugStream.println(readWithEoi);
  debugStream.print(F("ATN:  "));
  debugStream.println(isAsserted(ATN) ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (txBreak) break;

    // ATN asserted
    if (isAsserted(ATN)) break;

    // Read the next character on the GPIB bus
    r = readByte(&bytes[0], readWithEoi, &eoiDetected);

    // If ATN asserted then break here
    if (isAsserted(ATN)) break;

#ifdef DEBUG_GPIBbus_READ
    debugStream.print(bytes[0], HEX), debugStream.print(' ');
#else
    // Output the character to the serial port
    dataStream.print((char)bytes[0]);
#endif

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (readWithEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (detectEndByte) {
        if (r == endByte) break;
      }else{
        if (isTerminatorDetected(bytes, eor)) break;
      }
    }

    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG_GPIBbus_READ
  debugStream.println();
  debugStream.println(F("After loop flags:"));
  debugStream.print(F("ATN: "));
  debugStream.println(isAsserted(ATN));
  debugStream.print(F("TMO:  "));
  debugStream.println(r);
  debugStream.print(F("Bytes read: "));
  debugStream.println(x);
  debugStream.println(F("GPIBbus::receiveData: <- End listen."));
#endif

  // Detected that EOI has been asserted
  if (eoiDetected) {
#ifdef DEBUG_GPIBbus_RECEIVE
    dataStream.println(F("GPIBbus::receiveData: EOI detected!"));
#endif
    // If eot_enabled then add EOT character
    if (cfg.eot_en) dataStream.print(cfg.eot_ch);
  }

  // Verbose timeout error
#ifdef DEBUG_GPIBbus_RECEIVE
  if (r > 0) {
    debugStream.println(F("GPIBbus::receiveData: Timeout waiting for sender!"));
    debugStream.println(F("GPIBbus::receiveData: Timeout waiting for transfer to complete!"));
  }
#endif

  // Set device back to idle state
//  setControls(DIDS);

#ifdef DEBUG_GPIBbus_READ
  debugStream.println(F("<- End listen."));
#endif

  // Reset break flag
  if (txBreak) txBreak = false;

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("GPIBbus::receiveData: done."));
#endif

  if (r > 0) return ERR;

  return OK;

}


/***** Send a series of characters as data to the GPIB bus *****/
void GPIBbus::sendData(char *databuffer, size_t dsize, bool lastChunk) {

  uint8_t err = 0;
  const size_t lastbyte = dsize - 1;

  if (cstate != DTAS) setControls(DTAS);

#ifdef DEBUG_GPIBbus_SEND
  debugStream.println(F("Set write data mode."));
  debugStream.print(F("Send->"));
#endif

  // Write the data string
  for (size_t i=0; i<dsize; i++) {
    
    // If ATN asserted then drop out of loop
//    if (isAsserted(ATN)) return;

    // If EOI asserting is on
    if (cfg.eoi) {
      // If EOI asserting is on
      if ((lastChunk) && (i == lastbyte)) {
//debugStream.println(F("Send EOI"));
        // Send character with EOI (if enabled)
        err = writeByte(databuffer[i], true);
      }else{
//debugStream.println(F("Send"));
        // Send character
        err = writeByte(databuffer[i], false);
      }
    } else {
      // Otherwise ignore non-escaped CR, LF and ESC
      if ((databuffer[i] != CR) || (databuffer[i] != LF) || (databuffer[i] != ESC)) err = writeByte(databuffer[i], false);
    }
#ifdef DEBUG_GPIBbus_SEND
    debugStream.print(databuffer[i]);
#endif
    if (err) break;
  }

#ifdef DEBUG_GPIBbus_SEND
  debugStream.println("<-End.");
  if (err) {
    debugStream.print("sendData: Error: ");
    debugStream.println(err);
  }
#endif

  if (!err  && !cfg.eoi) {
    // Write terminators according to EOS setting
    // Do we need to write a CR?
    if ((cfg.eos & 0x2) == 0) {
      writeByte(CR, false);
#ifdef DEBUG_GPIBbus_SEND
      debugStream.println(F("Appended CR"));
#endif
    }
    // Do we need to write an LF?
    if ((cfg.eos & 0x1) == 0) {
      writeByte(LF, false);
#ifdef DEBUG_GPIBbus_SEND
      debugStream.println(F("Appended LF"));
#endif
    }
  }

  if (lastChunk){
    if (lastChunk) setControls(DIDS);
    readyGpibDbus();
  }

#ifdef DEBUG_GPIBbus_SEND
    debugStream.println(F("<- End of send."));
#endif
 
}



/**************************************************/
/***** FUCTIONS TO READ/WRITE DATA TO STORAGE *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv *****/

#ifdef EN_STORAGE


/***** Receive parameters from the GPIB bus ****/
/*
 * Readbreak:
 * 7 - command received via serial
 */
uint8_t GPIBbus::receiveParams(bool detectEoi, char * receiveBuffer, uint8_t bufSize) {

  uint8_t r = 0; //, db;
  uint8_t bytes[3] = {0};
  int x = 0;
//  bool readWithEoi = false;
  bool eoiDetected = false;
//  uint8_t pos = 0;
  uint8_t savedstate = cstate;

  // Reset transmission break flag
  txBreak = 0;

  // EOI detection required ?
//  if (detectEoi) readWithEoi = true;    // Detect EOI as terminator

  // Set GPIB controls to device read mode
  setControls(DLAS);
//  readWithEoi = true;  // In device mode we read with EOI by default
  
#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("gpibReceiveParams: Start listen ->"));
  debugStream.println(F("Before loop flags:"));
  debugStream.print(F("TRNb: "));
  debugStream.println(txBreak);
  debugStream.print(F("rEOI: "));
//  debugStream.println(readWithEoi);
  debugStream.print(F("ATN:  "));
  debugStream.println(isAsserted(ATN) ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
//  while ((r == 0) && (x < bufSize)) {
  while (x < bufSize) {

    // Tranbreak > 0 indicates break condition
    if (txBreak) break;

    // ATN asserted
    if (isAsserted(ATN)) break;

    // Read the next character on the GPIB bus
    r = readByte(&bytes[0], detectEoi, &eoiDetected);
    if (r==0) {
      receiveBuffer[x] = bytes[0];
      x++;
    }else{
      break;
    }

    // If ATN asserted then break here
//    if (isAsserted(ATN)) break; // R will be 2
//    if (r==2) break;  // ATN asserted

#ifdef DEBUG_GPIBbus_RECEIVE
    debugStream.print(bytes[0], HEX), debugStream.print(' ');
#endif

    // Byte counter
//    x++;

    // EOI detection enabled and EOI detected?
    if (detectEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (isTerminatorDetected(bytes, cfg.eor)) break;
    }
    
    // Stop on timeout
//    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println();
  debugStream.println(F("After loop flags:"));
  if (r==2) debugStream.println(F("ATN asserted"));
  debugStream.print(F("ATN:  "));
  debugStream.println(isAsserted(ATN) ? 1 : 0);
  debugStream.print(F("Err:  "));
  debugStream.println(r);
  debugStream.print(F("Bytes read: "));
  debugStream.println(x);
  debugStream.println(F("GPIBbus::receiveParams: <- End listen."));
#endif

  // Detected that EOI has been asserted
  if (eoiDetected) {
#ifdef DEBUG_GPIBbus_RECEIVE
    dataStream.println(F("GPIBbus::receiveParams: EOI detected!"));
#endif
    // If eot_enabled then add EOT character
//    if (cfg.eot_en) dataStream.print(cfg.eot_ch);
  }

  // Verbose timeout error
#ifdef DEBUG_GPIBbus_RECEIVE
  if (r > 0) {
    debugStream.println(F("GPIBbus::receiveParams: Timeout waiting for sender!"));
    debugStream.println(F("GPIBbus::receiveParams: Timeout waiting for transfer to complete!"));
  }
#endif

  // Set device back to previous state
  setControls(savedstate);

  // Reset break flag
  if (txBreak) txBreak = false;

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("GPIBbus::receiveParams: done."));
#endif

  return x;

}


bool GPIBbus::receiveToFile(File& outputFile, bool detectEoi, bool detectEndByte, uint8_t endByte){

  uint8_t r = 0;  // GPIB bus read result
  int s = 0;  // SD card file write result
  uint8_t bytes[3] = {0};
  uint8_t eor = cfg.eor&7;
  int x = 0;
  bool readWithEoi = false;
  bool eoiDetected = false;

  endByte = endByte;  // meaningless but defeats compiler warning!

  // Reset transmission break flag
  txBreak = 0;

  // EOI detection required ?
  if (cfg.eoi || detectEoi || (cfg.eor==7)) readWithEoi = true;    // Use EOI as terminator

  // Set GPIB controls to device read mode
  if (!dataContinuity) setControls(DLAS);
  readWithEoi = true;  // In device mode we read with EOI by default
  
#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("gpibReceiveToFile: Start listen ->"));
  debugStream.println(F("Before loop flags:"));
  debugStream.print(F("TRNb: "));
  debugStream.println(txBreak);
  debugStream.print(F("rEOI: "));
  debugStream.println(readWithEoi);
  debugStream.print(F("ATN:  "));
  debugStream.println(isAsserted(ATN) ? 1 : 0);
#endif

  // Ready the data bus
  readyGpibDbus();

  // Perform read of data (r=0: data read OK; r>0: GPIB read error);
  while (r == 0) {

    // Tranbreak > 0 indicates break condition
    if (txBreak) break;

    // ATN asserted
    if (isAsserted(ATN)) break;

    // Read the next character on the GPIB bus
    r = readByte(&bytes[0], readWithEoi, &eoiDetected);

    // If ATN asserted then break here
    if (isAsserted(ATN)) break;

#ifdef DEBUG_GPIBbus_RECEIVE
    debugStream.print(bytes[0], HEX), debugStream.print(' ');
#endif
    // Write the character to the file
    s = outputFile.write(bytes[0]);
    // Stop on write error
    if (s < 1) {
      r = 5;
      break;
    }

    // Byte counter
    x++;

    // EOI detection enabled and EOI detected?
    if (readWithEoi) {
      if (eoiDetected) break;
    }else{
      // Has a termination sequence been found ?
      if (detectEndByte) {
        if (r == endByte) break;
      }else{
        if (isTerminatorDetected(bytes, eor)) break;
      }
    }
    
    // Stop on timeout
    if (r > 0) break;

    // Shift last three bytes in memory
    bytes[2] = bytes[1];
    bytes[1] = bytes[0];
  }

  if (s) outputFile.sync();

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println();
  debugStream.println(F("After loop flags:"));
  debugStream.print(F("ATN: "));
  debugStream.println(isAsserted(ATN));
  debugStream.print(F("TMO:  "));
  debugStream.println(r);
  debugStream.print(F("Bytes read: "));
  debugStream.println(x);
  debugStream.println(F("GPIBbus::receiveToFile: <- End listen."));
#endif

  // Detected that EOI has been asserted
  if (eoiDetected) {
#ifdef DEBUG_GPIBbus_RECEIVE
    dataStream.println(F("GPIBbus::receiveToFile: EOI detected!"));
#endif
    // If eot_enabled then add EOT character
    if (cfg.eot_en) dataStream.print(cfg.eot_ch);
  }

  // Verbose timeout error
#ifdef DEBUG_GPIBbus_RECEIVE
  if (r > 0 && r < 3) {
    debugStream.println(F("GPIBbus::receiveToFile: Timeout waiting for sender!"));
    debugStream.println(F("GPIBbus::receiveToFile: Timeout waiting for transfer to complete!"));
  }
#endif

  // File write error
#ifdef DEBUG_GPIBbus_RECEIVE
  if (r == 5) debugStream.println(F("GPIBbus::receiveToFile: failed to write to file!"));
#endif

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("<- End listen."));
#endif

  // Reset break flag
  if (txBreak) txBreak = false;

#ifdef DEBUG_GPIBbus_RECEIVE
  debugStream.println(F("GPIBbus::receiveToFile: done."));
#endif

  if (r > 0) return ERR;

  return OK;
       
}


#endif

/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** FUCTIONS TO READ/WRITE DATA TO STORAGE *****/
/**************************************************/



/***** Signal to break a GPIB transmission *****/
void GPIBbus::signalBreak(){
  txBreak = true;
}


/***** Control the GPIB bus - set various GPIB states *****/
/*
 * state is a predefined state (CINI, CIDS, CCMS, CLAS, CTAS, DINI, DIDS, DLAS, DTAS);
 * Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC
 * setGpibState byte1 (databits) : State - 0=LOW, 1=HIGH/INPUT_PULLUP; Direction - 0=input, 1=output;
 * setGpibState byte2 (mask)     : 0=unaffected, 1=enabled
 * setGpibState byte3 (mode)     : 0=set pin state, 1=set pin direction
 */
void GPIBbus::setControls(uint8_t state) {

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
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Initialised GPIB control mode"));
#endif
      break;

    case CIDS:  // Controller idle state
      setGpibState(0b10111000, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB lines to idle state"));
#endif
      break;

    case CCMS:  // Controller active - send commands
      setGpibState(0b10111001, 0b10011111, 1);
      setGpibState(0b01011111, 0b10011111, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB lines for sending a command"));
#endif
      break;

    case CLAS:  // Controller - read data bus
      // Set state for receiving data
      setGpibState(0b10100110, 0b10011110, 1);
      setGpibState(0b11011000, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB lines for reading data"));
#endif
      break;

    case CTAS:  // Controller - write data bus
      setGpibState(0b10111001, 0b10011110, 1);
      setGpibState(0b11011111, 0b10011110, 0);
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB lines for writing data"));
#endif
      break;
*/
    /* Bits control lines as follows: 8-ATN, 7-SRQ, 6-REN, 5-EOI, 4-DAV, 3-NRFD, 2-NDAC, 1-IFC */

    // Listener states
    case DIDS:  // Listner initialisation
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
      // Set data bus to input pullup
      readyGpibDbus();
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB signals IDLE state"));
#endif
      break;

    case DINI:  // Device idle state
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
      setGpibState(0b00000000, 0b00001110, 1);
      setGpibState(0b11111111, 0b00001110, 0);
      deviceAddressedState = DIDS;
      // Set data bus to input pullup
      readyGpibDbus();
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB signals to idle state"));
#endif
      break;

    case DLAS:  // Device listner active (actively listening - can handshake)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,LOW);
#endif      
      setGpibState(0b00000110, 0b00011110, 1);
      setGpibState(0b11111001, 0b00011110, 0);
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB signals to LISTEN state"));
#endif
      break;

    case DTAS:  // Device talker active (sending data)
#ifdef SN7516X
      digitalWrite(SN7516X_TE,HIGH);
#endif      
      setGpibState(0b00011000, 0b00011110, 1);
      setGpibState(0b11111001, 0b00011110, 0);
#ifdef GPIBbus_CONTROL
      debugStream.println(F("Set GPIB signals to TALK state"));
#endif
      break;
#ifdef GPIBbus_CONTROL
    default:
      // Should never get here!
      debugStream.println(F("Unknown GPIB state requested!"));
#endif
  }

  // Save state
  cstate = state;

  // GPIB bus delay (to allow state to settle)
//  delayMicroseconds(AR488.tmbus);

}


/***** Set GPI control state using numeric input (xdiag_h) *****/
void GPIBbus::setControlVal(uint8_t value, uint8_t mask, uint8_t mode){
  setGpibState(value, mask, mode);
}


/***** Set GPIB data bus to specific value (xdiag_h) *****/
void GPIBbus::setDataVal(uint8_t value){
  setGpibDbus(value);
}


/***** Flag more data to come - suppress addressing *****/
/*
void GPIBbus::setDataContinuity(bool flag){
  dataContinuity = flag;
}
*/

/***** Write a single byte using handshaking *****/
/*
 * Returns: 
 * 0 : write succeeded
 * 1 : Sender requested stop/abort
 * 2 : ATN asserted during write
 * 3 : NDAC/NRFD protocol error
 */
/*
uint8_t GPIBbus::writeByte(uint8_t db, bool isLastByte) {

  // If IFC has been asserted then abort
  if (isAsserted(IFC)) {
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("GPIBbus::writeByte: detected interface clear [IFC]"));
#endif
    return 1;    
  }

  // Wait for NDAC to go LOW (indicating that devices are at attention)
  if (waitOnPinState(LOW, NDAC, cfg.rtmo)) {
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("GPIBbus::writeByte: timeout waiting for receiver attention [NDAC asserted]"));
#endif
    return 2;
  }

  // Wait for NRFD to go HIGH (indicating that receiver is ready)
  if (waitOnPinState(HIGH, NRFD, cfg.rtmo))  {
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("gpibBus::writeByte: timeout waiting for receiver ready - [NRFD unasserted]"));
#endif
    return 3;
  }
*/
/*
  // If NDAC (and NRFD) are high receiver has asked us to stop transmitting
  if (!isAsserted(NDAC)){
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("gpibBus::writeByte: stop requested!"));
#endif    
    return 1;
  }
*/
/*
  // If ATN has been asserted we need to abort and listen
  if (isAsserted(ATN)) {
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("gpibBus::writeByte: ATN detected!"));
#endif    
    return 4;
  }


  // Place data on the bus
  setGpibDbus(db);

  if (cfg.eoi && isLastByte) {
    // If EOI enabled and this is the last byte then assert DAV and EOI
#ifdef DEBUG_GPIBbus_SEND
    debugStream.println(F("Asserting EOI..."));    
#endif
    setGpibState(0b00000000, 0b00011000, 0);
  }else{
    // Assert DAV (data is valid - ready to collect)
    setGpibState(0b00000000, 0b00001000, 0);
  }

  // Wait for NRFD to go LOW (receiver accepting data)
  if (waitOnPinState(LOW, NRFD, cfg.rtmo))  {
#ifdef DEBUG_GPIBbus_SEND    
    dataStream.println(F("gpibBus::writeByte: timeout waiting for data to be accepted - [NRFD asserted]"));
#endif
    return 5;
  }

  // Wait for NDAC to go HIGH (data accepted)
  if (waitOnPinState(HIGH, NDAC, cfg.rtmo))  {
#ifdef DEBUG_GPIBbus_SEND
    dataStream.println(F("gpibBus::writeByte: timeout waiting for data accepted signal - [NDAC unasserted]"));
#endif
    return 6;
  }

  if (cfg.eoi && isLastByte) {
    // If EOI enabled and this is the last byte then un-assert both DAV and EOI
    if (cfg.eoi && isLastByte) setGpibState(0b00011000, 0b00011000, 0);
  }else{
    // Unassert DAV
    setGpibState(0b00001000, 0b00001000, 0);
  }
  
  // Reset the data bus
  setGpibDbus(0);

  return 0;
}
*/


uint8_t GPIBbus::writeByte(uint8_t db, bool isLastByte) {
  unsigned long startMillis = millis();
  unsigned long currentMillis = startMillis + 1;
  const unsigned long timeval = cfg.rtmo;
  uint8_t stage = 4;

  // Wait for interval to expire
  while ( (unsigned long)(currentMillis - startMillis) < timeval ) {

    if (cfg.cmode == 1) {
      // If IFC has been asserted then abort
      if (isAsserted(IFC)) {
        setControls(DLAS);       
#ifdef DEBUG_GPIBbus_SEND
        debugStream.println(F("GPIBbus::writeByte: IFC detected!"));
#endif
        stage = 1;
        break;    
      }

      // If ATN has been asserted we need to abort and listen
      if (isAsserted(ATN)) {
        setControls(DLAS);       
#ifdef DEBUG_GPIBbus_SEND
        debugStream.println(F("gpibBus::writeByte: ATN detected!"));
#endif
        stage = 2;
        break;
      }
    }

    // Wait for NDAC to go LOW (indicating that devices (stage==4) || (stage==8) ) are at attention)
    if (stage == 4) {
      if (digitalRead(NDAC) == LOW) stage = 5;
    }

    // Wait for NRFD to go HIGH (indicating that receiver is ready)
    if (stage == 5) {
      if (digitalRead(NRFD) == HIGH) stage = 6;
    }

    if (stage == 6){
      // Place data on the bus
      setGpibDbus(db);
      if (cfg.eoi && isLastByte) {
        // If EOI enabled and this is the last byte then assert DAV and EOI
#ifdef DEBUG_GPIBbus_SEND
        debugStream.println(F("Asserting EOI..."));    
#endif
        setGpibState(0b00000000, 0b00011000, 0);
      }else{
        // Assert DAV (data is valid - ready to collect)
        setGpibState(0b00000000, 0b00001000, 0);
      }
      stage = 7;
    }

    if (stage == 7) {
      // Wait for NRFD to go LOW (receiver accepting data)
      if (digitalRead(NRFD) == LOW) stage = 8;
    }

    if (stage == 8) {
      // Wait for NDAC to go HIGH (data accepted)
      if (digitalRead(NDAC) == HIGH) {
        stage = 9;
        break;
      }
    }

    // Increment time
    currentMillis = millis();

  }

  // Handshake complete
  if (stage == 9) {
    if (cfg.eoi && isLastByte) {
      // If EOI enabled and this is the last byte then un-assert both DAV and EOI
      if (cfg.eoi && isLastByte) setGpibState(0b00011000, 0b00011000, 0);
    }else{
      // Unassert DAV
      setGpibState(0b00001000, 0b00001000, 0);
    }
    // Reset the data bus
    setGpibDbus(0);
    return 0;
  }

  // Otherwise timeout or ATN/IFC return stage at which it ocurred
#ifdef DEBUG_GPIBbus_SEND
  switch (stage) {
    case 4:
      debugStream.print("writeByte: NDAC timeout!");
      break;
    case 5:
      debugStream.print("writeByte: NRFD timout!");
      break;
    case 7:
      debugStream.print("writeByte: NRFD timout!");
      break;
    case 8:
      debugStream.print("writeByte: NDAC timout!");
      break;
    default:
      debugStream.print(F("writeByte: Error: "));
      debugStream.println(stage);
  }
#endif

  return stage;
}


/***** Set device addressing state *****/
/*
 * NOTE: this is a flag. It does not set the state of the GPIB controls
 */
/*
void GPIBbus::setDeviceAddressedState(uint8_t state){
  // Valid state supplied
  if (state==DLAS || state==DTAS) {
    deviceAddressedState = state;
    return;
  }
  // Otherwise set to idle
  deviceAddressedState = DIDS;
}
*/

/***** Device is addressed to listen? *****/
bool GPIBbus::isDeviceAddressedToListen(){
//  if (deviceAddressedState == DLAS) return true;
  if (cstate == DLAS) return true;
  return false;
}


/***** Device is addressed to talk? *****/
bool GPIBbus::isDeviceAddressedToTalk(){
//  if (deviceAddressedState == DTAS) return true;
  if (cstate == DTAS) return true;
  return false;
}


/***** Device is not addressed? *****/
bool GPIBbus::isDeviceNotAddressed(){
//  if (deviceAddressedState == DIDS) return true;
//  if ( (cstate == DIDS)  || (cstate == DINI) ) return true;
  if (cstate == DIDS) return true;
  return false;
}




/********** PRIVATE FUNCTIONS **********/


/***** Read a SINGLE BYTE of data from the GPIB bus using 3-way handshake *****/
/*
 * (- this function is called in a loop to read data    )
 * (- the GPIB bus must already be configured to listen )
 */
/*
uint8_t GPIBbus::readByte(uint8_t *db, bool readWithEoi, bool *eoi) {
  bool atnStat = isAsserted(ATN);
  *eoi = false;

  // Verify that IFC has not been asserted. If it has then abort
  if (isAsserted(IFC)) {
#ifdef DEBUG_GPIBbus_RECEIVE
    dataStream.println(F("GPIBbus::readByte: detected interface clear [IFC]"));
#endif
    return 4;    
  }

  // Unassert NRFD (we are ready for more data)
  setGpibState(0b00000100, 0b00000100, 0);

  // ATN asserted and just got unasserted - abort - we are not ready yet
  if (atnStat && (!isAsserted(ATN))) {
    setGpibState(0b00000000, 0b00000100, 0);
    return 3;
  }

  // Wait for DAV to go LOW indicating talker has finished setting data lines..
  if (waitOnPinState(LOW, DAV, cfg.rtmo))  {
#ifdef DEBUG_GPIBbus_RECEIVE
    debugStream.println(F("GPIBbus::readByte: timeout waiting for DAV to go LOW"));
#endif
    setGpibState(0b00000000, 0b00000100, 0);
    // No more data for you?
    return 1;
  }

  // Assert NRFD (NOT ready - busy reading data)
  setGpibState(0b00000000, 0b00000100, 0);

  // Check for EOI signal
  if (readWithEoi && isAsserted(EOI)) *eoi = true;

  // read from DIO
  *db = readGpibDbus();

  // Unassert NDAC signalling data accepted
  setGpibState(0b00000010, 0b00000010, 0);

  // Wait for DAV to go HIGH indicating data no longer valid (i.e. transfer complete)
  if (waitOnPinState(HIGH, DAV, cfg.rtmo))  {
#ifdef DEBUG_GPIBbus_RECEIVE
    debugStream.println(F("GPIBbus::readByte: timeout waiting DAV to go HIGH"));
#endif
    return 2;
  }

  // Re-assert NDAC - handshake complete, ready to accept data again
  setGpibState(0b00000000, 0b00000010, 0);

  // GPIB bus DELAY
//  delayMicroseconds(cfg.tmbus);

  return 0;
}
*/


uint8_t GPIBbus::readByte(uint8_t *db, bool readWithEoi, bool *eoi) {

  unsigned long startMillis = millis();
  unsigned long currentMillis = startMillis + 1;
  const unsigned long timeval = cfg.rtmo;
  uint8_t stage = 4;

  bool atnStat = atnStatus; // Save ATN status on entry
  *eoi = false;

  // Wait for interval to expire
  while ( (unsigned long)(currentMillis - startMillis) < timeval ) {

    if (cfg.cmode == 1) {
      if (isAsserted(IFC)) {
        // If IFC has been asserted then abort
#ifdef DEBUG_GPIBbus_RECEIVE
        dataStream.println(F("GPIBbus::readByte: IFC detected]"));
#endif
        stage = 1;
        break;    
      }

      if (atnStat != isAsserted(ATN)) { // Check saved status against current ATN status
        // ATN unasserted during handshake - not ready for data yet or ATN requested so abort (and exit ATN loop)
        stage = 2;
        break;
      }
    }  

    if (stage == 4) {
      // Unassert NRFD (we are ready for more data)
      setGpibState(0b00000100, 0b00000100, 0);
      stage = 6;
    }

    if (stage == 6) {
      // Wait for DAV to go LOW indicating talker has finished setting data lines..
      if (digitalRead(DAV) == LOW) {
        // Assert NRFD (Busy reading data)
        setGpibState(0b00000000, 0b00000100, 0);
        stage = 7;
      }
    }

    if (stage == 7) {
      // Check for EOI signal
      if (readWithEoi && isAsserted(EOI)) *eoi = true;
      // read from DIO
      *db = readGpibDbus();
      // Unassert NDAC signalling data accepted
      setGpibState(0b00000010, 0b00000010, 0);
      stage = 8;
    }

    if (stage == 8) {
      // Wait for DAV to go HIGH indicating data no longer valid (i.e. transfer complete)
      if (digitalRead(DAV) == HIGH) {
        // Re-assert NDAC - handshake complete, ready to accept data again
        setGpibState(0b00000000, 0b00000010, 0);
        stage = 9;
        break;     
      }
    }

    // Increment time
    currentMillis = millis();

  }

  // Completed
  if (stage == 9) return 0;

  if (stage==1) return 4;
  if (stage==2) return 3;
  
  // Otherwise return stage
#ifdef DEBUG_GPIBbus_RECEIVE
  if ( (stage==4) || (stage==8) ) {
    debugStream.println(F("readByte: DAV timout!"));
  }else{
    debugStream.print(F("readByte: Error: "));
    debugStream.println(stage);
  }
#endif

  return stage;

}



/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
/*
 * Note: without millis() - using millis() in here breaks the protocol for longer 
 *        transmissions. Reason unknown. 
 */
/*
boolean GPIBbus::waitOnPinState(uint8_t state, uint8_t pin, const uint16_t interval) {

  unsigned long timeout = interval;
  timeout = timeout * 1000;   // Convert milliseconds to microseconds

  bool atnStat = (isAsserted(ATN));
  unsigned long clk = 0;
  
  while (digitalRead(pin) != state) {
    // Check timer
    if (clk >= timeout) return true;
*/
/*
    // ATN changed state so abort
    if (atnStat != isAsserted(ATN)) {
#if defined (DEBUG_GPIBbus_RECEIVE) || defined (DEBUG_GPIBbus_SEND)
      if (!atnStat) setControls(DLAS);  // If ATN asserted imediately go into listen mode
      debugStream.println(F("ATN status changed!"));
#endif
      return true;
    }
*/
/*
    delayMicroseconds(100);
    clk = clk + 100;
  }
  return false;        // = no timeout therefore succeeded!
}
*/


/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
/*
 * Note: with millis() - experimental (rollover = 49 days)
 */
/*
boolean GPIBbus::waitOnPinState(uint8_t state, uint8_t pin, uint16_t interval) {
  unsigned long startMillis = millis();
  unsigned long currentMillis = startMillis + 1;
    
  // Wait for interval to expire
  while ( (unsigned long)(currentMillis - startMillis) < interval ) {
    // Check condition
    if (digitalRead(pin) == state) {
//      debugStream.print(F("Elapsed: "));
//      debugStream.println( (unsigned long)(currentMillis - startMillis) );
      return false;  // Condition met.
    }
    // Grab the current time
    currentMillis = millis();
  }
//  debugStream.print(F("Timeout!"));
  return true;  // Timeout!
}
*/


/***** Wait for "pin" to reach a specific state *****/
/*
 * Returns false on success, true on timeout.
 * Pin MUST be set as INPUT_PULLUP otherwise it will not change and simply time out!
 */
/*
   * Note: with micros() - experimental (rollover = 70 minutes)
 */
/*
boolean GPIBbus::waitOnPinState(uint8_t state, uint8_t pin, const unsigned long interval) {
  const unsigned long startMicros = micros();           // Snapshot of time the function started
  unsigned long currentMicros = startMicros + 1;  // Initialise the current time value
  const unsigned long timeval = interval * 1000;  // Convert milliseconds to microseconds
  bool timeout = true;

//      debugStream.print(F("Checking pin: "));
//      debugStream.println(pin);
 
  // Wait for interval to expire
  while ( (unsigned long)(currentMicros - startMicros) < timeval ) {
    // Check condition
    if (digitalRead(pin) == state) {
//      debugStream.print(F("Elapsed: "));
//      debugStream.println( (unsigned long)(currentMicros - startMicros) );
      timeout = false;  // Condition met.
      break;
    }
    // Grab the current time
    currentMicros = micros();
  }

//  timeout ? debugStream.println(F("Timeout!")) : debugStream.println(F("State matched."));
//  debugStream.print(F("Timeout!"));
  return timeout;  // Timeout!
}
*/

/***** Check for terminator *****/
bool GPIBbus::isTerminatorDetected(uint8_t bytes[3], uint8_t eorSequence){
  // Look for specified terminator (CR+LF by default)
  switch (eorSequence) {
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
  return false;
}


/***** Unaddress device *****/
/*
bool GPIBbus::unAddressDevice() {
  // De-bounce
  delayMicroseconds(30);
  // Utalk/unlisten
  if (sendCmd(GC_UNL)) return ERR;
  if (sendCmd(GC_UNT)) return ERR;
  return OK;
}
*/


/***** Untalk bus then address a device *****/
/*
 * talk: false=listen; true=talk;
 */
 /*
bool GPIBbus::addressDevice(uint8_t addr, bool talk) {
//  uint8_t saddr = 0;
  if (sendCmd(GC_UNL)) return ERR;
  if (talk) {
    // Device to talk, controller to listen
    if (sendCmd(GC_TAD + addr)) return ERR;
    if (sendCmd(GC_LAD + cfg.caddr)) return ERR;
  } else {
    // Device to listen, controller to talk
    if (sendCmd(GC_LAD + addr)) return ERR;
    if (sendCmd(GC_TAD + cfg.caddr)) return ERR;
  }
  // Send secondary address if available
//  if (cfg.saddr){
//    saddr = cfg.saddr;
//    cfg.saddr = 0;  // Clear the address for next "command"
//    if (sendCmd(saddr)) return ERR;
//  }

  return OK;
}
*/


/***** Set the SRQ signal *****/
void GPIBbus::setSrqSig() {
  // Set SRQ line to OUTPUT HIGH (asserted)
  setGpibState(0b01000000, 0b01000000, 1);
  setGpibState(0b00000000, 0b01000000, 0);
}


/***** Clear the SRQ signal *****/
void GPIBbus::clrSrqSig() {
  // Set SRQ line to INPUT_PULLUP (un-asserted)
  setGpibState(0b00000000, 0b01000000, 1);
  setGpibState(0b01000000, 0b01000000, 0);
}


/***** ^^^^^^^^^^^^^^^^^^^^ *****/
/***** GPIB CLASS FUNCTIONS *****/
/********************************/
