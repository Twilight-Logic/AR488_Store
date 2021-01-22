#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AR488_Store_Tek_4924.h"


/***** AR488_Store_Tek.cpp, ver. 0.01.04, 21/01/2021 *****/
/*
 * Tektronix Storage functions implementation
 */



storage::storage(){
  card = new Sd2Card;
  volume = new SdVolume;
  root = new SdFile;
}







/**************************************/
/***** SD Card handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/

void storage::init(){
  
}



/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** SD Card handling functions *****/
/**************************************/




/**************************************/
/***** Command handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/


/***** Command handler interface *****/
void storage::storeExecCmd(uint8_t cmd) {
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



/***** Command handlers *****/


void storage::stgc_0x60_h(){
  
}


void storage::stgc_0x61_h() {
  
}


void storage::stgc_0x62_h() {
  
}


void storage::stgc_0x67_h(){
  
}


void storage::stgc_0x69_h() {
  
}


void storage::stgc_0x6C_h() {
  
}


void storage::stgc_0x6F_h(){
  
}


void storage::stgc_0x7B_h(){
  
}


void storage::stgc_0x7C_h(){
  
}


void storage::stgc_0x7D_h(){
  
}


// Array containing index of accepted storage commands
storage::storeCmdRec storage::storeCmdHidx [] = { 
  { 0x60, &storage::stgc_0x60_h }, 
  { 0x61, &storage::stgc_0x61_h },
  { 0x62, &storage::stgc_0x62_h },
  { 0x67, &storage::stgc_0x67_h },
  { 0x69, &storage::stgc_0x69_h },
  { 0x6C, &storage::stgc_0x6C_h },
  { 0x6F, &storage::stgc_0x6F_h },
  { 0x7B, &storage::stgc_0x7B_h },
  { 0x7C, &storage::stgc_0x7C_h },
  { 0x7D, &storage::stgc_0x7D_h }
};


/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Command handling functions *****/
/**************************************/
