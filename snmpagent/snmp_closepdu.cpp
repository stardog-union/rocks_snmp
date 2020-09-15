/**
 * @file snmp_closepdu.h
 * @author matthewv
 * @date July 10, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implemenation of snmp agentX open close object (rfc 2741, January
 * 2000)
 */

#include <memory.h>

#ifndef SNMP_AGENT_H
#include "snmp_agent.h"
#endif

#ifndef SNMP_CLOSEPDU_H
#include "snmp_closepdu.h"
#endif

/**
 * Initialize the data members.
 * @date Created 07/10/11
 * @author matthewv
 */
ClosePDU::ClosePDU(SnmpAgent &Agent, unsigned Reason)
    : m_ClosePDUSent(0), m_Reason(Reason) {
  // clear the structures
  memset(&m_Header, 0, sizeof(m_Header));

  // initialize structures
  m_Header.m_Version = 1;
  m_Header.m_Type = eClosePDU;
  m_Header.m_Flags = 0;

  m_Header.m_SessionID = Agent.GetSessionId();
  m_Header.m_TransactionID = 1;
  m_Header.m_PacketID.u = Agent.GetNextPacketId();
  m_Header.m_PacketID.c[3] = eClosePDU;
  m_Header.m_PayloadLength = WriteEnd() - sizeof(m_Header);

  return;

} // ClosePDU::ClosePDU

/**
 * Release resources
 * @date Created 07/09/11
 * @author matthewv
 */
ClosePDU::~ClosePDU() { return; } // ClosePDU::~ClosePDU

/**
 * Construct vector of components that make up packet
 * @date Created 07/10/11
 * @author matthewv
 */
const struct iovec *ClosePDU::WriteIovec() {
  m_ClosePDUVec[0].iov_base = &m_Header;
  m_ClosePDUVec[0].iov_len = sizeof(m_Header);

  m_ClosePDUVec[1].iov_base = &m_Reason;
  m_ClosePDUVec[1].iov_len = sizeof(m_Reason);

  AdjustIovec(m_ClosePDUVec, 2, m_ClosePDUSent);

  return (m_ClosePDUVec);

} // ClosePDU::ReadIovec

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void ClosePDU::Dump() {
  printf("ClosePDU\n");
  printf("  m_ClosePDUSent: %zd\n", m_ClosePDUSent);
  printf("    m_ClosePDUVec[0].iov_base: %p\n", m_ClosePDUVec[0].iov_base);
  printf("    m_ClosePDUVec[0].iov_len: %zd\n", m_ClosePDUVec[0].iov_len);
  printf("    m_ClosePDUVec[1].iov_base: %p\n", m_ClosePDUVec[1].iov_base);
  printf("    m_ClosePDUVec[1].iov_len: %zd\n", m_ClosePDUVec[1].iov_len);

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
} // ClosePDU::Dump
