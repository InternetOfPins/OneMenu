/**
 * @file oneMenu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi based menu system
 * @version 5
*/

#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#endif

//IOP ecosystem includes
#include <hapi/hapi.h>
#include <oneList/oneList.h>
#include <oneOutput/oneOutput.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>

using hapi::Nil;

//menu include
#include "oneMenu/sys/base.h"
#include "sys/fields.h"
#include "sys/formats.h"
#include "body/static.h"
#include "item.h"
#include "out.h"
#include "menu.h"

