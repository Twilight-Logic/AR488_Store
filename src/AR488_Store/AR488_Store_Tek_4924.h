#ifndef AR488_STORE_TEK_4924_H
#define AR488_STORE_TEK_4924_H

//#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"
#include "sdios.h"
#include "AR488_Config.h"

/***** AR488_Storage_Tek_4924.h, ver. 0.04.01, 23/02/2021 *****/

// Number of storage GPIB commands
#define STGC_SIZE 10
//#define SPI_SPEED SD_SCK_MHZ(4)
#define SPI_SPEED SD_SCK_MHZ(16)
#define SD_CONFIG SdSpiConfig(SDCARD_CS_PIN, DEDICATED_SPI, SPI_SPEED)

class SDstorage {

  public:

    // Storage management functions
    SDstorage();
    void showVolumeInfo();
    bool isSDInit();
    bool isVolumeMounted();
    bool isStorageInit();
//    void listFiles();
    bool chkTek4924Directory();
//    bool chkTapesFile();
//    bool selectTape(uint8_t tnum);
    
//    void storeExecCmd(uint8_t cmd);

    const size_t stgcSize = 10;
    uint8_t currentTapeNum = 1;
    uint8_t currentFileNum = 1;
    char currentTapeName[35] = {'\0'};
    char currentFileName[25] = {'\0'};

template<typename T> void showSDInfo(T* output) {
  output->print(F("Card type:\t\t"));
  switch (arSdCard.card()->type()) {
    case SD_CARD_TYPE_SD1:
      output->println(F("MMC"));
      break;
    case SD_CARD_TYPE_SD2:
      output->println(F("SDSC"));
      break;
    case SD_CARD_TYPE_SDHC:
      output->println(F("SDHC"));
      break;
    default:
      output->println(F("Unknown"));
  }
  output->print(F("Card size:\t\t"));
  output->print(0.000512 * sdCardCapacity(&m_csd));
  output->println(F("Mb"));
}


template<typename T> void showSdVolumeInfo(T* output) { 
  uint32_t volumesize = arSdCard.card()->sectorCount();

  // Type and size of the first FAT-type volume
  if (arSdCard.vol()->fatType()>0){
    output->print(F("Volume type is:\t\tFAT"));
    output->println(arSdCard.vol()->fatType(), DEC);
    output->print(F("Clusters:\t\t"));
    output->println(arSdCard.clusterCount());
    output->print(F("Blocks per cluster:\t"));
    output->println(arSdCard.vol()->sectorsPerCluster());
    output->print("Total blocks:\t\t");
    output->println(arSdCard.vol()->sectorsPerCluster() * arSdCard.vol()->clusterCount());

    volumesize = arSdCard.vol()->sectorsPerCluster(); // clusters are collections of blocks
    volumesize *= arSdCard.vol()->clusterCount();    // we'll have a lot of clusters
    volumesize /= 2;                          // SD card blocks are always 512 bytes (2 blocks are 1KB)
    volumesize /= 1024;                       // Convert to Mb

    if (volumesize>1024) {
      output->print(F("Volume size (Gb):\t"));
      output->println((float)volumesize/1024.0);      
    }else{
      output->print(F("Volume size (Mb):\t"));
      output->println(volumesize);
    }

  }
}


template<typename T> void listSdFiles(T* output){
  if (arSdCard.begin(SD_CONFIG)){
    FsFile root = arSdCard.open("/");
    listDir(root, 0, output);
  }
}


template<typename T> void listDir(FsFile dir, int numTabs, T* output){
  while (true) {
    FsFile entry = dir.openNextFile();
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
//      output->print("\t\t");
//      output->println(entry.size(), DEC);
    }
    entry.close();
  }
}


  private:

    const char* tapeRoot = "/Tek_4924";

    // Chip select pin
    #ifdef SDCARD_CS_PIN
      const uint8_t sdCardCsPin = SDCARD_CS_PIN;
    #else
      const uint8_t sdCardCsPin = 4;
    #endif

    // FAT16 + FAT32
//    SdFat32 arSdCard;
//    File32 sdFile;

    // FAT16 + FAT32 + ExFAT
    SdFs arSdCard;
    ExFile sfFile; 

    csd_t m_csd;

    bool issdinit = false;
    bool isvolmounted = false;
    bool isstorageinit = false;
    
    using stgcHandler = void (SDstorage::*)();

    // Storage GPIB command functions
//    void stgc_0x60_h();
//    void stgc_0x61_h();
//    void stgc_0x62_h();
//    void stgc_0x67_h();
//    void stgc_0x69_h();
//    void stgc_0x6C_h();
//    void stgc_0x6F_h();
//    void stgc_0x7B_h();
//    void stgc_0x7C_h();
//    void stgc_0x7D_h();


    // Storage command function record
    struct storeCmdRec {
      const uint8_t cmdByte;
      stgcHandler handler;
    };

    static storeCmdRec storeCmdHidx[STGC_SIZE];

};



#endif // AR488_STORE_TEK_4924_H
