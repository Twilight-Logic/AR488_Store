#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"
#include "AR488_GPIB.h"

#ifdef EN_STORAGE


/***** AR488_Store_Tek_4924.cpp, ver. 0.04.06, 02/03/2021 *****/
/*
 * Tektronix Storage functions implementation
 */

/*
 * GPIB handling routines included from GPIB.h:
 * 
 * void gpibSendDataFromFile(File sdfile);
 * bool gpibWriteToFile(File sdfile);
 */




/**************************************/
/***** SD Card handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/

SDstorage::SDstorage(){
  // Function fails on error
  // Initialise SD card object
  if (!arSdCard.begin(SD_CONFIG)) return;
//  if (!arSdCard.begin(SDCARD_CS_PIN, SPI_SPEED)) return;
  if (!arSdCard.card()->readCSD(&m_csd)) return;
  issdinit = true;
  
  // Initialise volume object
  if (arSdCard.vol()->fatType() == 0) return;
  isvolmounted = true;

  // Check for presence of Tek4924 directory, create if required
  if (!chkTek4924Directory()) return;
  // Check for tapes list, create if required
  if (!chkTapesFile()) return;
  
//      selectTape(1);
}


bool SDstorage::isSDInit(){
  return issdinit;
}


bool SDstorage::isVolumeMounted(){
  return isvolmounted;
}


bool SDstorage::isStorageInit(){
  return isstorageinit;
}


/***** Show information about the SD card *****/
void SDstorage::showSDInfo(print_t* output) {
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


/***** Show information about the volume on the SD card *****/
void SDstorage::showSdVolumeInfo(print_t* output) { 
//  uint32_t volumesize = arSdCard.card()->sectorCount();
uint32_t volumesize = 0;
  // Type and size of the first FAT-type volume
  if (arSdCard.vol()->fatType()>0){
    output->print(F("Volume type is:\t\tFAT"));
    output->println(arSdCard.vol()->fatType(), DEC);
    output->print(F("Clusters:\t\t"));
    output->println(arSdCard.clusterCount());
    output->print(F("Blocks per cluster:\t"));
    output->println(arSdCard.vol()->sectorsPerCluster());
    output->print(F("Total blocks:\t\t"));
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


/***** List files on the SD card *****/
void SDstorage::listSdFiles(print_t* output){
  if (arSdCard.begin(SD_CONFIG)){
    arSdCard.ls(output, "/", LS_R|LS_DATE|LS_SIZE );
  }
}


/***** Look for Tek_49245 directory *****/
/*
 * If it doesn't exist then it will be created
 */
bool SDstorage::chkTek4924Directory() {
  if (arSdCard.exists(tapeRoot)){
    return true; 
  }else{
    return arSdCard.mkdir(tapeRoot);
  }
}


/***** Look for "tapes" file *****/
bool SDstorage::chkTapesFile() {
  File tapeListFile;

  char tapeListFileName[20];
  memset (tapeListFileName, '\0', 20);
  strcat(tapeListFileName, tapeRoot);
  strcat(tapeListFileName, "/");
  strcat(tapeListFileName, tapeList);
  strcat(tapeListFileName, ".lst");
  
  if (arSdCard.exists(tapeListFileName)){
    return true;
  }else{
    tapeListFile = arSdCard.open(tapeListFileName, FILE_WRITE);
    if (tapeListFile) {
      tapeListFile.close();
      return true;
    }
  }
  return false;
}


/*
bool SDstorage::selectTape(uint8_t tnum){
  char tnumstr[2];

  sprintf(tnumstr, "%02d", tnum );
  memset(currentTapeName, '\0', 35);
  strcat(currentTapeName, "/");
  strcat(currentTapeName, tapeRoot);
  strcat(currentTapeName, "/");
  strcat(currentTapeName, tnumstr);
  strcat(currentTapeName, "_TAPE_");

  if (arSdCard.exists(currentTapeName)){
    return true;
  }else{
    return arSdCard.mkdir(currentTapeName);
  }
 
 return false;   
}
*/

/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** SD Card handling functions *****/
/**************************************/




/**************************************/
/***** Command handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/


/***** Command handler interface *****/
/*
void SDstorage::storeExecCmd(uint8_t cmd) {
  uint8_t i = 0;
//  int sclsize = sizeof(storeCmdHidx) / sizeof(storeCmdHidx[0]);
//  int sclsize = std::size(storeCmdHidx);
  size_t sclsize = STGC_SIZE; 

  
  // Check whether the command byte is valid
  do {
    if (storeCmdHidx[i].cmdByte == cmd) break;
    i++;
  } while (i < sclsize);

  // If valid then call handler
  if (i<sclsize){
#ifdef DEBUG_STORE
    Serial.print(F("Executing command "));
    Serial.println(cmd, HEX);
#endif
    if (i < sclsize) (this->*storeCmdHidx[i].handler)();
  }

}
*/


/***** Command handlers *****/

void SDstorage::stgc_0x60_h(){
  
}


void SDstorage::stgc_0x61_h() {
  
}


void SDstorage::stgc_0x62_h() {
  
}


void SDstorage::stgc_0x67_h(){
  
}


void SDstorage::stgc_0x69_h() {
  
}


void SDstorage::stgc_0x6C_h() {
  
}


void SDstorage::stgc_0x6F_h(){
  
}


void SDstorage::stgc_0x7B_h(){
  
}


void SDstorage::stgc_0x7C_h(){
  
}


void SDstorage::stgc_0x7D_h(){
  
}



// Array containing index of accepted storage commands
SDstorage::storeCmdRec SDstorage::storeCmdHidx [] = { 
  { 0x60, &SDstorage::stgc_0x60_h }, 
  { 0x61, &SDstorage::stgc_0x61_h },
  { 0x62, &SDstorage::stgc_0x62_h },
  { 0x67, &SDstorage::stgc_0x67_h },
  { 0x69, &SDstorage::stgc_0x69_h },
  { 0x6C, &SDstorage::stgc_0x6C_h },
  { 0x6F, &SDstorage::stgc_0x6F_h },
  { 0x7B, &SDstorage::stgc_0x7B_h },
  { 0x7C, &SDstorage::stgc_0x7C_h },
  { 0x7D, &SDstorage::stgc_0x7D_h }
};


/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Command handling functions *****/
/**************************************/

#endif  // EN_STORAGE
