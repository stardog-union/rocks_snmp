/**
 * @file snmp_agent.h
 * @author matthewv
 * @date July 2, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp AgentX admin / service functions
 */

#ifndef SNMP_AGENT_H
#define SNMP_AGENT_H

#include <mutex>
#include <queue>

#include "tcp_event.h"

#include "snmp_pdu.h"
#include "snmp_value.h"

#ifndef STARTUP_LIST_H
//    #include "startup_list.h"
#endif

typedef std::shared_ptr<class SnmpAgent> SnmpAgentPtr;

/**
 * Manage connection and admin messages of snmp AgentX agent side
 *
 * Use events/state from TcpEventSocket, and some custom

\dot
digraph TcpEventSocket {
a [ label="Closed"];
b [ label="Connecting"];
c [ label="Established"];
d [ label="Error"];
e [ label="Timeout"];
f [ label="Retry wait"];
g [ label="Opened"];
h [ label="Registered"];
i [ label="Deregistered"];

a -> b [label="address given"];

b -> d [label="error"];
b -> e [label="timeout"];
b -> c [label="writable"];

c -> g [label="open pdu good"];
c -> d [label="error"];
c -> e [label="timeout"];

d -> f [label="force close"];

e -> f [label="force close"];

f -> b [label="wait done"];

g -> d [label="error"];
g -> e [label="timeout"];
g -> h [label="reg. good"];

h -> i [label="shutdown"];

i -> a [label="pdu closed"];
}
\enddot
 */

typedef std::vector<unsigned> OidVector_t;

class SnmpAgent : public TcpEventSocket {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  /// list of state nodes
  enum SnmpAgentNode_e {
    // some Node values from TcpEvent used
    SA_NODE_OPENED = 1201,     //!< open pdu accepted by master
    SA_NODE_REGISTERED = 1202, //!< register pdu accepted by master

  };

  struct SnmpAgentId {
    const unsigned *m_AgentPrefix; //!< pointer to array of OID values
    size_t m_AgentPrefixLen;       //!< count of values in AgentPrefix array
    const char *m_AgentName;       //!< pointer to static string with agent name
  };

protected:
  // RWLockControl m_RWLock;           //!< protection for OidSet
  OidVector_t m_OidPrefix;  //!< OID identifying base of tree for this agent
  std::string m_AgentName;  //!< string passed to master
  SnmpValPtrSet_t m_OidSet; //!< collection of OIDs within prefix

  unsigned m_SessionId;
  unsigned m_PacketId;           //!< previous IP packet id
  PduInboundBufPtr m_InboundPtr; //!< all traffic from master goes here

  //    StartupListObject ** m_StartupList; //!< snmp variables with static
  //    initialization

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  SnmpAgent(SnmpAgentId &AgentId, unsigned IpHostOrder, unsigned PortHostOrder);
  //              StartupListObject ** Startup=NULL);

  virtual ~SnmpAgent();

  /// debug
  void Dump();

  //
  // reader writer modification
  //
  /// get data from handle
  void Read(PduInboundBufPtr &Buffer) {
    ReaderWriterBufPtr ptr;
    ptr = Buffer;
    ReaderWriter::Read(ptr);
  };

  /// write data to handle
  // void Write(ReaderWriterBufPtr & Buffer);

  //
  // statemachine callbacks
  //
  bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &Caller,
                        bool PreNotify) override;

  //
  // meventobj callbacks
  //

  /// allows initialization by independent event thread where appropriate
  virtual void ThreadInit(MEventMgrPtr &Mgr);

  /// External callback used when time value expires
  virtual void TimerCallback();

  /// External callback when handle contains error flag
  virtual bool ErrorCallback();

  /// External callback when handle contains HUP and/or RDHUP flag
  virtual bool CloseCallback(int);

  //
  // accessors
  //

  /// give out next "unique" packet id, rollover expected
  unsigned GetNextPacketId() { return (__sync_add_and_fetch(&m_PacketId, 1)); };

  const OidVector_t &GetOidPrefix() const { return (m_OidPrefix); };

  unsigned *GetOidPrefixPtr() { return (&m_OidPrefix.at(0)); };

  size_t GetOidPrefixLen() { return (m_OidPrefix.size()); };

  const char *GetAgentName() { return (m_AgentName.c_str()); };

  size_t GetAgentNameLen() { return (m_AgentName.length()); };

  unsigned GetSessionId() { return (m_SessionId); };

  // RWLockControl & GetRWLockControl() {return(m_RWLock);};

  //
  // other functions
  //

  /// add a variable to oid list
  bool AddVariable(SnmpValInfPtr &Variable, bool WaitLock = true);
#if 0
    bool AddVariable(SnmpValInf & Variable, bool WaitLock=true)
    {
        SnmpValInfPtr new_ptr;

        new_ptr=Variable;
        return(AddVariable(new_ptr, WaitLock));
    };
#endif
  /// look up values and add to output vector
  bool GetVariables(std::vector<struct iovec> &ResponseVec,
                    const PduSubId &StartId, const PduSubId &EndId,
                    bool GetNext, unsigned short &Error,
                    StateMachinePtr & Notify);

protected:
  /// advance state machine, send snmp open pdu
  bool ProcessConnection();

  /// master has sent a request or a response to us
  bool ProcessInboundPdu();

  /// master has sent a response
  bool ProcessResponsePdu();

  /// master has sent a Get or GetNext pdu
  bool ProcessRequestPdu();

private:
  SnmpAgent();                  //!< disabled:  use 2 integer constructor
  SnmpAgent(const SnmpAgent &); //!< disabled:  copy operator
  SnmpAgent &operator=(const SnmpAgent &); //!< disabled:  assignment operator

}; // SnmpAgent

#endif // ifndef SNMP_AGENT_H
