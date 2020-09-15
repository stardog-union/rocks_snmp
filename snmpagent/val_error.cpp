/**
 * @file val_error.cpp
 * @author matthewv
 * @date December 18, 2011
 * @date Copyright 2011
 *
 * @brief Snmp construction of error message in value
 */

#ifndef VAL_ERROR_H
    #include "val_error.h"
#endif


static VarBindHeader gNoSuchObject={eNoSuchObject, 0};
static VarBindHeader gEndOfMibView={eEndOfMibView, 0};

static SnmpOid gVoidOid={NULL, 0};
class SnmpValError gSnmpValErrorNSO(gVoidOid, gNoSuchObject);
class SnmpValError gSnmpValErrorEOM(gVoidOid, gEndOfMibView);

/**
 * Populate response from static data
 * @date Created 12/18/11
 * @author matthewv
 */
void
SnmpValError::AppendToIovec(
    const PduSubId & ValId,
    std::vector<struct iovec> & IoArray) const
{
    struct iovec builder;

    IoArray.reserve(IoArray.size()+2);

    // variable type
    builder.iov_base=(void *)&m_ValType;
    builder.iov_len=sizeof(VarBindHeader);
    IoArray.push_back(builder);

    // variable oid
    builder.iov_base=(void *)&ValId;
    builder.iov_len=sizeof(PduSubId)+ValId.m_SubIdLen*sizeof(unsigned);
    IoArray.push_back(builder);

    return;

}   // SnmpValError::AppendToIoved
