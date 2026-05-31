/**
 * @file oneMenu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi based menu system
 * @version 5
*/

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#undef max
#endif

#ifdef __AVR__
  #include <assert.h>
  #include "menu/sys/platform/avr/avr_std.h"
#else
  #include <iostream>
  #include <cstdint>
  #include <cassert>
  #include <type_traits>
  #include <utility>
  #include <cstring>
  #include <cstdlib>
  #include <cstdio>
  #include <limits>
  #include <algorithm>
#endif

//IOP ecosystem includes
#include <hapi/hapi.h>
// #include <oneList/oneList.h>
#include <oneOutput/oneOutput.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>

using hapi::Nil;

//menu include
#include "oneMenu/sys/enums.h"
#include "oneMenu/sys/base.h"
#include "sys/fields.h"
#include "sys/formats.h"
#include "body/static.h"
#include "out.h"
#include "item.h"
#include "menu.h"
#include "nav.h"

