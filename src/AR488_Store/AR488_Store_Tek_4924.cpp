#include <Arduino.h>

#include "AR488_Store_Tek_4924.h"



/***** AR488_Store_Tek_4924.cpp, ver. 0.05.20, 05/07/2021 *****/
/*
 * Tektronix 4924 Tape Storage functions implementation
 */

#ifdef EN_STORAGE


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
  pinMode(SDCARD_CS_PIN, OUTPUT);
  digitalWrite(SDCARD_CS_PIN, HIGH);
//  if (!sd.cardBegin(SD_CONFIG)) {
  if (!sd.begin(SD_CONFIG)) {
    errorCode = 7;
#ifdef DEBUG_STORE
    debugStream.println(F("SD card initialisation failed!"));
#endif
  }
  
}

/*
bool SDstorage::isInit(){
  return isinit;
}
*/

/*
bool SDstorage::isVolumeMounted(){
  return isvolmounted;
}
*/


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
/*
void SDstorage::showSdVolumeInfo(Stream& outputStream) {
//  Sd2Card sdcard;
//  SdVolume sdvolume;
//  uint32_t volumesize;

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
*/


/***** List files on SD card *****/
/*  
void SDstorage::listSdFiles(Stream& outputStream){
  if (SD.begin(chipSelect)){
    File root = SD.open("/");
    listDir(root, 0, outputStream);
  }
}
*/


/***** Recursive directory listing functuioon *****/
/*  
void SDstorage::listDir(Stream& outputStream, File dir, int numTabs){
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
}
*/


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


/***** OLD/APPEND command (tek_OLD) *****/
void SDstorage::stgc_0x64_h() {
  char path[60] = {0};
  char linebuffer[line_buffer_size];

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: started OLD/APPEND handler..."));
#endif

  strncpy(path, directory, strlen(directory));
  strncat(path, f_name, strlen(f_name));

  // cout << "OLD of file: " << directory << f_name << "  path= " << path << '\r\n' << '\r\n';
  ifstream sdin(path);

  // Set GPIB bus for talking (output)
  gpibBus.setControls(DTAS);

  while (sdin.getline(linebuffer, line_buffer_size, '\r')) {

//    cout << buffer << '\r';
      gpibBus.sendRawData(linebuffer, strlen(linebuffer));

  }

  f_type = 'O';  // set file type to NOT OPEN
  file.close();    // and close the current file

    // cout << '\r'; //add one CR to last line processed to end Tek BASIC program output

  // Send EOI to end transmission
  gpibBus.sendEOI();

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: done."));
#endif

}


/***** TYPE command *****/
void SDstorage::stgc_0x66_h() {
  
}


/***** KILL command *****/
void SDstorage::stgc_0x67_h(){
  
}


/***** HEADER command *****/
void SDstorage::stgc_0x69_h() {

}


/***** PRINT command *****/
void SDstorage::stgc_0x6C_h() {
  
}


/***** INPUT command *****/
void SDstorage::stgc_0x6D_h() {
  
}


