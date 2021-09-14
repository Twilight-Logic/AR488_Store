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


/***** AR488_Storage_Tek_4924.h, ver. 0.05.42, 14/09/2021 *****/

// Default chip select pin number is defined on some cards as SDCARD_SS_PIN
// If its not defined and its not been set in config then we use pin 4
#ifndef SDCARD_CS_PIN
  #ifdef SDCARD_SS_PIN
    #define SDCARD_CS_PIN SDCARD_SS_PIN 
  #else
    #define SDCARD_CS_PIN 4
  #endif
#endif

#define SD_CONFIG SdSpiConfig(SDCARD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(SDCARD_CLK))

// Number of storage GPIB commands
#define STGC_SIZE 18
// Length of character stream buffer
#define LINELENGTH 74
// Number of files allowed per directory (virtual "tape")
#define FILES_PER_DIRECTORY 99


#define DATA_CONTINUE false
#define DATA_COMPLETE true



struct alphaIndex {
  const char idx;
  const char * desc;
};



/***** Character stream buffer *****/
class CharStream : public Stream {
  public:
//    CharStream(char *buf, uint8_t dsize) : bufsize(dsize), databuf(buf), tail(0) { }
    CharStream(uint8_t dsize) : bufsize(dsize), databuf(new char[dsize]), tail(0) { }

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


/***** Tektronix File Information class *****/
class TekFileInfo {

  public:
    TekFileInfo();
    void clear();
    uint8_t getFnumVal();
    void getFnumStr(char * numstr);
    char getFtype();
    char getFusage();
    void getFtypeStr(char * typestr);
    void getFusageStr(char * usagestr);
    void getFrecords(char * recordstr);
    void getFsize(char * sizestr);

    void getFilename(char * filename);
    void getTekHeader(char * header);

    void setFromFilename(char * filename);
    bool setFnumber(uint8_t filenum);
    void setFtype(char typechar);
    void setFusage(char usagechar);
    void setFrecords(uint16_t records);
    void setFsize(size_t filesize);
    void setFsecret(bool isSecret);

  private:
 
    uint16_t fsizeToRecords(unsigned long fsize);
    uint8_t fnum;
    char ftype;
    char fusage;
    bool fsecret;
    size_t fsize;
    uint16_t frecords;

};



/***** SD storage class for the Tek 4924 *****/
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
    const uint8_t full_path_size = 60;    // 44 + 13 char plus CR + NULL
    char directory[13] = "/root/";     //allow up to ten character directory names plus two '/' and NULL terminator

    char f_name[46];                //the current filename variable
    char f_type='N';                //the current filetype string variable

    SdFat sd;


//    SdFile rdfile;
//    fstream sdinout;

//    TekFileInfo tekFile;
    File sdinout;
    
    uint8_t listIdx = 0;

    uint8_t errorCode = 0;

    using stgcHandler = void (SDstorage::*)();

    // Storage GPIB command functions
    // STATUS
    void stgc_0x60_h();
    // SAVE
    void stgc_0x61_h();
    // CLOSE
    void stgc_0x62_h();
    // ?
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
    // ?
    void stgc_0x6D_h();
    // READ
    void stgc_0x6E_h();
    // WRITE
    void stgc_0x6F_h();
    // BSAVE / BOLD
    void stgc_0x71_h();
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
    uint16_t hexToDataHeader(char * hexstr);
    bool searchForFile(uint8_t filenum, File& fileObj);
    uint8_t getLastFile(File& fileObj);
    uint8_t binaryRead();
    bool binaryWrite();
    bool renameFile(File& fileObj, char ftype, char fusage);
};


void printHex2(char *buffr, int dsize);

#endif // AR488_STORE_TEK_4924_H
