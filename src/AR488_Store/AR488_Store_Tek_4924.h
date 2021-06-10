#ifndef AR488_STORE_TEK_4924_H
#define AR488_STORE_TEK_4924_H

#include "AR488_Config.h"

#ifdef EN_STORAGE

//#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"

/***** AR488_Storage_Tek_4924.h, ver. 0.04.06, 02/03/2021 *****/

// Number of storage GPIB commands
#define STGC_SIZE 10
//#define SPI_SPEED SD_SCK_MHZ(4)
#define SPI_SPEED SD_SCK_MHZ(16)
#define SD_CONFIG SdSpiConfig(SDCARD_CS_PIN, SPI_SPEED)

class SDstorage {

  public:

    // Storage management functions
    SDstorage();
    void showSDInfo(print_t* output);
    void showSdVolumeInfo(print_t* output);
    void listSdFiles(print_t* output);
    bool chkTek4924Directory();
//    bool selectTape(uint8_t tnum);

    bool isSDInit();
    bool isVolumeMounted();
    bool isStorageInit();

    bool chkTapesFile();
//    bool selectTape(uint8_t tnum);
    
//    void storeExecCmd(uint8_t cmd);

    const size_t stgcSize = 10;
    uint8_t currentTapeNum = 1;
    uint8_t currentFileNum = 1;
//    char currentTapeName[35] = {'\0'};
    char currentFileName[48] = {'\0'};

  private:

    const char* tapeRoot = "/Tek_4924";
    const char* tapeList = "tapes";

    // Chip select pin
    #ifdef SDCARD_CS_PIN
      const uint8_t sdCardCsPin = SDCARD_CS_PIN;
    #else
      const uint8_t sdCardCsPin = 4;
    #endif

    // FAT16 + FAT32
    SdFat32 arSdCard;
    File32 sdFile;
    csd_t m_csd;

    bool issdinit = false;
    bool isvolmounted = false;
    bool isstorageinit = false;
    
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

#endif  // EN_STORAGE

#endif // AR488_STORE_TEK_4924_H
