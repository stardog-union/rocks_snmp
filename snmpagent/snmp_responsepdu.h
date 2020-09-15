/**
 * @file snmp_responsepdu.h
 * @author matthewv
 * @date July 10, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX response pdu object (rfc 2741, January
 * 2000)
 */

#ifndef SNMP_RESPONSEPDU_H
#define SNMP_RESPONSEPDU_H

#include "snmp_pdu.h"

typedef std::shared_ptr<class ResponsePDU> ResponsePDUPtr;

/**
 * Buffer for sending agentx-Response-PDU packet
 * @date created 10/02/11
 */
class ResponsePDU : public ReaderWriterBuf {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:
  std::vector<iovec>
      m_ResponsePDUVec; //!< m_Header [0], PduResponse[1], variable data
  std::vector<iovec>
      m_ResponsePDUVecCopy; //!< working copy used during actual xmit
  size_t m_ResponsePDUSent; //!< bytes received so far
  size_t m_WriteEnd;

  PduHeader m_Header;     //!< first 20 bytes of packet, contains length
  PduResponse m_Response; //!< next 8 bytes, uptime & response code

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  //
  // standard interface for ReaderWriterBuf
  //

  ResponsePDU(class SnmpAgent &Agent, PduInboundBufPtr &Request);

  virtual ~ResponsePDU();

  //
  virtual const struct iovec *WriteIovec();

  virtual int WriteIovecCnt() { return (m_ResponsePDUVec.size()); };

  virtual size_t WriteLen() { return (m_ResponsePDUSent); };

  virtual void WriteMarkLen(size_t Written) { m_ResponsePDUSent += Written; };

  virtual size_t WriteEnd() { return (m_WriteEnd); };

  //
  // custom routines
  //

  /// calculate the total length:  set PayloadLength and WriteEnd
  void SetWriteEnd();

  // debug
  void Dump();

protected:
private:
  ResponsePDU();                    //!< disabled:  default constructor
  ResponsePDU(const ResponsePDU &); //!< disabled:  copy operator
  ResponsePDU &
  operator=(const ResponsePDU &); //!< disabled:  assignment operator
};                                // class ResponsePDU

#endif // ifndef SNMP_RESPONSEPDU_H
