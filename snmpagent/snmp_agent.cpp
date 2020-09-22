/**
 * @file snmp_agent.cpp
 * @author matthewv
 * @date July 2, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implementation of snmp AgentX admin / service functions
 */

#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>

#include "meventmgr.h"
#include "snmp_agent.h"
#include "snmp_closepdu.h"
#include "snmp_getresponse.h"
#include "snmp_openpdu.h"
#include "snmp_registerpdu.h"
#include "val_error.h"
#include "logging.h"

/**
 * Initialize the data members.
 * @date 07/04/11  matthewv  Created
 * @author matthewv
 */
SnmpAgent::SnmpAgent(
    SnmpAgentId &AgentId,   //!< identification info sent to master
    unsigned IpHostOrder,   //!< zero or host order ip address
    unsigned PortHostOrder) //!< zero or host order tcp port
    //    StartupListObject ** Startup)
    : m_SessionId(0), m_PacketId(0) //, m_StartupList(Startup)
{
  unsigned loop;

  // SnmpAgent member data
  m_OidPrefix.reserve(AgentId.m_AgentPrefixLen);
  for (loop = 0; loop < AgentId.m_AgentPrefixLen; ++loop)
    m_OidPrefix.push_back(AgentId.m_AgentPrefix[loop]);

  if (NULL != AgentId.m_AgentName)
    m_AgentName = AgentId.m_AgentName;

  // TcpEventSocket member data
  m_NetIp = htonl(IpHostOrder);
  m_NetPort = htons(PortHostOrder);

  m_InboundPtr = std::make_shared<PduInboundBuf>();
  if (NULL == m_InboundPtr.get())
    Logging(LOG_ERR, "%s: Out of memory 2.", __func__);

  return;

} // SnmpAgent::SnmpAgent

/**
 * Release resources
 * @date 07/04/11  matthewv  Created
 * @author matthewv
 */
SnmpAgent::~SnmpAgent() {} // SnmpAgent::~SnmpAgent

/**
 * Add a new snmp variable to our list
 *  The pointer is NOT set up for auto delete.
 * @date Created 10/01/11
 * @author matthewv
 * @returns true on successful insert to list
 */
bool SnmpAgent::AddVariable(
    SnmpValInfPtr &Variable, //!< non-NULL pointer to variable to add
    bool LockWait)           //!< true to wait on read/write lock
{
  //    RWLock lock;
  bool ret_flag;

  ret_flag = true;

  // this lock could dead lock thread if a read is partially
  //  sent and a write command attempted ... hence TryWrite
#if 0
    if (LockWait)
        ret_flag=lock.WaitWriteLock(m_RWLock);
    else
        ret_flag=lock.TryWriteLock(m_RWLock);
#endif

  if (ret_flag && NULL != Variable.get()) {
    std::pair<SnmpValPtrSet_t::iterator, bool> ins_ret;

    Variable->InsertPrefix(m_OidPrefix);
    ins_ret = m_OidSet.insert(Variable);
    ret_flag = ins_ret.second;
    if (!ret_flag)
      Logging(LOG_ERR, "%s: failed to add snmp variable", __func__);
  } // if

  return (ret_flag);

} // SnmpAgent::AddVariable

/**
 * One time call as object becomes active on thread.
 *  Used to perform delayed init of snmp agent items
 * @date 10/02/11 Created
 * @author matthewv
 */
void SnmpAgent::ThreadInit(MEventMgrPtr &Mgr) {
#if 0
    StartupListObject * startup;

    // load static defined snmp variables
    if (NULL!=m_StartupList)
    {
        for (startup=*m_StartupList; NULL!=startup; startup=startup->GetNext())
        {
            SnmpValStatic * var;
            // (not work) var=dynamic_cast<SnmpValStatic *>(startup);
            var=(SnmpValStatic *)startup;

            if (NULL!=var)
            {
                SnmpValInfPtr var_inf;

                var_inf=var;
                AddVariable(var_inf);
            }   // if
            else
            {
                Logging(LOG_ERR, "%s: dynamic_cast failure", __func__);
            }   // else
        }   // if
    }   // if
#endif
  // (changed) TcpEventSocket contains code to start the socket if
  //  the ip and port are already set
  TcpEventSocket::ThreadInit(Mgr);

  Connect();

  return;

} // SnmpAgent::ThreadInit

/**
 * A read or write request took too long, or inactive too long
 * @date Created 07/04/11
 * @author matthewv
 */
