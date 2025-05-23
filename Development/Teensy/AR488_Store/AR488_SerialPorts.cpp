#include <Arduino.h>
#include "AR488_SerialPorts.h"

/***** AR488_SerialPort.cpp, ver. 0.05.86, 13/07/2022 *****/
/*
 * Serial Port implementation
 */


/***** DEVNULL Library *****
 *  AUTHOR: Rob Tillaart
 *  VERSION: 0.1.5
 *  PURPOSE: Arduino library for a /dev/null stream - useful for testing
 *  URL: https://github.com/RobTillaart/DEVNULL
 */

DEVNULL::DEVNULL()
{
  setTimeout(0);        //  no timeout.
  _bottomLessPit = -1;  //  nothing in the pit
}

int  DEVNULL::available()
{
  return 0;
};

int  DEVNULL::peek()
{
  return EOF;
};

int  DEVNULL::read()
{
  return EOF;
};

//  placeholder to keep CI happy
void DEVNULL::flush()
{
  return;
};

size_t DEVNULL::write(const uint8_t data)
{
  _bottomLessPit = data;
  return 1;
}

size_t DEVNULL::write( const uint8_t *buffer, size_t size)
{
  if (size > 0) _bottomLessPit = buffer[size - 1];
  return size;
}

int DEVNULL::lastByte()
{
  return _bottomLessPit;
}

/***** DEVNULL Library *****/



#ifdef DATAPORT_ENABLE
  #ifdef AR_SERIAL_SWPORT

    SoftwareSerial dataPort(SW_SERIAL_RX_PIN, SW_SERIAL_TX_PIN);

    void startDataPort() {
      dataPort.begin(AR_SERIAL_SPEED);
    }

  
  #else

    Stream& dataPort = AR_SERIAL_PORT;

    void startDataPort() {
      AR_SERIAL_PORT.begin(AR_SERIAL_SPEED);
    }

  
  #endif

#else

  DEVNULL _dndata;
  Stream& dataPort = _dndata;

#endif  // DATAPORT_ENABLE



#ifdef DEBUG_ENABLE
  #ifdef DB_SERIAL_SWPORT

    SoftwareSerial debugPort(SW_SERIAL_RX_PIN, SW_SERIAL_TX_PIN);

    void startDebugPort() {
      debugPort.begin(DB_SERIAL_SPEED);
    }
  
  #else

    Stream& debugPort = DB_SERIAL_PORT;

    void startDebugPort() {
      DB_SERIAL_PORT.begin(DB_SERIAL_SPEED);
    }

  #endif

  void printHex(uint8_t byteval) {
    char x[4] = {'\0'};
    sprintf(x,"%02X ", byteval);
    debugPort.print(x);
  }

  void printHexBuf(char * buf, size_t bsize){
    for (size_t i = 0; i < bsize; i++) {
      printHex(buf[i]);
    }
    debugPort.println();
  }

#else

  DEVNULL _dndebug;
  Stream& debugPort = _dndebug;

#endif  // DEBUG_ENABLE
