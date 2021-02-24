#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"
#include "AR488_GPIB.h"


/***** AR488_Store_Tek_4924.cpp, ver. 0.04.02, 23/02/2021 *****/
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

  // Initialise SD card object
//  SD.begin(chipSelect);

  if (arSdCard.begin(sdCardCsPin, SPI_SPEED)) isinit = true;
//  if (arSdCard.init(SPI_HALF_SPEED, chipSelect)) isinit = true;
 
  // Attempt to mount volume
  if (isinit) {
//  if (arSdVolume.init(arSdCard)) isvolmounted = true;
    


  // Check for the existence of the Tek_4924 directory
/*
  if (chkTek4924Directory()) {

    if (chkTapesFile()){
      selectTape(1);
    }

  }
*/
  }
}


bool SDstorage::isInit(){
  return isinit;
}


bool SDstorage::isVolumeMounted(){
  return isvolmounted;
}





/***** Look for Tek_49245 directory *****/
/*
 * If it doesn't exist then it will be created
 */
/* 
bool SDstorage::chkTek4924Directory() {
  if (SD.exists(F("/Tek_4924"))){
    return true; 
  }else{
    return SD.mkdir(F("/Tek_4924"));
  }
}
*/

/***** Look for "tapes" file *****/
/*
bool SDstorage::chkTapesFile() {
  if (SD.exists(tapeRoot)){
    return true;
  }else{
    File tapes = SD.open(tapeRoot, FILE_WRITE);
    if (tapes) {
      tapes.println(F("01_tape_default"));
      tapes.close();
      return true;
    }else{
      return false;
    }
  }
}
*/

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

  if (SD.exists(currentTapeName)){
    return true;
  }else{
    return SD.mkdir(currentTapeName);
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

/*
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
*/

/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Command handling functions *****/
/**************************************/
