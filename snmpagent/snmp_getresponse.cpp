/**
 * @file snmp_getresponse.cpp
 * @author matthewv
 * @date October 2, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implemenation of snmp agentX response pdu object for Get & GetNext
 * (rfc 2741, January 2000)
 */

#include <memory.h>
#include <stdio.h>

#ifndef SNMP_AGENT_H
#include "snmp_agent.h"
#endif

#ifndef SNMP_GETRESPONSEPDU_H
#include "snmp_getresponse.h"
#endif

/**
 * Initialize the data members.
 * @date Created 10/02/11
 * @author matthewv
 */
GetResponsePDU::GetResponsePDU(SnmpAgent &Agent, PduInboundBufPtr &Request)
    : ResponsePDU(Request), m_PendingData(0) {
  // only initialize / read data if lower levels happy
  if (eNoAgentXError == m_Response.m_Error) {
    bool send_now, flag = {false};
    int count;
    const char *ptr, *limit;
    const PduSubId *id_start, *id_end;
    unsigned short error;

    send_now = true;
    count = 0;
    ptr = Request->GetInboundBuf();
    limit = ptr + (Request->ReadLen() - sizeof(PduHeader));

    // walk identifiers and fill in responses
    while (ptr < limit) {
      ++count; // 1 based index

      // isolate the range set
      id_start = (const PduSubId *)ptr;
      id_end = (const PduSubId *)(ptr + (4 * id_start->m_SubIdLen +
                                         sizeof(PduSubId)));
      ptr = (const char *)id_end;
      ptr = ptr + (4 * id_end->m_SubIdLen + sizeof(PduSubId));

      // add variable to output
      error = 0;

      // hmmm ... self/weak pointer not set at this point in constructor
      StateMachinePtr shared; // pointer not yet set ... = GetStateMachinePtr();
      flag = Agent.GetVariables(m_ResponsePDUVec, *id_start, *id_end,
                                eGetNextPDU == Request->GetHeader().m_Type,
                                error, shared);

      // if data is asynchronous, await notification on all
      //   (GetVariables added completion call)
      if (!flag)
        ++m_PendingData;

      send_now = send_now && flag;
      m_Response.m_Error = error;
    } // while

    SetDataReady(send_now);
    if (!send_now) {
      StateMachinePtr shared = Agent.GetStateMachinePtr();
      AddCompletion(shared);
    }

#if 0
        if (!good)
        {
            m_Response.m_Index=count;
        }   // if
#endif
  } // if

  // establish overall response size info
  SetWriteEnd();

  return;

} // GetResponsePDU::GetResponsePDU

/**
 * Release resources
 * @date Created 10/02/11
 * @author matthewv
 */
GetResponsePDU::~GetResponsePDU() { return; } // GetResponsePDU::~GetResponsePDU

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void GetResponsePDU::Dump() {
  size_t loop;

  printf("GetResponsePDU\n");
  printf("  m_ResponsePDUSent: %zd\n", m_ResponsePDUSent);

  printf("    m_ResponsePDUVec size: %zd\n", m_ResponsePDUVec.size());
  for (loop = 0; loop < m_ResponsePDUVec.size(); ++loop) {
    printf("    m_ResponsePDUVec[%zd].iov_base: %p\n", loop,
           m_ResponsePDUVec[loop].iov_base);
    printf("    m_ResponsePDUVec[%zd].iov_len: %zd\n", loop,
           m_ResponsePDUVec[loop].iov_len);
  } // for

  printf("          m_Version: %u\n", (unsigned)m_Header.m_Version);
  printf("             m_Type: %u\n", (unsigned)m_Header.m_Type);
  printf("            m_Flags: 0x%x\n", (unsigned)m_Header.m_Flags);

  printf("        m_SessionID: %u\n", m_Header.m_SessionID);
  printf("    m_TransactionID: %u\n", m_Header.m_TransactionID);
  printf("         m_PacketID: %u (%u)\n", m_Header.m_PacketID.u,
         (unsigned)m_Header.m_PacketID.c[3]);
  printf("    m_PayloadLength: %u\n", m_Header.m_PayloadLength);
  printf("         WriteEnd(): %zd\n", WriteEnd());

  return;
} // GetResponsePDU::Dump