/***** READ command (tek_READ_one) *****/
void SDstorage::stgc_0x6E_h() {

    /*
        Tektronix 4050 DATA Types: ASCII and BINARY
        ASCII DATA
        - ASCII Program lines are delimited with CR (unless changed) and files delimited by FF for EOF
        - ASCII Program DATA numeric items are delimited with Space, Comma, Semicolon, Colon, or CR
        - ASCII Program DATA string items are delimited by leading and trailing Quotation Marks or CR
        - To use a Quotation mark inside a character string requires double quotes: ""Help""
        BINARY DATA
        - Each data item contains its own header identifying the length of the item and the type
        - This two byte Header is generated by the talker sending the binary data
        - The two byte data header has the following form:
        MSB                            LSB   MSB                        LSB
        |T3  T2  T1  L13  L12  L11  L10  L9 | L8  L7  L6  L5  L4  L3  L2  L1|
        |           BYTE 1                  |             BYTE 2            |
        L13 to L1 comprise the length of the item including the two header bytes.
        Since all numbers in Tek 4050 BASIC are 8-byte floating point, the binary number data length = 10
        BINARY data types for READ and WRITE commands
        ----------
        T3, T2, T1 are the types of BINARY data
        0   0   0 Unassigned
        0   0   1 Binary number
        0   1   0 Binary string
        0   1   1 Unassigned
        1   0   0 Unassigned
        1   0   1 Unassigned
        1   1   0 Unassigned
        1   1   1 EOF
        Tek 4050 BASIC "TYP" command returns a value based on next item in an open file:
        TYP    Description
        ---    -----------
        0      Empty File or File Not Open
        1      End of File Character
        2      ASCII Numeric Data or Character String
        3      BINARY Numeric Data
        4      BINARY Character String
    */

  char path[60] = {0};
  char buffr[line_buffer_size];
  char buffr2[bin_buffer_size];


    char incoming = '\0';
    char CR = '\r';

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: started READ handler..."));
#endif
    
    strcpy(path, directory);
    strcat(path, f_name);

    ifstream sdin(path);  // initialize stream to beginning of the file
    switch (f_type) {
        case 'P':

#ifdef DEBUG_STORE_COMMANDS
//            cout << F("ASCII Program\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("ASCII PROGRAM"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif

            sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
            cout << buffr << '\r';                                // send that line to the serial port

            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
                        if (sdin.eof()) {
                            cout << "__EOF__" << endl;
                            return;
                        }
                        cout << buffr << '\r';                                // send that line to the serial port

                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'D':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("ASCII DATA\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("ASCII DATA"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif
            sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
            cout << buffr << '\r';                                // send that line to the serial port

            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, line_buffer_size, '\r');          // fetch one CR terminated line of the program
                        if (sdin.eof()) {
                            cout << "__EOF__" << endl;
                            return;
                        }

                        //no need to parse buffer: 4050 will read a CR terminated buffer and then parse based on BASIC parameter type
                        cout << buffr << '\r';                                // send that line to the serial port

                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }
            break;

        case 'B':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("BINARY PROGRAM\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("BINARY PROGRAM"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif
            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');
                        if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                        } else {
                            strcpy(buffr2, buffr);
                            strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;
                        }
                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'H':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("BINARY DATA\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("BINARY DATA"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif

            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');
                        if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                        } else {
                            strcpy(buffr2, buffr);
                            strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;
                        }
                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'S':
#ifdef DEBUG_STORE_COMMANDS
//            cout << F("SECRET ASCII PROGRAM\r");
//            cout << F("Type SPACE for next item, or type q to quit\r \r ");
        debugStream.println(F("SECRET ASCII PROGRAM"));
        debugStream.println(F("Type SPACE for next item, or type q to quit\r"));
#endif
            while (1) {
                while (Serial.available()) {  //any character for next data item or 'q' to quit read1
                    incoming = Serial.read();
                    if (incoming != 'q') {
                        incoming = '\0';     // clear the flag
                        sdin.getline(buffr, bin_buffer_size, '\r');

                        /*
                            if (strlen(buffr) == 0) {
                            cout << "__EOF__" << endl;
                            return;
                            } else { */

                        strcpy(buffr2, buffr);
                        strcat(buffr2, "\r"); //add CR back to buffer, this line was CR terminated;

                        //convert HEX to binary, output HEX then binary for each buffer

                        int i = 0, len = strlen(buffr2), n = 0;
                        //cout << "Buffer= " << len << " \n";
                        char byt = {0};
                        char tmp[3] = {0, 0, 0};

                        //cout << F("\nHEX: ") << buffr << " \n";
                        //cout << " \n";
                        for (i = 0; i < len - 1; i += 2)
                        {
                            tmp[0] = buffr2[i];
                            tmp[1] = buffr2[i + 1];
                            byt = strtoul(tmp, NULL, 16);
                            cout << byt;
                            //fputc(byt, stdout);
                        }
                        cout << "\r";
                    } else {  // incoming was 'q'
                        incoming = '\0';  // clear the flag
                        cout << "__Quit__" << endl;
                        return;
                    }
                }
            }

            break;

        case 'L':
            cout << F("LAST file is EMPTY\n");
            break;

        case 'N':
            cout << F("New file - no data\n");
            break;

        case 'O':
            cout << F("No file is open\n");
            break;
    }
    sdin.close();              //close input stream as we have reached EOF
    f_type = 'O';              //set file type to "File Not Open"

  // Send EOI to end transmission
  gpibBus.sendEOI();


#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: done."));
#endif


}