void SnmpAgent::TimerCallback() {
  // try to activate snmp connection again
  if (TS_NODE_CLOSED == GetState()) {
    Connect();
  } // if
  else {
    TcpEventSocket::TimerCallback();
  } // else

} // SnmpAgent::TimerCallback

/**
 * event logic saw an error on the file descriptor
 * @date Created 07/04/11
 * @author matthewv
 * @returns true if simultaneous Read/Write callbacks should proceed
 */
bool SnmpAgent::ErrorCallback() {
  bool ret_flag;

  ret_flag = TcpEventSocket::ErrorCallback();

  // setup a call to reattach
  if (TS_NODE_CLOSED == GetState()) {
    SetTimerMS(30000);
  } // if

  return (ret_flag);

} // SnmpAgent::ErrorCallback

/**
 * Snmp master must have closed / restarted
 * @date Created 02/12/12
 * @author matthewv
 * @returns true if simultaneous Read/Write callbacks should proceed
 */
bool SnmpAgent::CloseCallback(int EpollFlags) {
  bool ret_flag;

  ret_flag = TcpEventSocket::CloseCallback(EpollFlags);

  // setup a call to reattach
  if (TS_NODE_CLOSED == GetState()) {
    SetTimerMS(30000);
  } // if

  return (ret_flag);

} // SnmpAgent::CloseCallback

/**
 * @brief Turn edge notifications into State changes
 *
 * @date Created 07/04/11
 * @author matthewv
 * @returns  true if edge handled to state transition
 */
bool SnmpAgent::EdgeNotification(
    unsigned int EdgeId, //!< what just happened, what graph edge are we walking
    StateMachinePtr &Caller, //!< what state machine object initiated the edge
    bool PreNotify) //!< for watchers, is the before or after owner processes
{
  bool used;

  used = false;

  // only care about our own events
  if (this == Caller.get()) {
    switch (EdgeId) {
    case TcpEventSocket::TS_EDGE_CONNECTED:
      // TcpEventSocket sets ESTABLISHED state
      used = TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
      if (used)
        used = ProcessConnection();
      break;

    // a write buffer fully sent
    case RW_EDGE_SENT:
      used = TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
      // ProcessCurrentResponse();
      break;

    // a PDU has arrived from the master
    case RW_EDGE_RECEIVED:
      used = ProcessInboundPdu();
      break;

    default:
      // send down a level.  If not used then it is an error
      used = TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
      if (!used) {
        Logging(LOG_ERR, "SnmpAgent::%s: unknown edge value passed [EdgeId=%u, State=%u]",
                __func__, EdgeId, GetState());
        SendEdge(RW_EDGE_ERROR);
      } // if
      break;
    } // switch
  }   // if

  else {
    used = TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
  } // else

  return (used);

} // SnmpAgent::EdgeNotifications

/**
 * TCP connection established, send PDU Open to snmp master
 *
 * @date created 07/04/11
 * @author matthewv
 * @returns false if problems in message setup, true on success
 */
bool SnmpAgent::ProcessConnection() {
  bool ret_flag;
  ReaderWriterBufPtr ptr;

  ret_flag = true;

  // set up the read side now (the response buffer)
  m_InboundPtr->Reset();
  Read(m_InboundPtr);

  // send the OpenPdu message, buffer will auto delete
  ptr = std::make_shared<OpenPDU>(*this);
  Write(ptr);

  return (ret_flag);

} // SnmpAgent::ProcessConnection

/**
 * PDU has arrived from master, do something with it
 *
 * @date created 07/09/11
 * @author matthewv
 * @returns true if we know what to do with message, false if we do not
 */
bool SnmpAgent::ProcessInboundPdu() {
  bool ret_flag;

  ret_flag = true;

  switch (m_InboundPtr->GetPduType()) {
  case eResponsePDU:
    ret_flag = ProcessResponsePdu();
    break;

  case eGetPDU:
  case eGetNextPDU:
    ret_flag = ProcessRequestPdu();
    break;

  default:
    ret_flag = false;
    Logging(LOG_ERR, "%s: Unknown pdu type %d seen", __func__,
            m_InboundPtr->GetPduType());
    break;
  } // switch

  // establish for next packet
  m_InboundPtr->Reset();

  // zero length pdu setup infinite loop that kills stack
  if (ret_flag)
    Read(m_InboundPtr);

  return (ret_flag);

} // SnmpAgent::ProcessInboundPdu

/**
 * PDU has arrived from master, do something with it
 *
 * @date created 07/09/11
 * @author matthewv
 * @returns true if we know what to do with message, false if we do not
 */
