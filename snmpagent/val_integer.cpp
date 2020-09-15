/**
 * @file val_integer.cpp
 * @author matthewv
 * @date November 26, 2011
 * @date Copyright 2012
 *
 * @brief Integer based snmp value
 */

#ifndef VAL_INTEGER_H
#include "val_integer.h"
#endif

struct VarBindHeader gVarInteger = {eInteger, 0};
struct VarBindHeader gVarCounter32 = {eCounter32, 0};
struct VarBindHeader gVarGauge32 = {eGauge32, 0};

/**
 * Initialize the data members.
 * @date Created 07/04/11
 * @author matthewv
 */
