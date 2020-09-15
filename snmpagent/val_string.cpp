/**
 * @file val_string.h
 * @author matthewv
 * @date February 5, 2012
 * @date Copyright 2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef VAL_STRING_H
#include "val_string.h"
#endif

struct VarBindHeader gVarString = {eOctetString, 0};

/**
 * Initialize the data members.
 * @date Created 07/04/11
 * @author matthewv
 */
