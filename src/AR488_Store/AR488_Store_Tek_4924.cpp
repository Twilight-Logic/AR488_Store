#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"



/***** AR488_Store_Tek_4924.cpp, ver. 0.05.17, 30/06/2021 *****/
/*
 * Tektronix 4924 Tape Storage functions implementation
 */


extern Stream& debugStream;

extern GPIBbus gpibBus;

extern ArduinoOutStream cout;

char streamBuffer[LINELENGTH];
CharStream charStream(streamBuffer, LINELENGTH);
ArduinoOutStream gpibout(charStream);

//#define CR   0xD    // Carriage return
//#define LF   0xA    // Newline/linefeed



/**************************************/
/***** SD Card handling functions *****/
/*****vvvvvvvvvvvvvvvvvvvvvvvvvvvv*****/

SDstorage::SDstorage(){
/*
  // Initialise SD card object
  SD.begin(chipSelect);
  if (arSdCard.init(SPI_HALF_SPEED, chipSelect)) isinit = true;
 
  // Attempt to mount volume
  if (arSdVolume.init(arSdCard)) isvolmounted = true;

  // Check for the existence of the Tek_4924 directory
  if (chkTek4924Directory()) {

    if (chkTapesFile()){
      selectTape(1);
    }

  }
 */
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  if (!sd.cardBegin(SD_CONFIG)) {
#ifdef DEBUG_STORE
    debugStream.println(F("SD card initialisation failed!"));
#endif
  }
  
}


bool SDstorage::isInit(){
  return isinit;
}


bool SDstorage::isVolumeMounted(){
  return isvolmounted;
}


/***** Showe information about the SD card *****/
/*
void SDstorage::showSDInfo(Stream& outputStream) {
//  Sd2Card sdcard;
  outputStream.print(F("Card type:\t\t"));
  switch (arSdCard.type()) {
    case SD_CARD_TYPE_SD1:
      outputStream.println("MMC");
      break;
    case SD_CARD_TYPE_SD2:
      outputStream.println("SDSC");
      break;
    case SD_CARD_TYPE_SDHC:
      outputStream.println("SDHC");
      break;
    default:
      outputStream.println("Unknown");
  }
}
*/


/***** Show information about the SD volume *****/
void SDstorage::showSdVolumeInfo(Stream& outputStream) {
//  Sd2Card sdcard;
//  SdVolume sdvolume;
//  uint32_t volumesize;

  // Type and size of the first FAT-type volume
/*
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
*/
}


/***** List files on SD card *****/
void SDstorage::listSdFiles(Stream& outputStream){
/*  
  if (SD.begin(chipSelect)){
    File root = SD.open("/");
    listDir(root, 0, outputStream);
  }
*/
}


