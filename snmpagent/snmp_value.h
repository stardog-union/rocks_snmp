/**
 * @file snmp_value.h
 * @author matthewv
 * @date August 31, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX value objects (rfc 2741, January 2000)
 */

#ifndef SNMP_VALUE_H
#define SNMP_VALUE_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "util/logging.h"

#include "snmp_pdu.h"
#include "statemachine.h"

/**
 * AgentX PDU type codes
 * @date 07/09/11
 */
enum ValueTypeCodes_e {
  eInteger = 2,
  eOctetString = 4,
  eNull = 5,
  eObjectIdentifier = 6,

  eIpAddress = 64,
  eCounter32 = 65,
  eGauge32 = 66,
  eTimeTicks = 67,
  eOpaque = 68,
  eCounter64 = 70,

  eNoSuchObject = 128,
  eNoSuchInstance = 129,
  eEndOfMibView = 130,
};

/**
 * AgentX Value Headers
 * @date January 2000, rfc 2741
 */
struct VarBindHeader {
  unsigned short m_Type; //!< enum ValueTypeCodes
  unsigned short m_Reserved1;
} __attribute__((packed));

extern struct VarBindHeader gVarNoSuchObject;
extern struct VarBindHeader gVarendOfMibView;

struct SnmpOid {
  const unsigned *m_Oid; //!< pointer to static list of OID values
  size_t m_OidLen;       //!< length of OID vector
};

typedef std::vector<unsigned> OidVector_t;

typedef std::shared_ptr<class SnmpValInf> SnmpValInfPtr;

/**
 * Interface for an SNMP Variable (absolute base class)
 * @date created 08/31/11
 */
class SnmpValInf : public StateMachine
{
public:
  /// list of state nodes
  enum SnmpValNode_e {
    SNMP_NODE_DATAGOOD = 1011,  //!< data in this object is current
    SNMP_NODE_DATASTALE = 1012, //!< data is stale, ask for new
    SNMP_NODE_UPDATING = 1013,  //!< awaiting requested update
  };

  /// list of state edges
  enum SnmpValEdge_e {
    SNMP_EDGE_REQUEST_DATA =
        1021,                   //!< please update my data, someone wants good
    SNMP_EDGE_DATA_GOOD = 1022, //!< data update arrived
    SNMP_EDGE_DATA_STALE =
        1023, //!< outside source telling us this is old data,
              //!<  request new as needed
  };

  /*************************************************************
   *  Member objects
   *************************************************************/
public:
protected:
  PduSubId m_SubId;         //!< subid for response
  OidVector_t m_Oid;        //!< oid suffix for this var
  bool m_PrefixSet;         //!< InsertPrefix() has been called

private:
  /*************************************************************
   *  Member functions
   *************************************************************/
public:
  SnmpValInf();
  SnmpValInf(const SnmpOid &Suffix);
  SnmpValInf(const OidVector_t &Suffix);
  SnmpValInf(unsigned Suffix);

  virtual ~SnmpValInf(){};

  /// initialization common to all constructors
  void Init();

  std::shared_ptr<SnmpValInf> GetSnmpValInfPtr() {
    return std::static_pointer_cast<SnmpValInf>(GetStateMachinePtr());
  }

  /// default construtor used, set suffix now
  void SetSuffix(const SnmpOid &Suffix);

  /// add prefix to oid to ease search / compare
  void InsertPrefix(const OidVector_t &OidPrefix);

  /// tables need several items added to value oid
  void InsertTablePrefix(const OidVector_t &OidAgentPrefix,
                         const OidVector_t &OidTablePrefix,
                         const OidVector_t &OidAreaPrefix,
                         const OidVector_t &OidRowIdSuffix);

  /// static function to create various snmp_types
  static SnmpValInfPtr ValueFactory(ValueTypeCodes_e Type);

  /// append this variable to output stream
  virtual void AppendToIovec(std::vector<struct iovec> &IoArray) = 0;

  bool operator<(const SnmpValInf &rhs) const {return m_Oid < rhs.m_Oid;}


  /// debug support, convert value to string for output
  virtual std::string &GetValueAsString(std::string &Output) = 0;

  /// ValueFactory support ... make everyone accept string input
  virtual void assign(const char *) = 0;

  //
  // state machine related methods
  //

  /// used to pause reply while fresh data fetched in specialized values
  virtual bool IsDataReady(StateMachinePtr & Notify);

  /// Public routine to receive Edge notification
  //    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &
  //    Caller, bool PreNotify);

  /// generic dump of value ("<oid> = GetvalueAsString()")
  virtual void SnmpDump() const;

protected:
  /// common routine to add header info to output stream
  virtual void AddHeaderIovec(std::vector<struct iovec> &IoArray){};

private:
  SnmpValInf(const SnmpValInf &);            //!< disabled:  copy operator
  SnmpValInf &operator=(const SnmpValInf &); //!< disabled:  assignment operator

}; // class SnmpValInf


class SnmpValPtrCompare {
public:
  bool operator()(const SnmpValInfPtr &P1, const SnmpValInfPtr &P2) const {
    return *P1 < *P2;
  };
}; // SnmpValPtrCompare

/**
 * "Clean" set of snmp variables without concern for storage method
 * @date Created 10/01/11
 */

typedef std::set<SnmpValInfPtr, SnmpValPtrCompare> SnmpValPtrSet_t;


/**
 * Class used solely in searching internal SnmpValInf sets
 * @date 10/02/11
 * @author matthewv
 */
class SnmpValLookup : public SnmpValInf {
public:
  SnmpValLookup(SnmpOid &Oid) : SnmpValInf(Oid){};

  virtual ~SnmpValLookup(){};

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {};

  /// debug support, convert value to string for output
  std::string &GetValueAsString(std::string &Output) override {
    return (Output);
  };
  void assign(const char *String) override {};

}; // SnmpValLookup

#endif // ifndef SNMP_VALUE_H
