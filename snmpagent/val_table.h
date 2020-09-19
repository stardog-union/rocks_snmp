/**
 * @file val_table.h
 * @author matthewv
 * @date November 26, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef VAL_TABLE_H
#define VAL_TABLE_H

#include <set>
#include <string>
#include <vector>

#include "snmp_agent.h"
#include "val_integer.h"

class SnmpValTable {
protected:
  SnmpAgent &m_Agent;        //!< parent object
  OidVector_t m_TablePrefix; //!< after general prefix, add this for table

  SnmpValIntegerPtr m_RowCount; //!< number of rows in table
  std::vector<SnmpValUnsigned32Ptr>
      m_RowXlate; //!< cross ref of row id to index

public:
  SnmpValTable(SnmpAgent &Agent, SnmpOid &TablePrefix);

  virtual ~SnmpValTable(){};

  // add another index to end of row list
  bool AddRow(int RowIndex);

  SnmpAgent &GetSnmpAgent() { return (m_Agent); };

  // void AppendToIovec(std::vector<struct iovec> & IoArray) override;

  /// debug support, convert value to string for output
  // virtual std::string & GetValueAsString(std::string &Output);

private:
  SnmpValTable();                     //!< disabled:  default constructor
  SnmpValTable(const SnmpValTable &); //!< disabled:  copy operator
  SnmpValTable &
  operator=(const SnmpValTable &); //!< disabled:  assignment operator

}; // SnmpValTable

#endif // ifndef VAL_TABLE_H
