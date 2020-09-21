/**
 * @file val_string.h
 * @author matthewv
 * @date February 5, 2012
 * @date Copyright 2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef VAL_STRING_H
#define VAL_STRING_H

#include <set>
#include <string>
#include <vector>

#ifndef SNMP_VALUE_H
#include "snmp_value.h"
#endif

extern struct VarBindHeader gVarString;

class SnmpValString : public SnmpValInf {
protected:
  unsigned m_Length;    //!< length of string, without padding
  std::string m_String; //!< text string

public:
  SnmpValString(const SnmpOid &Oid) : SnmpValInf(Oid), m_Length(0){};

  SnmpValString(const OidVector_t &Oid) : SnmpValInf(Oid), m_Length(0){};

SnmpValString(unsigned Oid) : SnmpValInf(Oid), m_Length(0){};

  virtual ~SnmpValString(){};

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {
    struct iovec builder;
    int padding;

    IoArray.reserve(IoArray.size() + 6);

    // variable type
    builder.iov_base = (void *)&gVarString;
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

    // value ... Length
    builder.iov_base = (void *)&m_Length;
    builder.iov_len = sizeof(m_Length);
    IoArray.push_back(builder);

    // value ... String
    builder.iov_base = (void *)m_String.c_str();
    builder.iov_len = m_Length;
    IoArray.push_back(builder);

    // value ... padding
    padding = (0 == (m_Length & 0x3) ? 0 : 4 - (m_Length & 0x3));
    builder.iov_base = (void *)&gSnmpAgentPadString;
    builder.iov_len = padding;
    IoArray.push_back(builder);
  }

  /// debug support, convert value to string for output
  std::string &GetValueAsString(std::string &Output) override {
    Output = m_String;
    return (Output);
  };

  /// ValueFactory support ... make everyone accept string input
  void assign(const char *String) override {
    if (NULL != String)
      m_String = String;
    else
      m_String.clear();

    m_Length = m_String.length();
  };

  void assign(const char *String, size_t Length) {
    if (NULL != String)
      m_String.assign(String, Length);
    else
      m_String.clear();

    m_Length = m_String.length();
  };

  const char *c_str() const { return (m_String.c_str()); };

private:
  SnmpValString();                      //!< disabled:  default constructor
  SnmpValString(const SnmpValString &); //!< disabled:  copy operator
  SnmpValString &
  operator=(const SnmpValString &); //!< disabled:  assignment operator

}; // SnmpValString

typedef std::shared_ptr<class SnmpValString> SnmpValStringPtr;

#endif // ifndef VAL_STRING_H