bool SnmpAgent::ProcessResponsePdu() {
  bool ret_flag;
  PduResponse *resp;

  ret_flag = true;
  resp = (PduResponse *)m_InboundPtr->GetInboundBuf();

  switch (m_InboundPtr->GetResponseType()) {
  case eOpenPDU:
    if (TS_NODE_ESTABLISHED == GetState() && 0 == resp->m_Error) {
      ReaderWriterBufPtr ptr;

      ret_flag = true;
      SetState(SA_NODE_OPENED);

      // this is huge.  Must save and return on every other packet we generate
      m_SessionId = m_InboundPtr->GetHeader().m_SessionID;

      // send the RegisterPdu message, buffer will auto delete
      //                ptr.reset(new ClosePDU(*this, 5));
      ptr = std::make_shared<RegisterPDU>(*this);
      Write(ptr);
    } // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: OpenPDU response seen in %u state (error %u)",
              __func__, GetState(), (unsigned)resp->m_Error);
    } // else
    break;

  case eRegisterPDU:
    if (SA_NODE_OPENED == GetState() && 0 == resp->m_Error) {
      ReaderWriterBufPtr ptr;

      ret_flag = true;
      SetState(SA_NODE_REGISTERED);
    } // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: RegisterPDU response seen in %u state", __func__,
              GetState());
    } // else
    break;

  default:
    ret_flag = false;
    Logging(LOG_ERR, "%s: Unknown pdu response %d seen", __func__,
            m_InboundPtr->GetResponseType());
    break;
  } // switch

  return (ret_flag);

} // SnmpAgent::ProcessResponsePdu

/**
 * A request pdu has arrived from master, do something with it
 *
 * @date created 10/02/11
 * @author matthewv
 * @returns true if we know what to do with message, false if we do not
 */
bool SnmpAgent::ProcessRequestPdu() {
  bool ret_flag;

  ret_flag = true;

  switch (m_InboundPtr->GetHeader().m_Type) {
  case eGetPDU:
  case eGetNextPDU:
    if (SA_NODE_REGISTERED == GetState()) {
      ReaderWriterBufPtr ptr;

      ret_flag = true;

      // create the response from the request
      ptr = std::make_shared<GetResponsePDU>(*this, m_InboundPtr);
      Write(ptr);
    } // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: GetPDU/GetNextPDU request seen in %u state",
              __func__, GetState());
    } // else
    break;

  default:
    ret_flag = false;
    Logging(LOG_ERR, "%s: Unknown pdu request %d seen", __func__,
            m_InboundPtr->GetHeader().m_Type);
    break;
  } // switch

  return (ret_flag);

} // SnmpAgent::ProcessRequestPdu

/**
 * Look up one or more variables in our collection
 * @date Created 10/02/11
 * @author matthewv
 * @returns true if variable(s) ready to send
 */
bool SnmpAgent::GetVariables(
    std::vector<struct iovec>
        &ResponseVec,        //!< vector to append returned values
    const PduSubId &StartId, //!< start of lookup range
    const PduSubId &EndId,   //!< end of lookup range (or zero subid count)
    bool GetNext,            //!< start/end from a GetNext request
    unsigned short &Error,   //!< [output] type of lookup error
    StateMachinePtr & Notify)                  //!< if data delayed, notify
//    completion
{
  bool send_now;
  std::vector<unsigned> vec;
  SnmpOid oid;
  SnmpValPtrSet_t::iterator it;
  SnmpValInfPtr ptr;

  send_now = true;

  oid.m_Oid = (unsigned *)(&StartId + 1);
  oid.m_OidLen = StartId.m_SubIdLen;

  SnmpValInfPtr lookup_ptr = std::make_shared<SnmpValLookup>(oid);

  if (!GetNext) {
    it = m_OidSet.find(lookup_ptr);

    if (m_OidSet.end() != it) {
      (*it)->AppendToIovec(ResponseVec);
      ptr = *it; // un const
      send_now=send_now && ptr->IsDataReady(Notify);
    }            // if
    else {
      // push VarBindHeader with eNoSuchObject
      gSnmpValErrorNSO.AppendToIovec(StartId, ResponseVec);
    } // else
  }   // if
  else {
    it = m_OidSet.upper_bound(lookup_ptr);

    if (m_OidSet.end() != it) {
      (*it)->AppendToIovec(ResponseVec);
      ptr = *it; // un const
      send_now=send_now && ptr->IsDataReady(Notify);
    }            // if
    else {
      // push VarBindHeader with eEndOfMibView
      gSnmpValErrorEOM.AppendToIovec(StartId, ResponseVec);
    } // else

  } // else

  return (send_now);

} // SnmpAgent::GetVariables