/***** Recursive directory listing functuioon *****/
void SDstorage::listDir(Stream& outputStream, File dir, int numTabs){
/*  
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      outputStream.print('\t');
    }
    outputStream.print(entry.name());
    if (entry.isDirectory()) {
      outputStream.println("/");
      listDir(outputStream, entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      outputStream.print("\t\t");
      outputStream.println(entry.size(), DEC);
    }
    entry.close();
  }
*/
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
void SDstorage::storeExecCmd(uint8_t cmd) {
  uint8_t i = 0;
  while (storeCmdHidx[i].cmdByte) {
#ifdef DEBUG_STORE
//    debugStream.print(storeCmdHidx[i].cmdByte, HEX);
//    debugStream.print(" ");
#endif
    if (storeCmdHidx[i].cmdByte == cmd) {
#ifdef DEBUG_STORE
      debugStream.print(F("Executing secondary address command: "));
      debugStream.println(cmd, HEX);
#endif
      // Call handler
      (this->*storeCmdHidx[i].handler)();
      return;
    }
    i++;
  }
#ifdef DEBUG_STORE
  debugStream.print(F("Secondary command: "));
  debugStream.print(cmd, HEX);
  debugStream.println(F(" not found!"));
#endif
}


/***** Command handlers *****/

/***** STATUS command *****/
void SDstorage::stgc_0x60_h(){
  
}


/***** SAVE command *****/
void SDstorage::stgc_0x61_h(){
  
}


/***** CLOSE command *****/
void SDstorage::stgc_0x62_h(){
  
}


/***** OPEN command *****/
void SDstorage::stgc_0x63_h(){
  
}


/***** OLD/APPEND command *****/
void SDstorage::stgc_0x64_h() {
  
}


/***** TYPE command *****/
void SDstorage::stgc_0x66_h() {
  
}


/***** KILL command *****/
void SDstorage::stgc_0x67_h(){
  
}


// DIRECTORY command
void SDstorage::stgc_0x69_h() {

}


/***** PRINT command *****/
void SDstorage::stgc_0x6C_h() {
  
}


/***** INPUT command *****/
void SDstorage::stgc_0x6D_h() {
  
}


/***** READ command *****/
void SDstorage::stgc_0x6E_h() {
  
}


/***** WRITE command *****/
void SDstorage::stgc_0x6F_h() {
  
}


/***** TLIST command *****/
void SDstorage::stgc_0x73_h(){

  uint8_t number = 1;
  uint8_t index = 0;
  bool lastfile = false;
  
  // Set GPIB bus for talking (output)
  gpibBus.setControls(DTAS);

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("Started TLIST handler..."));
#endif

  // match the specific Tek4924 tape file number from f_name
//  for (int number = 1; number < nMax; number++) { // find each file in # sequence
//  for (uint8_t number = 1; number < nMax; number++) { // find each file in # sequence
  while ((!lastfile) && (number < nMax)) {

    //cout << "Directory: " << directory << '\r\n';
    f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
    file.close();  //  close any file that could be open (from previous FIND for example)

    if (!dirFile.open(directory, O_RDONLY)) {
      sd.errorHalt("Open directory failed - possible invalid directory name");
    }
    file.rewind();

//    for (int index = 0; index < nMax; index++) { //SdFat file index number
//    for (uint8_t index = 0; index < nMax; index++) { //SdFat file index number

    while ((file.openNext(&dirFile, O_RDONLY)) && index < nMax) {

      // while (n < nMax ) {
//      file.openNext(&dirFile, O_RDONLY);
      // Skip directories, hidden files, and null files
      if (!file.isSubDir() && !file.isHidden()) {

        file.getName(f_name, 46);

//        int filenumber = atoi(f_name);
        uint8_t filenumber = atoi(f_name);

        if (filenumber == number) {
          // print the entire file 'header' with leading space, and CR + DC3 delimiters
          // Note \x019 = end of medium
//          cout << F(" ") << f_name << '\r' << '\x019';

          // Stream info to buffer 
          charStream.flush();
          gpibout << F(" ") << f_name << '\r' << '\x019';
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
        
        }
        
        if ((f_name[7] == 'L') && (filenumber == number)) {
          // then this is the LAST file - end of TLIST
//          file.close();  // end of iteration close **this** file
//          f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
//          f_type = 'O';    // set file type to "O" for NOT OPEN
//          dirFile.close();  // end of iteration through all files, close directory

#ifdef DEBUG_STORE_COMMANDS
          debugStream.println(F("Reached last file."));  
#endif    
          lastfile = true;
//          return;

        } else if (file.isDir()) {
          // Indicate a directory.
//          cout << '/' << endl;

          // Stream info to buffer
          charStream.flush();
          gpibout << '/' << endl;
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());

        }

      }
      index++;
      file.close();  // end of iteration close **this** file
    }

    number++;
    f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
    f_type = 'O';    // set file type to "O" for NOT OPEN
    dirFile.close();  // end of iteration through all files, close directory
  } 

  // Send EOI to end transmission
  gpibBus.sendEOI();

  // Return GPIB bus to listening idle state
  gpibBus.setControls(DIDS);

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("End TLIST handler."));  
#endif

}


