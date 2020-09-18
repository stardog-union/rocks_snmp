/**
 * @file val_integer64.h
 * @author matthewv
 * @date January 18, 2012
 * @date Copyright 2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef VAL_INTEGER64_H
#define VAL_INTEGER64_H

#include <set>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifndef SNMP_VALUE_H
#include "snmp_value.h"
#endif

extern struct VarBindHeader gVarCounter64;

// typedef unsigned long long int uint64_t;

class SnmpValUnsigned64 : public SnmpValInf {
protected:
  uint64_t m_Unsigned64;
  VarBindHeader
      &m_UnsignedType; //!< reference to static of Integer, Counter, Gauge

public:
  SnmpValUnsigned64(VarBindHeader &UnsignedType)
      : m_Unsigned64(0), m_UnsignedType(UnsignedType){};

  SnmpValUnsigned64(const SnmpOid &Oid, VarBindHeader &UnsignedType)
      : SnmpValInf(Oid), m_Unsigned64(0), m_UnsignedType(UnsignedType){};

  SnmpValUnsigned64(const OidVector_t &Oid, VarBindHeader &UnsignedType)
      : SnmpValInf(Oid), m_Unsigned64(0), m_UnsignedType(UnsignedType){};

  SnmpValUnsigned64(unsigned ID, VarBindHeader &UnsignedType)
      : SnmpValInf(ID), m_Unsigned64(0), m_UnsignedType(UnsignedType){};

  virtual ~SnmpValUnsigned64(){};

  virtual void AppendToIovec(std::vector<struct iovec> &IoArray) const {
    struct iovec builder;

    IoArray.reserve(IoArray.size() + 4);

    // variable type
    builder.iov_base = (void *)&m_UnsignedType;
    builder.iov_len = sizeof(VarBindHeader);
    IoArray.push_back(builder);

    // variable oid
    builder.iov_base = (void *)&m_SubId;
    builder.iov_len = sizeof(m_SubId);
    IoArray.push_back(builder);

    // oid array
    builder.iov_base = (void *)&m_Oid.at(0);
    builder.iov_len = sizeof(unsigned) * m_SubId.m_SubIdLen;
    IoArray.push_back(builder);

    // value
    builder.iov_base = (void *)&m_Unsigned64;
    builder.iov_len = sizeof(m_Unsigned64);
    IoArray.push_back(builder);
  }

  /// debug support, convert value to string for output
  virtual std::string &GetValueAsString(std::string &Output) {
    char buf[65];
    snprintf(buf, 65, "%lu", m_Unsigned64);
    Output = buf;
    return (Output);
  };

  /// ValueFactory support ... make everyone accept string input
  virtual void assign(const char *String) {
    if (NULL != String)
      m_Unsigned64 = strtoull(String, NULL, 10);
  };

  uint64_t operator++() { return (++m_Unsigned64); };

  SnmpValUnsigned64 &assign(uint64_t UValue64) {
    m_Unsigned64 = UValue64;
    return (*this);
  };

  SnmpValUnsigned64 &assign(unsigned UValue) {
    m_Unsigned64 = (uint64_t)UValue;
    return (*this);
  };

  uint64_t unsigned64() const { return (m_Unsigned64); };

private:
  SnmpValUnsigned64(); //!< disabled:  default constructor
  SnmpValUnsigned64(const SnmpValUnsigned64 &); //!< disabled:  copy operator
  SnmpValUnsigned64 &
  operator=(const SnmpValUnsigned64 &); //!< disabled:  assignment operator

}; // SnmpValUnsigned64

typedef std::shared_ptr<class SnmpValUnsigned64> SnmpValUnsigned64Ptr;


class SnmpValCounter64 : public SnmpValUnsigned64 {
public:
  SnmpValCounter64() : SnmpValUnsigned64(gVarCounter64){};

  SnmpValCounter64(const SnmpOid &Oid)
      : SnmpValUnsigned64(Oid, gVarCounter64){};

  SnmpValCounter64(const OidVector_t &Oid)
      : SnmpValUnsigned64(Oid, gVarCounter64){};

private:
  SnmpValCounter64(const SnmpValCounter64 &); //!< disabled:  copy operator
  SnmpValCounter64 &
  operator=(const SnmpValCounter64 &); //!< disabled:  assignment operator

}; // SnmpValCounter64

class SnmpValCounterPtr64 : public SnmpValUnsigned64 {
protected:
  uint64_t *m_Unsigned64Ptr;

public:
  SnmpValCounterPtr64(const SnmpOid &Oid, uint64_t *Unsigned64Ptr = NULL)
      : SnmpValUnsigned64(Oid, gVarCounter64), m_Unsigned64Ptr(Unsigned64Ptr) {
    if (NULL == m_Unsigned64Ptr)
      m_Unsigned64Ptr = &m_Unsigned64;
  };

  SnmpValCounterPtr64(const OidVector_t &Oid, uint64_t *Unsigned64Ptr = NULL)
      : SnmpValUnsigned64(Oid, gVarCounter64), m_Unsigned64Ptr(Unsigned64Ptr) {
    if (NULL == m_Unsigned64Ptr)
      m_Unsigned64Ptr = &m_Unsigned64;
  };

  SnmpValCounterPtr64(unsigned ID, uint64_t *Unsigned64Ptr = NULL)
      : SnmpValUnsigned64(ID, gVarCounter64), m_Unsigned64Ptr(Unsigned64Ptr) {
    if (NULL == m_Unsigned64Ptr)
      m_Unsigned64Ptr = &m_Unsigned64;
  };

  void AppendToIovec(std::vector<struct iovec> &IoArray) const override {
    SnmpValUnsigned64::AppendToIovec(IoArray);
    struct iovec builder;

    // value
    IoArray.pop_back();
    builder.iov_base = (void *)m_Unsigned64Ptr;
    builder.iov_len = sizeof(m_Unsigned64);
    IoArray.push_back(builder);
  };

  void SetPointer(uint64_t *Pointer) {
    if (NULL != Pointer)
      m_Unsigned64Ptr = Pointer;
  };

private:
  SnmpValCounterPtr64(); //!< disabled:  default constructor
  SnmpValCounterPtr64(
      const SnmpValCounterPtr64 &); //!< disabled:  copy operator
  SnmpValCounterPtr64 &
  operator=(const SnmpValCounterPtr64 &); //!< disabled:  assignment operator

}; // SnmpValCounterPtr64

#endif // ifndef VAL_INTEGER_H
