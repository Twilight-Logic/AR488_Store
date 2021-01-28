#ifndef AR488_STORE_TEK_H
#define AR488_STORE_TEK_H

#include <SD.h>

/***** AR488_Eeprom_Tek.h, ver. 0.01.08, 28/01/2021 *****/
/*
 * Tektronix Storage Functions Definitions
 */


#define STGC_SIZE 10

class SDstorage {
  public:

    // Storage management functions
    SDstorage();
    void getInfo();
    bool isInit();
    bool isVolumeMounted();
    uint8_t sdType();
    void storeExecCmd(uint8_t cmd);
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
