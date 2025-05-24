/*
  This file allows the library to be given priority over the globally installed "SdFat" library
  by the Arduino library discovery system:
  https://forum.arduino.cc/t/how-to-select-a-specific-version-of-a-library/1383714

  This file is intentionally left empty (except for this comment).

  Please copy this file to the location of the SdFat directory included with your Teensy board
  package:

  Windows:
  \Users\{username}\Appdata\Local\Arduino15\packages\teensy\hardware\avr\{1.59.0}\libraries\SdFat\src\

  Linux:
  /home/{username}/.arduino15/packages/teensy/hardware/avr/{1.59.0}/libraries/SdFat/src/

  {username} is your user name
  {1.59.0}   is the current board package version - substitute number of the installed version

  The neccessary logic to use this file has already been included in the file AR488_Store_Tek.h
  header file:

  #ifdef __IMXRT1062__
    #include <Teensy_SdFat.h>
  #endif
  #include <SdFat.h>
  #include <SdFatConfig.h>
  #include <sdios.h>

  When compiling for another board, the Teensy file will be ignored and the standard SdFat
  library used instead.

  The reference to the Teensy_SdFat.h file must be always placed before any other SdFat library
  components otherwise the compiler will search the default library path first.

*/
