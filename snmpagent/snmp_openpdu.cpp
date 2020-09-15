/**
 * @file snmp_openpdu.h
 * @author matthewv
 * @date July 9, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implemenation of snmp agentX open pdu object (rfc 2741, January 2000)
 */

#include <memory.h>

#ifndef SNMP_AGENT_H
#include "snmp_agent.h"
#endif

#ifndef SNMP_OPENPDU_H
#include "snmp_openpdu.h"
#endif

/**
 * Initialize the data members.
 * @date Created 07/09/11
 * @author matthewv
 */
OpenPDU::OpenPDU(SnmpAgent &Agent)
    : m_OpenPduSent(0), m_OidLen(0), m_DescLen(0), m_PadLen(0) {
  if (NULL != Agent.GetOidPrefixPtr()) {
    // save parameters
    m_Oid = Agent.GetOidPrefixPtr();
    m_OidLen = Agent.GetOidPrefixLen();
    m_Description = Agent.GetAgentName();
    m_DescLen = Agent.GetAgentNameLen();

    // calculate before using "Write()" in payload length
    m_PadLen = (0 == (m_DescLen & 0x3) ? 0 : 4 - (m_DescLen & 0x3));

    // clear the structures
    memset(&m_Header, 0, sizeof(m_Header));
    memset(&m_TimeoutOid, 0, sizeof(m_TimeoutOid));

    // initialize structures
    m_Header.m_Version = 1;
    m_Header.m_Type = eOpenPDU;
    m_Header.m_Flags = 0;

    m_Header.m_SessionID = 0;
    m_Header.m_TransactionID = 0;
    m_Header.m_PacketID.u = Agent.GetNextPacketId();
    m_Header.m_PacketID.c[3] = eOpenPDU;
    m_Header.m_PayloadLength = WriteEnd() - sizeof(m_Header);

    m_TimeoutOid.m_Timeout = 0;
    m_TimeoutOid.m_SubIdLen = m_OidLen;
    m_TimeoutOid.m_Prefix = 4;
    m_TimeoutOid.m_Index = 0;

  } // if
  else {
    Logging(LOG_ERR, "%s: Invalid parameters (%p, %p)", __func__);
  } // else

  return;

} // OpenPDU::OpenPDU

/**
 * Release resources
 * @date Created 07/09/11
 * @author matthewv
 */
OpenPDU::~OpenPDU() { return; } // OpenPDU::~OpenPDU

/**
 * Construct vector of components that make up packet
 * @date Created 07/07/11
 * @author matthewv
 */
const struct iovec *OpenPDU::WriteIovec() {
  m_OpenPduVec[0].iov_base = &m_Header;
  m_OpenPduVec[0].iov_len = sizeof(m_Header);

  m_OpenPduVec[1].iov_base = &m_TimeoutOid;
  m_OpenPduVec[1].iov_len = sizeof(m_TimeoutOid);

  m_OpenPduVec[2].iov_base = (void *)m_Oid;
  m_OpenPduVec[2].iov_len = sizeof(unsigned) * m_OidLen;

  m_OpenPduVec[3].iov_base = &m_DescLen;
  m_OpenPduVec[3].iov_len = sizeof(unsigned);

  m_OpenPduVec[4].iov_base = (void *)m_Description;
  m_OpenPduVec[4].iov_len = m_DescLen;

  m_OpenPduVec[5].iov_base = &gSnmpAgentPadString;
  m_OpenPduVec[5].iov_len = m_PadLen;

  AdjustIovec(m_OpenPduVec, 6, m_OpenPduSent);

  return (m_OpenPduVec);

} // OpenPDU::ReadIovec

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void OpenPDU::Dump() {
  printf("OpenPDU\n");
  printf("  m_OpenPduSent: %zd\n", m_OpenPduSent);
  printf("    m_OpenPduVec[0].iov_base: %p\n", m_OpenPduVec[0].iov_base);
  printf("    m_OpenPduVec[0].iov_len: %zd\n", m_OpenPduVec[0].iov_len);
  printf("    m_OpenPduVec[1].iov_base: %p\n", m_OpenPduVec[1].iov_base);
  printf("    m_OpenPduVec[1].iov_len: %zd\n", m_OpenPduVec[1].iov_len);
  printf("    m_OpenPduVec[2].iov_base: %p\n", m_OpenPduVec[2].iov_base);
  printf("    m_OpenPduVec[2].iov_len: %zd\n", m_OpenPduVec[2].iov_len);
  printf("    m_OpenPduVec[3].iov_base: %p\n", m_OpenPduVec[3].iov_base);
  printf("    m_OpenPduVec[3].iov_len: %zd\n", m_OpenPduVec[3].iov_len);
  printf("    m_OpenPduVec[4].iov_base: %p\n", m_OpenPduVec[4].iov_base);
  printf("    m_OpenPduVec[4].iov_len: %zd\n", m_OpenPduVec[4].iov_len);
  printf("    m_OpenPduVec[5].iov_base: %p\n", m_OpenPduVec[5].iov_base);
  printf("    m_OpenPduVec[5].iov_len: %zd\n", m_OpenPduVec[5].iov_len);

  printf("          m_Version: %u\n", (unsigned)m_Header.m_Version);
  printf("             m_Type: %u\n", (unsigned)m_Header.m_Type);
  printf("            m_Flags: 0x%x\n", (unsigned)m_Header.m_Flags);

  printf("        m_SessionID: %u\n", m_Header.m_SessionID);
  printf("    m_TransactionID: %u\n", m_Header.m_TransactionID);
  printf("         m_PacketID: %u (%u)\n", m_Header.m_PacketID.u,
         (unsigned)m_Header.m_PacketID.c[3]);
  printf("    m_PayloadLength: %u\n", m_Header.m_PayloadLength);

  printf("         m_OidLen(): %u\n", m_OidLen);
  printf("        m_DescLen(): %u\n", m_DescLen);
  printf("         m_PadLen(): %u\n", m_PadLen);
  printf("         WriteEnd(): %zd\n", WriteEnd());

  return;
} // OpenPDU::Dump
