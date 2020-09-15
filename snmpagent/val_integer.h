/**
 * @file val_integer.h
 * @author matthewv
 * @date November 26, 2011
 * @date Copyright 2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef VAL_INTEGER_H
#define VAL_INTEGER_H

#include <set>
#include <vector>
#include <string>
#include <stdlib.h>

#ifndef SNMP_VALUE_H
    #include "snmp_value.h"
#endif

extern struct VarBindHeader gVarInteger;
extern struct VarBindHeader gVarCounter32;
extern struct VarBindHeader gVarGauge32;


class SnmpValUnsigned32 : public SnmpValInf
{
protected:
    unsigned m_Unsigned;
    VarBindHeader & m_UnsignedType;   //!< reference to static of Integer, Counter, Gauge

public:
    SnmpValUnsigned32(const SnmpOid & Oid, VarBindHeader & UnsignedType)
    : SnmpValInf(Oid), m_Unsigned(0), m_UnsignedType(UnsignedType) {};

    SnmpValUnsigned32(const OidVector_t & Oid, VarBindHeader & UnsignedType)
    : SnmpValInf(Oid), m_Unsigned(0), m_UnsignedType(UnsignedType) {};

    virtual ~SnmpValUnsigned32() {};

    virtual void AppendToIovec(std::vector<struct iovec> & IoArray) const
    {
        struct iovec builder;

        IoArray.reserve(IoArray.size()+4);

        // variable type
        builder.iov_base=(void *)&m_UnsignedType;
        builder.iov_len=sizeof(VarBindHeader);
        IoArray.push_back(builder);

        // variable oid
        builder.iov_base=(void *)&m_SubId;
        builder.iov_len=sizeof(m_SubId);
        IoArray.push_back(builder);

        // oid array
        builder.iov_base=(void *)&m_Oid.at(0);
        builder.iov_len=sizeof(unsigned)*m_SubId.m_SubIdLen;
        IoArray.push_back(builder);

        // value
        builder.iov_base=(void *)&m_Unsigned;
        builder.iov_len=sizeof(m_Unsigned);
        IoArray.push_back(builder);
    }

    /// debug support, convert value to string for output
    virtual std::string & GetValueAsString(std::string &Output)
    {char buf[33]; snprintf(buf,33,"%u", m_Unsigned); Output=buf; return(Output);};

    /// ValueFactory support ... make everyone accept string input
    virtual void assign(const char *String)
    {if (NULL!=String) m_Unsigned=strtoul(String, NULL, 10);};

    unsigned operator++() {return(++m_Unsigned);};

    SnmpValUnsigned32 & assign(unsigned UValue) {m_Unsigned=UValue; return(*this);};

    unsigned unsigned32() const {return(m_Unsigned);};
private:
    SnmpValUnsigned32();                                          //!< disabled:  default constructor
    SnmpValUnsigned32(const SnmpValUnsigned32 & );             //!< disabled:  copy operator
    SnmpValUnsigned32 & operator=(const SnmpValUnsigned32 &);  //!< disabled:  assignment operator

};  // SnmpValUnsigned32

typedef std::shared_ptr<class SnmpValUnsigned32> SnmpValUnsigned32Ptr;


class SnmpValStaticUnsigned32 : /*public StartupListObject,*/ public SnmpValUnsigned32
{
public:
SnmpValStaticUnsigned32(/*StartupListObject ** RootPointer,*/ SnmpOid & Oid, VarBindHeader & UnsignedType)
    : /*StartupListObject(RootPointer),*/ SnmpValUnsigned32(Oid, UnsignedType) {};


private:
    SnmpValStaticUnsigned32();                                          //!< disabled:  default constructor
    SnmpValStaticUnsigned32(const SnmpValStaticUnsigned32 & );             //!< disabled:  copy operator
    SnmpValStaticUnsigned32 & operator=(const SnmpValStaticUnsigned32 &);  //!< disabled:  assignment operator

};  // SnmpValStaticUnsigned32



class SnmpValInteger : public SnmpValUnsigned32
{
public:
    SnmpValInteger(const SnmpOid & Oid)
        : SnmpValUnsigned32(Oid, gVarInteger) {};

    SnmpValInteger(const OidVector_t & Oid)
        : SnmpValUnsigned32(Oid, gVarInteger) {};

private:
    SnmpValInteger();                                          //!< disabled:  default constructor
    SnmpValInteger(const SnmpValInteger & );             //!< disabled:  copy operator
    SnmpValInteger & operator=(const SnmpValInteger &);  //!< disabled:  assignment operator

};  // SnmpValInteger

typedef std::shared_ptr<class SnmpValInteger> SnmpValIntegerPtr;

class SnmpValStaticGauge32 : public SnmpValStaticUnsigned32
{
public:
SnmpValStaticGauge32(/*StartupListObject ** RootPointer, */SnmpOid & Oid)
    : SnmpValStaticUnsigned32(/*RootPointer,*/ Oid, gVarGauge32) {};


private:
    SnmpValStaticGauge32();                                          //!< disabled:  default constructor
    SnmpValStaticGauge32(const SnmpValStaticGauge32 & );             //!< disabled:  copy operator
    SnmpValStaticGauge32 & operator=(const SnmpValStaticGauge32 &);  //!< disabled:  assignment operator

};  // SnmpValStaticGauge32


class SnmpValGauge32 : public SnmpValUnsigned32
{
public:
    SnmpValGauge32(const SnmpOid & Oid)
        : SnmpValUnsigned32(Oid, gVarGauge32) {};

    SnmpValGauge32(const OidVector_t & Oid)
        : SnmpValUnsigned32(Oid, gVarGauge32) {};

private:
    SnmpValGauge32();                                          //!< disabled:  default constructor
    SnmpValGauge32(const SnmpValGauge32 & );             //!< disabled:  copy operator
    SnmpValGauge32 & operator=(const SnmpValGauge32 &);  //!< disabled:  assignment operator

};  // SnmpValGauge32



#endif // ifndef VAL_INTEGER_H
