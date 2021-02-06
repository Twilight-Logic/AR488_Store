#ifndef AR488_STORE_TEK_4924_H
#define AR488_STORE_TEK_4924_H

#include <SPI.h>
#include <SD.h>
#include "AR488_Config.h"

/***** AR488_Storage_Tek_4924.h, ver. 0.02.02, 06/02/2021 *****/
/*
 * Tektronix Storage Functions Definitions
 */

// Number of storage GPIB commands
#define STGC_SIZE 10


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

template<typename T> void listSdFiles(T* output){
  if (SD.begin(chipSelect)){
    File root = SD.open("/");
    listDir(root, 0, output);
  }
}


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
    

  private:

    const char* tapeRoot = "/Tek_4924";

    // Chip select pin
    #ifdef SDCARD_CS_PIN
      const uint8_t chipSelect = SDCARD_CS_PIN;
    #else
      const uint8_t chipSelect = 4;
    #endif
  
    // SD card objects
    Sd2Card arSdCard;
    SdVolume arSdVolume;
    SdFile arSdRoot;

    using stgcHandler = void (SDstorage::*)();

    // Storage GPIB command functions
    void stgc_0x60_h();
    void stgc_0x61_h();
    void stgc_0x62_h();
    void stgc_0x67_h();
    void stgc_0x69_h();
    void stgc_0x6C_h();
    void stgc_0x6F_h();
    void stgc_0x7B_h();
    void stgc_0x7C_h();
    void stgc_0x7D_h();


    // Storage command function record
    struct storeCmdRec {
      const uint8_t cmdByte;
      stgcHandler handler;
    };

    static storeCmdRec storeCmdHidx[STGC_SIZE];

};



#endif // AR488_STORE_TEK_4924_H
