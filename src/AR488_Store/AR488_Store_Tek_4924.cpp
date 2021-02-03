#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"


/***** AR488_Store_Tek.cpp, ver. 0.01.09, 03/02/2021 *****/
/*
 * Tektronix Storage functions implementation
 */








/**************************************/
/***** SD Card handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/

SDstorage::SDstorage(){

  // Initialise SD card object
  SD.begin(chipSelect);
  if (sdcard.init(SPI_HALF_SPEED, chipSelect)) isinit = true;
  
  // Attempt to mount volume
  if (sdvolume.init(sdcard)) isvolmounted = true;

}


bool SDstorage::isInit(){
  return isinit;
}


bool SDstorage::isVolumeMounted(){
  return isvolmounted;
}

/*
uint8_t SDstorage::sdType(){
  return sdcard.type();
}


uint32_t SDstorage::sdSize(){
  return sdcard.cardSize();
}
*/


/*
uint8_t SDstorage::fatType(){
  return sdvolume.fatType();
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
  if (i < sclsize) (this->*storeCmdHidx[i].handler)();

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