/***** TLIST command *****/
/***** WRITE command *****/
void SDstorage::stgc_0x6F_h() {
  uint8_t number = 1;
  uint8_t index = 0;
  bool lastfile = false;

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: started TLIST handler..."));
#endif

  // match the specific Tek4924 tape file number from f_name
//  for (int number = 1; number < nMax; number++) { // find each file in # sequence
//  for (uint8_t number = 1; number < nMax; number++) { // find each file in # sequence
  while ((!lastfile) && (number < nMax)) {

    //cout << "Directory: " << directory << '\r\n';
    f_name[0] = 0;  // clear f_name, otherwise f_name=last filename
    file.close();  //  close any file that could be open (from previous FIND for example)

    if (!dirFile.open(directory, O_RDONLY)) {
//      sd.errorHalt("Open directory failed - possible invalid directory name");
      errorCode = 1;
#ifdef DEBUG_STORE_COMMANDS
      debugStream.print(F("stgc_0x73_h: open directory '"));
      debugStream.print(directory);
      debugStream.println(F("' failed - name may be invalid"));
#endif
      return;
    }
    file.rewind();

//    for (int index = 0; index < nMax; index++) { //SdFat file index number
//    for (uint8_t index = 0; index < nMax; index++) { //SdFat file index number

    while ((file.openNext(&dirFile, O_RDONLY)) && (index < nMax)) {

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
          debugStream.println(F("stgc_0x73_h: reached last file."));  
#endif    
          lastfile = true;
          break;
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

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x73_h: end TLIST handler."));  
#endif

}


/***** FIND command *****/
void SDstorage::stgc_0x7B_h(){
  char receiveBuffer[83] = {0};   // *0 chars + CR/LF + NULL
  uint8_t paramLen = 0;
  uint8_t num = 0;
  char fname[46];

#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7B_h: started FIND handler..."));
#endif

  paramLen = gpibBus.receiveParams(false, receiveBuffer, 83);

  // If we have received parameter data
  if (paramLen > 0) {

#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x7B_h: received parameter: "));
    debugStream.println(receiveBuffer);
#endif

    num = atoi(receiveBuffer);

    // char f_name[46];  //the filename variable
    /*  FINDs and OPENs file (num)
        Returns: string result= "A" for ASCII, "H" for HEX (Binary or Secret file), "N" for Not Found
        FIND iterates through each file in a directory until filenumber matches num
        since SdFat file index is not sequential with Tek 4050 filenames
    */

    if (!dirFile.open(directory, O_RDONLY)) {
//        sd.errorHalt("Open directory failed");
      errorCode = 3;
      return;    
    }

    // Set GPIB bus for talking (output)
//    gpibBus.setControls(DTAS);

    for (int index = 0; index < nMax; index++) { //SdFat file index number

      // while (n < nMax ) {
      file.openNext(&dirFile, O_RDONLY);
    
      // Skip directories, hidden files, and null files
      if (!file.isSubDir() && !file.isHidden()) {

        file.getName(fname, 46);

        int filenumber = atoi(fname);

        if (filenumber == num) {
          // debug print the entire file 'header' with leading space, and CR + DC3 delimiters
//          cout << F(" ") << f_name << "\r ";
          strncpy(f_name, fname, 46);
#ifdef DEBUG_STORE_COMMANDS
          debugStream.print(F("stgc_0x7B_h: found: "));
          debugStream.println(f_name);
#endif
          // all BINARY files are in HEX format
          if ((f_name[7] == 'B') && (f_name[15] == 'P')) {
            dirFile.close();  // end of iteration through all files, close directory
//            return 'B'; // BINARY PROGRAM file
            f_type = 'B';
          } else if ((f_name[7] == 'B') && (f_name[15] == 'D')) {
            dirFile.close();  // end of iteration through all files, close directory
//            return 'H'; // BINARY DATA file ** to read - parse the data_type
            f_type = 'H';
          } else if (f_name[25] == 'S') {  // f_name[25] is location of file type SECRET ASCII PROGRAM
            dirFile.close();  // end of iteration through all files, close directory
//            return 'S'; // SECRET ASCII PROGRAM file
            f_type = 'S';
          } else if (f_name[15] == 'P') {  // f_name[15] is location of file type (PROG, DATA,...)
            dirFile.close();  // end of iteration through all files, close directory
//            return 'P'; // ASCII PROGRAM file
            f_type = 'P';
          } else if (f_name[7] == 'N') {  // f_name[7] is location of file type (ASCII,BINARY,NEW, or LAST)
            dirFile.close();  // end of iteration through all files, close directory
//            return 'N'; // ASCII LAST file
            f_type = 'N';
          } else if (f_name[7] == 'L') {  // f_name[7] is location of file type (ASCII,BINARY,LAST)
            dirFile.close();  // end of iteration through all files, close directory
//            return 'L'; // ASCII LAST file
            f_type = 'L';
          } else {
            dirFile.close();  // end of iteration through all files, close directory
//            return 'D'; // ASCII DATA file, also allows other types like TEXT and LOG to be treated as DATA
            f_type = 'D';
          }
 /*
          if (ftype) {
            gpibout << f_type << "\r";
            gpibBus.sendRawData(charStream.toCharArray(), 2);
            gpibBus.sendEOI();
            gpibBus.setControls(DIDS);
            return;
          }
*/

          return;
        }

        //f_name[0]=0; // clear f_name
/*        
        charStream.flush();
        if (ftype) gpibout << ftype;
        if (file.isDir()) gpibout << '/';
        gpibout << "\r";
*/
/*
        if (file.isDir()) {
          // Indicate a directory.
//          cout << '/' << endl;
        }
*/

#ifdef DEBUG_STORE_COMMANDS
        if (file.isDir()) {
          debugStream.println(F("stgc_0x7B_h: /"));
          return;
        }
#endif

/*
        // Send info to GPIB bus
        gpibBus.sendRawData(charStream.toCharArray(), charStream.length());        
*/
      }
      file.close();  // end of iteration close **this** file

    }
//    cout << F("File ") << num << " not found" << endl;
#ifdef DEBUG_STORE_COMMANDS
    debugStream.print(F("stgc_0x7B_h: file "));
    debugStream.print(num);
    debugStream.println(F(" not found!"));
#endif

/*
    charStream.flush();
    gpibout << F("File ") << num << " not found" << endl;
    // Send info to GPIB bus
    gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
    // Send EOI
    gpibBus.sendEOI();
    // Return bus to idle
    gpibBus.setControls(DIDS);
*/    
    f_name[0] = '\0';  // clear f_name
    dirFile.close();  // end of iteration through all files, close directory

//   return 'O'; // File num NOT OPEN (or NOT FOUND)
  }
#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x7B_h: done."));
#endif  
}


/***** DIRECTORY (CD) command *****/
void SDstorage::stgc_0x73_h(){
  // limit the emulator dir name to single level "/" plus 8 characters trailing "/" plus NULL
  char dnbuffer[13] = {0};
  uint8_t j = 1;
  
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_069_h: started CD handler..."));
#endif
  
  // Read the directory name data
  gpibBus.receiveParams(false, dnbuffer, 12);   // Limit to 12 characters

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x69_h: received directory name: "));
  debugStream.println(dnbuffer);
#endif
  
  // set global directory = dir
  // directory = dir;

  // Set directory path
  directory[0] = '/';
  for (uint8_t i=0; i<strlen(dnbuffer); i++) {
    // Ignore slashes and line terminators (CR or LF)
    if ((dnbuffer[i] != 0x2F) && (dnbuffer[i] != 0x5C) && (dnbuffer[i] != 0x0A) && (dnbuffer[i] != 0x0D)) {
      // Add character to directory name
      if (j < 11) directory[j] = dnbuffer[i]; // Copy max 10 characters
      j++;
    }
  }
  // Add terminating slash and NULL
  directory[j] = '/';
  directory[j+1] = '\0';

#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x69_h: set directory name: "));
  debugStream.println(directory);
#endif
  
  f_name[0] = 0; // delete any previous filename that was open
  
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x69_h: end CD handler."));  
#endif

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
  char errstr[5] = {0};
#ifdef DEBUG_STORE_COMMANDS
  debugStream.print(F("stgc_0x7E_h: sending error code "));
  debugStream.println(errorCode);
#endif
//  charStream.flush();
//  gpibout << errorCode;
  itoa(errorCode, errstr, 2);
  // Send info to GPIB bus
//  gpibBus.sendRawData(charStream.toCharArray(), charStream.length());
  gpibBus.sendRawData(errstr, strlen(errstr));
  // Signal end of data
  gpibBus.sendEOI();
#ifdef DEBUG_STORE_COMMANDS
  debugStream.println(F("stgc_0x7E_h: done."));
#endif
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

#endif // EN_STORAGE