/***** FIND command *****/
void SDstorage::stgc_0x7B_h(){
  char receiveBuffer[82];
  uint8_t paramLen = 0;
  uint8_t num = 0;
  char ftype = '\0';

  paramLen = gpibBus.receiveParams(false, receiveBuffer, 82);

  // If we have received parameter data
  if (paramLen > 0) {

    num = atoi(receiveBuffer);

    // Set GPIB bus for talking (output)
    gpibBus.setControls(DTAS);
 

    // char f_name[46];  //the filename variable
    /*  FINDs and OPENs file (num)
         Returns: string result= "A" for ASCII, "H" for HEX (Binary or Secret file), "N" for Not Found

         FIND iterates through each file in a directory until filenumber matches num
         since SdFat file index is not sequential with Tek 4050 filenames
    */

    if (!dirFile.open(directory, O_RDONLY)) {
        sd.errorHalt("Open directory failed");
    }

    for (int index = 0; index < nMax; index++) { //SdFat file index number

        // while (n < nMax ) {
        file.openNext(&dirFile, O_RDONLY);
        // Skip directories, hidden files, and null files
        if (!file.isSubDir() && !file.isHidden()) {

            file.getName(f_name, 46);

            int filenumber = atoi(f_name);

            if (filenumber == num) {
                // debug print the entire file 'header' with leading space, and CR + DC3 delimiters
//                cout << F(" ") << f_name << "\r ";

          charStream.flush();
          gpibout << F(" ") << f_name << "\r ";
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());


                // all BINARY files are in HEX format
                if ((f_name[7] == 'B') && (f_name[15] == 'P')) {
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'B'; // BINARY PROGRAM file
                    ftype = 'B';
                } else if ((f_name[7] == 'B') && (f_name[15] == 'D')) {
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'H'; // BINARY DATA file ** to read - parse the data_type
                    ftype = 'H';
                } else if (f_name[25] == 'S') {  // f_name[25] is location of file type SECRET ASCII PROGRAM
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'S'; // SECRET ASCII PROGRAM file
                      ftype = 'S';
                } else if (f_name[15] == 'P') {  // f_name[15] is location of file type (PROG, DATA,...)
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'P'; // ASCII PROGRAM file
                      ftype = 'P';
                } else if (f_name[7] == 'N') {  // f_name[7] is location of file type (ASCII,BINARY,NEW, or LAST)
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'N'; // ASCII LAST file
                      // ftype = 'N';
                } else if (f_name[7] == 'L') {  // f_name[7] is location of file type (ASCII,BINARY,LAST)
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'L'; // ASCII LAST file
                      ftype = 'L';
                } else {
                    dirFile.close();  // end of iteration through all files, close directory
//                    return 'D'; // ASCII DATA file, also allows other types like TEXT and LOG to be treated as DATA
                    ftype = 'D';
                }
                if (ftype) {
                  gpibBus.sendRawData(&ftype, 1);
                  gpibBus.sendEOI();
                  gpibBus.setControls(DIDS);
                  return;
                }
            }
            //f_name[0]=0; // clear f_name

            if (file.isDir()) {
                // Indicate a directory.
//                cout << '/' << endl;

          charStream.flush();
          gpibout << '/' << endl;
          // Send info to GPIB bus
          gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
                
            }

        }
        file.close();  // end of iteration close **this** file

    }
//    cout << F("File ") << num << " not found" << endl;

    charStream.flush();
    gpibout << F("File ") << num << " not found" << endl;
    // Send info to GPIB bus
    gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
    // Send EOI
    gpibBus.sendEOI();
    // Return bus to idle
    gpibBus.setControls(DIDS);

    
    f_name[0] = '\0';  // clear f_name
    dirFile.close();  // end of iteration through all files, close directory

//    return 'O'; // File num NOT OPEN (or NOT FOUND)

  }

}


/***** MARK command *****/
void SDstorage::stgc_0x7C_h(){
  
}


/***** SECRET command *****/
void SDstorage::stgc_0x7D_h(){
  
}


/***** ERROR command *****/
/*
 *  1 - Domain error / invalid argument
 *  2 - File not found
 *  3 - Mag tape format error
 *  4 - Illegal access
 *  5 - File not open
 *  6 - Read error
 *  7 - No cartridge inserted
 *  8 - Over read (illegal tape length)
 *  9 - Write protected
 * 10 - Read after write error
 * 11 - End of medium
 * 12 - End of file
 */
void SDstorage::stgc_0x7E_h(){  
  gpibBus.setControls(DTAS);
  charStream.flush();
  gpibout << errorCode << "\r" << endl;
  // Send info to GPIB bus
  gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
  // Send all clear (for test only)
  gpibBus.sendEOI();
  gpibBus.setControls(DIDS);
}




// Array containing index of accepted storage commands
SDstorage::storeCmdRec SDstorage::storeCmdHidx [] = { 
  { 0x60, &SDstorage::stgc_0x60_h }, 
  { 0x61, &SDstorage::stgc_0x61_h },
  { 0x62, &SDstorage::stgc_0x62_h },
  { 0x63, &SDstorage::stgc_0x63_h },
  { 0x64, &SDstorage::stgc_0x64_h },
  { 0x66, &SDstorage::stgc_0x66_h },
  { 0x67, &SDstorage::stgc_0x67_h },
  { 0x69, &SDstorage::stgc_0x69_h },
  { 0x6C, &SDstorage::stgc_0x6C_h },
  { 0x6D, &SDstorage::stgc_0x6D_h },
  { 0x6E, &SDstorage::stgc_0x6E_h },
  { 0x6F, &SDstorage::stgc_0x6F_h },
  { 0x73, &SDstorage::stgc_0x73_h },
  { 0x7B, &SDstorage::stgc_0x7B_h },
  { 0x7C, &SDstorage::stgc_0x7C_h },
  { 0x7D, &SDstorage::stgc_0x7D_h },
  { 0x7E, &SDstorage::stgc_0x7E_h }
};


/*****^^^^^^^^^^^^^^^^^^^^^^^^^^^^*****/
/***** Command handling functions *****/
/**************************************/
