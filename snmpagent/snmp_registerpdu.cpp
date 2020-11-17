/**
 * @file snmp_registerpdu.cpp
 * @author matthewv
 * @date July 9, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implemenation of snmp agentX open pdu object (rfc 2741, January 2000)
 */

#include <memory.h>
#include <stdio.h>

#include "snmp_agent.h"
#include "snmp_registerpdu.h"
#include "logging.h"

/**
 * Initialize the data members.
 * @date Created 07/10/11
 * @author matthewv
 */
RegisterPDU::RegisterPDU(SnmpAgent &Agent) : m_RegisterPDUSent(0), m_OidLen(0) {
  if (NULL != Agent.GetOidPrefixPtr()) {
    // save parameters
    m_Oid = Agent.GetOidPrefixPtr();
    m_OidLen = Agent.GetOidPrefixLen();

    // clear the structures
    memset(&m_Header, 0, sizeof(m_Header));
    memset(&m_TimeoutOid, 0, sizeof(m_TimeoutOid));

    // initialize structures
    m_Header.m_Version = 1;
    m_Header.m_Type = eRegisterPDU;
    m_Header.m_Flags = 0;

    m_Header.m_SessionID = Agent.GetSessionId();
    m_Header.m_TransactionID = 0;
    m_Header.m_PacketID.u = Agent.GetNextPacketId();
    m_Header.m_PacketID.c[3] = eRegisterPDU;
    m_Header.m_PayloadLength = WriteEnd() - sizeof(m_Header);

    m_TimeoutOid.m_Timeout = 0;
    m_TimeoutOid.m_Priority = 127;
    m_TimeoutOid.m_RangeSubId = 0;

    m_TimeoutOid.m_SubIdLen = m_OidLen;
    m_TimeoutOid.m_Prefix = 4;
    m_TimeoutOid.m_Index = 0;
  } // if
  else {
    Logging(LOG_ERR, "%s: Invalid parameters", __func__);
  } // else

  return;

} // RegisterPDU::RegisterPDU

/**
 * Release resources
 * @date Created 07/10/11
 * @author matthewv
 */
RegisterPDU::~RegisterPDU() { return; } // RegisterPDU::~RegisterPDU

/**
 * Construct vector of components that make up packet
 * @date Created 07/10/11
 * @author matthewv
 */
const struct iovec *RegisterPDU::WriteIovec() {
  m_RegisterPDUVec[0].iov_base = &m_Header;
  m_RegisterPDUVec[0].iov_len = sizeof(m_Header);

  m_RegisterPDUVec[1].iov_base = &m_TimeoutOid;
  m_RegisterPDUVec[1].iov_len = sizeof(m_TimeoutOid);

  m_RegisterPDUVec[2].iov_base = (void *)m_Oid;
  m_RegisterPDUVec[2].iov_len = sizeof(unsigned) * m_OidLen;

  AdjustIovec(m_RegisterPDUVec, 3, m_RegisterPDUSent);

  return (m_RegisterPDUVec);

} // RegisterPDU::ReadIovec

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void RegisterPDU::Dump() {
  printf("RegisterPDU\n");
  printf("  m_RegisterPDUSent: %zd\n", m_RegisterPDUSent);
  printf("    m_RegisterPDUVec[0].iov_base: %p\n",
         m_RegisterPDUVec[0].iov_base);
  printf("    m_RegisterPDUVec[0].iov_len: %zd\n", m_RegisterPDUVec[0].iov_len);
  printf("    m_RegisterPDUVec[1].iov_base: %p\n",
         m_RegisterPDUVec[1].iov_base);
  printf("    m_RegisterPDUVec[1].iov_len: %zd\n", m_RegisterPDUVec[1].iov_len);
  printf("    m_RegisterPDUVec[2].iov_base: %p\n",
         m_RegisterPDUVec[2].iov_base);
  printf("    m_RegisterPDUVec[2].iov_len: %zd\n", m_RegisterPDUVec[2].iov_len);

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
} // RegisterPDU::Dump
