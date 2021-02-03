#ifndef AR488_STORE_TEK_H
#define AR488_STORE_TEK_H

#include <SPI.h>
#include <SD.h>

/***** AR488_Eeprom_Tek.h, ver. 0.01.08, 28/01/2021 *****/
/*
 * Tektronix Storage Functions Definitions
 */


#define STGC_SIZE 10




template<typename T> void showSDInfo(T* output) {
  Sd2Card sdcard;
  output->print(F("Card type:\t\t"));
  switch (sdcard.type()) {
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
//  output->print(F("SD Card size:\t"));
//  output->print(sdcard.cardSize());
//  output->println(F("MB"));
}


template<typename T> void showSdVolumeInfo(T* output) {
  Sd2Card sdcard;
  SdVolume sdvolume;
  uint32_t volumesize;

  // Type and size of the first FAT-type volume

  if (sdvolume.init(sdcard)){
    output->print("Volume type is:\t\tFAT");
    output->println(sdvolume.fatType(), DEC);
    output->print("Clusters:\t\t");
    output->println(sdvolume.clusterCount());
    output->print("Blocks per cluster:\t");
    output->println(sdvolume.blocksPerCluster());
    output->print("Total blocks:\t\t");
    output->println(sdvolume.blocksPerCluster() * sdvolume.clusterCount());

    volumesize = sdvolume.blocksPerCluster(); // clusters are collections of blocks
    volumesize *= sdvolume.clusterCount();    // we'll have a lot of clusters
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




class SDstorage {
  public:

    // Storage management functions
    SDstorage();
    void showVolumeInfo();
    bool isInit();
    bool isVolumeMounted();
    
//    void storeExecCmd(uint8_t cmd);
    const size_t stgcSize = 10;
    bool isinit = false;
    bool isvolmounted = false;
    

  private:
    Sd2Card sdcard;
    SdVolume sdvolume;
    SdFile sdroot;

#ifdef CHIP_SELECT_PIN
    const uint8_t chipSelect = CHIP_SELECT_PIN;
#else
    const uint8_t chipSelect = 4;
#endif






/*
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
*/
};



#endif // AR488_STORE_TEK_H
