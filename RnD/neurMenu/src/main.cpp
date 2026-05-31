#include <oneMenu/oneMenu.h>

#include "oneMenu/fmt/ansiFmt.h"
#include "oneMenu/IO/ansiOut.h"
#include "oneMenu/IO/streamOut.h"
#include "oneMenu/IO/arduino/serialOut.h"

// OutDef<oneOutput::ConsoleOut> out;

IOutDef<
  ScrollPrinter,//menu parts to use
  // FullPrinter,
  ANSIFmt,//add some ANSI colors and format to the output
  // TextFmt,
  ClearFreeFmt,//this can take a lot of burden away from user format
  DataParser<>,//put all data into characters
  CtrlChars,
  UTF8,//bypass UTF8 surrogate codes
  TextWrap,//long texts continue next line
  Clip,//keep content inside area
  ColorTrack<int>,//track color setting for device resume...
  Cursor,//track cursor position for resume...
  Gate,
  ANSIOut,//inject ansi codes into the next output device
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<20,10>,
  StaticArea<30,8>
> out;

int main() {
  out<<"Wtf!"<<endl;
  return 0;
}