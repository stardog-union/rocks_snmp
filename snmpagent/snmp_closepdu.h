/**
 * @file snmp_closepdu.h
 * @author matthewv
 * @date July 10, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX close pdu object (rfc 2741, January 2000)
 */

#ifndef SNMP_CLOSEPDU_H
#define SNMP_CLOSEPDU_H

#include "snmp_pdu.h"

typedef std::shared_ptr<class ClosePDU> ClosePDUPtr;

/**
 * Buffer for sending agentx-Close-PDU packet
 * @date created 07/10/11
 */
class ClosePDU : public ReaderWriterBuf {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  enum {
    eCloseReasonOther = 1,
    eCloseReasonError = 2,
    eCloseReasonProtocolError = 3,
    eCloseReasonTimeouts = 4,
    eCloseReasonShutdown = 5,
    eCloseReasonByManager = 6,
  };

protected:
  struct iovec m_ClosePDUVec[6]; //!< m_Header [0], reason [1]
  size_t m_ClosePDUSent;         //!< bytes received so far

  PduHeader m_Header; //!< first 20 bytes of packet, contains length
  unsigned m_Reason;

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  //
  // standard interface for ReaderWriterBuf
  //

  ClosePDU(class SnmpAgent &Agent, unsigned Reason);

  virtual ~ClosePDU();

  //
  virtual const struct iovec *WriteIovec();

  virtual int WriteIovecCnt() { return (2); };

  virtual size_t WriteLen() { return (m_ClosePDUSent); };

  virtual void WriteMarkLen(size_t Written) { m_ClosePDUSent += Written; };

  virtual size_t WriteEnd() { return (sizeof(m_Header) + sizeof(unsigned)); };

  //
  // custom routines
  //

  void Dump();

protected:
private:
  ClosePDU();                            //!< disabled:  default constructor
  ClosePDU(const ClosePDU &);            //!< disabled:  copy operator
  ClosePDU &operator=(const ClosePDU &); //!< disabled:  assignment operator
};                                       // class ClosePDU

#endif // ifndef SNMP_CLOSEPDU_H
