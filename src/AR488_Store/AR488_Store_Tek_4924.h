#ifndef AR488_STORE_TEK_4924_H
#define AR488_STORE_TEK_4924_H

//#include <BufferedPrint.h>
//#include <FreeStack.h>
//#include <MinimumSerial.h>

#include <SPI.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <sdios.h>
//#include <Stream.h>
#include "string.h"

#include "AR488_Config.h"
#include "AR488_GPIBdevice.h"


/***** AR488_Storage_Tek_4924.h, ver. 0.05.26, 12/07/2021 *****/

// Chip select pin
#ifndef SDCARD_CS_PIN
  #define SDCARD_CS_PIN 4
#endif

#define SD_CONFIG SdSpiConfig(SDCARD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(20))


// Number of storage GPIB commands
#define STGC_SIZE 17
// Length of character stream buffer
#define LINELENGTH 82


#define DATA_CONTINUE false
#define DATA_COMPLETE true


/***** Character stream buffer *****/
class CharStream : public Stream {
  public:
//    CharStream(char *buf, uint8_t dsize) : bufsize(dsize), databuf(buf), tail(0) { }
    CharStream(uint8_t dsize) : databuf(new char[dsize]), bufsize(dsize), tail(0) { }

    // Stream methods
    virtual int available() { return bufsize - tail; }
    virtual int read() { return 0; }
    virtual int peek() { return 0; }
    virtual void flush() { memset(databuf, '\0', bufsize); tail=0; }
    // Print methods
    virtual size_t write(uint8_t c) { databuf[tail] = (char)c; tail++; return 1;}
    uint8_t length() { return tail; }
    char* toCharArray() { return databuf; }

  private:
    uint8_t bufsize;
    char *databuf;
    uint8_t tail;
};

/*
template<class T>
inline Print &operator <<(Print &stream, const T &arg) {stream.print(arg); return stream;}
*/

class SDstorage {

  public:

    // Storage management functions
    SDstorage();
    void storeExecCmd(uint8_t cmd);


  private:

    // create a serial stream
    // Max of 99 files assumed in a single directory
    const uint8_t nMax = 99;              // Maximum file count
    const uint8_t line_buffer_size = 74;  // 72 char line max in Tek plus CR plus NULL
    const uint8_t bin_buffer_size = 65;
    const uint8_t file_header_size = 46;  // 44 char plus CR + NULL
    char directory[13] = "/root/";     //allow up to ten character directory names plus two '/' and NULL terminator

    char f_name[46];                //the current filename variable
    char f_type='N';                //the current filetype string variable

    SdFat sd;
    SdFile rdfile;
    fstream sdinout;

    uint16_t binary_header;         // Header for binary data

    union file_header {
      struct {
        char f_number[7] = {0};
        char f_type[8] = {0};
        char f_usage[5] = {0};
        char f_comment[17] = {0};
        char f_secret[1] = {0};
        char f_size[6] = {0};
        char f_end[2] = {'\r','\0'};
      };
      char f_name[46];   // Total header = 40 characters
    };
    file_header current_header;

    uint8_t errorCode = 0;

    using stgcHandler = void (SDstorage::*)();

    // Storage GPIB command functions
    // STATUS
    void stgc_0x60_h();
    // SAVE
    void stgc_0x61_h();
    // CLOSE
    void stgc_0x62_h();
    void stgc_0x63_h();
    // OLD
    void stgc_0x64_h();
    // TYPE
    void stgc_0x66_h();
    // KILL
    void stgc_0x67_h();
    // HEADER
    void stgc_0x69_h();
    // PRINT
    void stgc_0x6C_h();
    void stgc_0x6D_h();
    // READ
    void stgc_0x6E_h();
    // WRITE
    void stgc_0x6F_h();
    // TLIST
    void stgc_0x73_h();
    // FIND
    void stgc_0x7B_h();
    // MARK
    void stgc_0x7C_h();
    // SECRET
    void stgc_0x7D_h();
    // ERROR
    void stgc_0x7E_h();

    // Storage command function record
    struct storeCmdRec {
      const uint8_t cmdByte;
      stgcHandler handler;
    };

    static storeCmdRec storeCmdHidx[STGC_SIZE];

};


void printHex2(char *buffr, int dsize);

#endif // AR488_STORE_TEK_4924_H
