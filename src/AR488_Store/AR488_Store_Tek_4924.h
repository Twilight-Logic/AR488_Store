#include <BufferedPrint.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <sdios.h>

// libraries added by Monty
#include <SPI.h>
//#include <SD.h>
#include "sdios.h"
#include "string.h"


#ifndef AR488_STORE_TEK_4924_H
#define AR488_STORE_TEK_4924_H


#include <SPI.h>
#include <Stream.h>
#include "AR488_Config.h"
#include "AR488_GPIBdevice.h"


/***** AR488_Storage_Tek_4924.h, ver. 0.05.12, 25/06/2021 *****/

#define SD_CONFIG SdSpiConfig(SDCARD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(20))

// Number of storage GPIB commands
#define STGC_SIZE 17

#define LINELENGTH 82



/*
class GPIBstream : public Stream {

  public:

    GPIBstream(int timeout) : stat(0), rtmo(timeout) { }
  
    virtual int available() { }
    virtual int read() { }
    virtual int peek() { }
    virtual void flush() { }
    // Print methods
    virtual size_t write(uint8_t c) { stat = writeByte(c); return stat;}


  private:

    uint8_t stat;
    int rtmo;
    bool writeByte(uint8_t db);
    bool writeByteHandshake(uint8_t db);
    bool waitOnPinState(uint8_t state, uint8_t pin, int interval);
    bool isTerminatorDetected(uint8_t bytes[3], uint8_t eorSequence);
    bool isAsserted(uint8_t gpibsig);
  
};
*/



class CharStream : public Stream {
  public:
    CharStream(char *buf, uint8_t dsize) : bufsize(dsize), databuf(buf), tail(0) { }

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


class StringStream : public Stream
{
public:
    StringStream(String &s) : string(s), position(0) { }

    // Stream methods
    virtual int available() { return string.length() - position; }
    virtual int read() { return position < string.length() ? string[position++] : -1; }
    virtual int peek() { return position < string.length() ? string[position] : -1; }
    virtual void flush() { }
    // Print methods
    virtual size_t write(uint8_t c) { string += (char)c; return 1;}

private:
    String &string;
//    unsigned int length;
    unsigned int position;
};




class SDstorage {

  public:

    // Storage management functions
    SDstorage();
    void showVolumeInfo();
    bool isInit();
    bool isVolumeMounted();
    void listFiles();
    bool chkTek4924Directory();
    bool chkTapesFile();
    bool selectTape(uint8_t tnum);
    
    void storeExecCmd(uint8_t cmd);

    const size_t stgcSize = 10;
    bool isinit = false;
    bool isvolmounted = false;
    uint8_t currentTapeNum = 1;
    uint8_t currentFileNum = 1;
    char currentTapeName[35] = {'\0'};
    char currentFileName[25] = {'\0'};

    void showSDInfo(Stream& outputStream);
    void showSdVolumeInfo(Stream& outputStream);
    void listSdFiles(Stream& outputStream);
    void listDir(Stream& outputStream, File dir, int numTabs);

/*
template<typename T> void showSDInfo(T* output) {
//  Sd2Card sdcard;
  output->print(F("Card type:\t\t"));
  switch (arSdCard.type()) {
    case SD_CARD_TYPE_SD1:
      output->println("MMC");
      break;
    case SD_CARD_TYPE_SD2:
      output->println("SDSC");
      break;
    case SD_CARD_TYPE_SDHC:
      output->println("SDHC");
      break;
    default:
      output->println("Unknown");
  }
}
*/


/*
template<typename T> void showSdVolumeInfo(T* output) {
//  Sd2Card sdcard;
//  SdVolume sdvolume;
  uint32_t volumesize;

  // Type and size of the first FAT-type volume

  if (arSdVolume.init(arSdCard)){
    output->print("Volume type is:\t\tFAT");
    output->println(arSdVolume.fatType(), DEC);
    output->print("Clusters:\t\t");
    output->println(arSdVolume.clusterCount());
    output->print("Blocks per cluster:\t");
    output->println(arSdVolume.blocksPerCluster());
    output->print("Total blocks:\t\t");
    output->println(arSdVolume.blocksPerCluster() * arSdVolume.clusterCount());

    volumesize = arSdVolume.blocksPerCluster(); // clusters are collections of blocks
    volumesize *= arSdVolume.clusterCount();    // we'll have a lot of clusters
    volumesize /= 2;                          // SD card blocks are always 512 bytes (2 blocks are 1KB)
    volumesize /= 1024;                       // Convert to Mb

    if (volumesize>1024) {
      output->print("Volume size (Gb):\t");
      output->println((float)volumesize/1024.0);      
    }else{
      output->print("Volume size (Mb):\t");
      output->println(volumesize);
    }
  }
}
*/


/*  
template<typename T> void listSdFiles(T* output){
  if (SD.begin(chipSelect)){
    File root = SD.open("/");
    listDir(root, 0, output);
  }
}
*/


/*
template<typename T> void listDir(File dir, int numTabs, T* output){
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      output->print('\t');
    }
    output->print(entry.name());
    if (entry.isDirectory()) {
      output->println("/");
      listDir(entry, numTabs + 1, output);
    } else {
      // files have sizes, directories do not
      output->print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
*/


  private:

    /***** McGraw variables *****/
    /*--------------------------*/
    // create a serial stream
    // Max of 99 files assumed in a single directory
    const uint8_t nMax = 99;
    char directory[13] = "/root/";  //allow up to ten character directory names plus two '/' and NULL terminator

    char f_name[46];                //the filename variable
    char f_type='N';                //the filetype string variable


    /*--------------------------*/
    /***** McGraw variables *****/



    const char* tapeRoot = "/Tek_4924";


    // Chip select pin
    #ifdef SDCARD_CS_PIN
      const uint8_t chipSelect = SDCARD_CS_PIN;
    #else
      const uint8_t chipSelect = 4;
    #endif
  
    // SD card objects
    SdCard arSdCard;
    FsVolume arSdVolume;
    SdFile arSdRoot;

    SdFat sd;
    SdFile file;
    SdFile dirFile;
    SdFile rdfile;

    using stgcHandler = void (SDstorage::*)();

    // Storage GPIB command functions
    void stgc_0x60_h();
    void stgc_0x61_h();
    void stgc_0x62_h();
    void stgc_0x63_h();
    void stgc_0x64_h();
    void stgc_0x66_h();
    void stgc_0x67_h();
    void stgc_0x69_h();
    void stgc_0x6C_h();
    void stgc_0x6D_h();
    void stgc_0x6E_h();
    void stgc_0x6F_h();
    void stgc_0x73_h();
    void stgc_0x7B_h();
    void stgc_0x7C_h();
    void stgc_0x7D_h();
    void stgc_0x7E_h();

    // Storage command function record
    struct storeCmdRec {
      const uint8_t cmdByte;
      stgcHandler handler;
    };

    static storeCmdRec storeCmdHidx[STGC_SIZE];

};



#endif // AR488_STORE_TEK_4924_H
