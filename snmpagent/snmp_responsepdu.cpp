/**
 * @file snmp_responsepdu.cpp
 * @author matthewv
 * @date October 2, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implemenation of snmp agentX response pdu object (rfc 2741, January
 * 2000)
 */

#include <memory.h>
#include <stdio.h>

#ifndef SNMP_AGENT_H
#include "snmp_agent.h"
#endif

#ifndef SNMP_RESPONSEPDU_H
#include "snmp_responsepdu.h"
#endif

/**
 * Initialize the data members.
 * @date Created 10/02/11
 * @author matthewv
 */
ResponsePDU::ResponsePDU(PduInboundBufPtr &Request)
    : m_ResponsePDUSent(0), m_WriteEnd(0) {
  struct iovec builder;

  m_Header = Request->GetHeader();
  m_Header.m_Type = eResponsePDU;
  m_Header.m_PayloadLength = 0;

  m_Response.m_SysUpTime = 0;
  m_Response.m_Error = eNoAgentXError;
  m_Response.m_Index = 0;

  // for now, block any non-default contexts
  if (0x8 & m_Header.m_Flags) {
    m_Response.m_Error = eUnsupportedContext;
  } // if

  // create standard portion of transmit buffers
  builder.iov_base = &m_Header;
  builder.iov_len = sizeof(m_Header);
  m_ResponsePDUVec.push_back(builder);

  builder.iov_base = &m_Response;
  builder.iov_len = sizeof(m_Response);
  m_ResponsePDUVec.push_back(builder);

  return;

} // ResponsePDU::ResponsePDU

/**
 * Release resources
 * @date Created 10/02/11
 * @author matthewv
 */
ResponsePDU::~ResponsePDU() { return; } // ResponsePDU::~ResponsePDU

/**
 * Construct vector of components that make up packet
 * @date Created 10/02/11
 * @author matthewv
 */
const struct iovec *ResponsePDU::WriteIovec() {
  m_ResponsePDUVecCopy = m_ResponsePDUVec;
  AdjustIovec(&m_ResponsePDUVecCopy.at(0), m_ResponsePDUVecCopy.size(),
              m_ResponsePDUSent);

  return (&m_ResponsePDUVecCopy.at(0));

} // ResponsePDU::WriteIovec

/**
 * Adjust size variables based upon current vector
 * @date Created 10/02/11
 * @author matthewv
 */
void ResponsePDU::SetWriteEnd() {
  std::vector<struct iovec>::iterator it;

  m_WriteEnd = 0;
  for (it = m_ResponsePDUVec.begin(); m_ResponsePDUVec.end() != it; ++it)
    m_WriteEnd += it->iov_len;

  m_Header.m_PayloadLength = m_WriteEnd - sizeof(m_Header);

  return;

} // ResponsePDU::SetWriteEnd

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void ResponsePDU::Dump() {
  printf("ResponsePDU\n");
  printf("  m_ResponsePDUSent: %zd\n", m_ResponsePDUSent);
#if 0
    printf("    m_ResponsePDUVec[0].iov_base: %p\n", m_ResponsePDUVec[0].iov_base);
    printf("    m_ResponsePDUVec[0].iov_len: %zd\n", m_ResponsePDUVec[0].iov_len);
    printf("    m_ResponsePDUVec[1].iov_base: %p\n", m_ResponsePDUVec[1].iov_base);
    printf("    m_ResponsePDUVec[1].iov_len: %zd\n", m_ResponsePDUVec[1].iov_len);
    printf("    m_ResponsePDUVec[2].iov_base: %p\n", m_ResponsePDUVec[2].iov_base);
    printf("    m_ResponsePDUVec[2].iov_len: %zd\n", m_ResponsePDUVec[2].iov_len);
#endif

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
} // ResponsePDU::Dump
