/**
 * @file snmp_pdu.h
 * @author matthewv
 * @date July 4, 2011
 * @date Copyright 2011-2012
 *
 * @brief Implementation of snmp agentX pdu objects (rfc 2741, January 2000)
 */

#include <memory.h>

#include "snmp_pdu.h"
#include "util/logging.h"

PduString gSnmpAgentPadString = {{0, 0, 0, 0}};

/**
 * Initialize the data members.
 * @date Created 07/04/11
 * @author matthewv
 */
PduInboundBuf::PduInboundBuf() : m_InboundBuf(NULL) {
  m_InboundBuf = new char[eResponseSizeIncrement];
  m_InboundBufLen = (NULL != m_InboundBuf ? eResponseSizeIncrement : 0);

  if (NULL == m_InboundBuf) {
    Logging(LOG_ERR, "%s: out of memory error", __func__);
  } // if

  Reset();

  return;

} // PduInboundBuf::PduInboundBuf

/**
 * Release resources
 * @date Created 07/09/11
 * @author matthewv
 */
PduInboundBuf::~PduInboundBuf() {
  delete[] m_InboundBuf;
  m_InboundBuf = NULL;

  return;

} // PduInboundBuf::~PduInboundBuf

/**
 * Put object into a fresh state (for initial use or reuse)
 * @date Created 07/09/11
 * @author matthewv
 */
void PduInboundBuf::Reset() {
  m_InboundVec[0].iov_base = NULL;
  m_InboundVec[0].iov_len = 0;
  m_InboundVec[1].iov_base = NULL;
  m_InboundVec[1].iov_len = 0;
  m_InboundReceived = 0;

  // leave buffer alone, not reset
  memset(&m_Header, 0, sizeof(m_Header));

} // PduInboundBuf::~PduInboundBuf

/**
 * Give original of the response information
 * @date Created 06/09/11
 * @author matthewv
 */
const struct iovec *PduInboundBuf::ReadIovec() {
  size_t size;

  m_InboundVec[0].iov_base = &m_Header;
  m_InboundVec[0].iov_len = sizeof(m_Header);

  // only size the second vector if we have pdu size
  if (m_InboundReceived < sizeof(m_Header))
    size = 0;
  else
    size = m_Header.m_PayloadLength;

  // resize buffer
  if (m_InboundBufLen < size) {
    char *ptr;
    size_t round_size;

    round_size = ((size / eResponseSizeIncrement) + 1) * eResponseSizeIncrement;
    ptr = new char[round_size];
    if (NULL != ptr) {
      // copy any partial buffer (unlikely though)
      if (NULL != m_InboundBuf && sizeof(m_Header) < m_InboundReceived)
        memcpy(ptr, m_InboundBuf, m_InboundReceived - sizeof(m_Header));
      delete[] m_InboundBuf;
      m_InboundBuf = ptr;
      m_InboundBufLen = round_size;
    } // if
    else {
      Logging(LOG_ERR, "%s: out of memory error", __func__);
    } // else
  }   // if

  m_InboundVec[1].iov_base = m_InboundBuf;
  m_InboundVec[1].iov_len = size;

  AdjustIovec(m_InboundVec, 2, m_InboundReceived);

  return (m_InboundVec);

} // PduInboundBuf::ReadIovec

/**
 * How many bytes known missing?
 * @date Created 07/09/11
 * @author matthewv
 */
size_t PduInboundBuf::ReadMinimum() {
  size_t ret_size;

  ret_size = sizeof(m_Header);

  if (sizeof(m_Header) <= m_InboundReceived)
    ret_size += m_Header.m_PayloadLength;

  return (ret_size);

} // PduInboundBuf::ReadMinimum

/**
 * Debug aid
 * @date Created 07/10/11
 * @author matthewv
 */
void PduInboundBuf::Dump() {
  printf("PduInboundBuf\n");
  printf("  m_InboundReceived: %zd\n", m_InboundReceived);
  printf("    m_InboundBufLen: %zd\n", m_InboundBufLen);
  printf("    m_InboundVec[0].iov_base: %p\n", m_InboundVec[0].iov_base);
  printf("    m_InboundVec[0].iov_len: %zd\n", m_InboundVec[0].iov_len);
  printf("    m_InboundVec[1].iov_base: %p\n", m_InboundVec[1].iov_base);
  printf("    m_InboundVec[1].iov_len: %zd\n", m_InboundVec[1].iov_len);

  printf("          m_Version: %u\n", (unsigned)m_Header.m_Version);
  printf("             m_Type: %u\n", (unsigned)m_Header.m_Type);
  printf("            m_Flags: 0x%x\n", (unsigned)m_Header.m_Flags);

  printf("        m_SessionID: %u\n", m_Header.m_SessionID);
  printf("    m_TransactionID: %u\n", m_Header.m_TransactionID);
  printf("         m_PacketID: %u (%u)\n", m_Header.m_PacketID.u,
         (unsigned)m_Header.m_PacketID.c[3]);
  printf("    m_PayloadLength: %u\n", m_Header.m_PayloadLength);

  if (eResponsePDU == m_Header.m_Type) {
    PduResponse *resp;
    resp = (PduResponse *)GetInboundBuf();
    printf("        m_SysUpTime: %u\n", resp->m_SysUpTime);
    printf("            m_Error: %u\n", (unsigned)resp->m_Error);
    printf("            m_Index: %u\n", (unsigned)resp->m_Index);
  } // if

  return;
} // PduInboundBuf::Dump
