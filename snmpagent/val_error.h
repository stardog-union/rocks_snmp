/**
 * @file val_error.h
 * @author matthewv
 * @date November 26, 2011
 * @date Copyright 2011-2012
 *
 * @brief Snmp construction of error message in value
 */

#ifndef VAL_ERROR_H
#define VAL_ERROR_H

#include <string>
#include <vector>
#include <stdio.h>

#ifndef SNMP_VALUE_H
#include "snmp_value.h"
#endif

class SnmpValError : public SnmpValInf {
protected:
  VarBindHeader &m_ValType; //!< reference to static of error value

public:
  SnmpValError(const SnmpOid &Oid, VarBindHeader &ErrorType)
      : SnmpValInf(Oid), m_ValType(ErrorType){};

  SnmpValError(const OidVector_t &Oid, VarBindHeader &ErrorType)
      : SnmpValInf(Oid), m_ValType(ErrorType){};

  virtual ~SnmpValError(){};

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {};
  void AppendToIovec(const PduSubId &ValId,
                             std::vector<struct iovec> &IoArray) const;

  /// debug support, convert value to string for output
  std::string &GetValueAsString(std::string &Output) override {
    char buf[33];
    snprintf(buf, 33, "%u", (unsigned)m_ValType.m_Type);
    Output = buf;
    return (Output);
  };

  void assign(const char *String) override {};

private:
  SnmpValError();                     //!< disabled:  default constructor
  SnmpValError(const SnmpValError &); //!< disabled:  copy operator
  SnmpValError &
  operator=(const SnmpValError &); //!< disabled:  assignment operator

}; // SnmpValError

extern class SnmpValError gSnmpValErrorNSO;
extern class SnmpValError gSnmpValErrorEOM;

#endif // ifndef VAL_INTEGER_H
