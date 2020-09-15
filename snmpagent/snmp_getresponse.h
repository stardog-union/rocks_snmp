/**
 * @file snmp_responsepdu.h
 * @author matthewv
 * @date July 10, 2011
 * @date Copyright 2011
 *
 * @brief Declarations for snmp agentX response pdu object (rfc 2741, January
 * 2000)
 */

#ifndef SNMP_GETRESPONSEPDU_H
#define SNMP_GETRESPONSEPDU_H

#include "snmp_pdu.h"

#include "snmp_responsepdu.h"

typedef std::shared_ptr<class GetResponsePDU> GetResponsePDUPtr;

/**
 * Buffer for sending agentx-GetResponse-PDU packet
 * @date created 10/02/11
 */
class GetResponsePDU : public ResponsePDU {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:
  int m_PendingData; //!< count of pending data items

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  //
  // standard interface for ReaderWriterBuf
  //

  GetResponsePDU(class SnmpAgent &Agent, PduInboundBufPtr &Request);

  virtual ~GetResponsePDU();

  /// Public routine to receive Edge notification
#if 0
    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr & Caller, bool PreNotify)
    {
        bool ret_flag;

        ret_flag=false;
        if (ReaderWriter::RW_EDGE_DATAREADY==EdgeId)
        {
            ret_flag=true;

            if (0 < m_PendingData)
            {
                --m_PendingData;

                if (0==m_PendingData)
                    ret_flag=ResponsePDU::EdgeNotification(EdgeId, Caller, PreNotify);
            }   // if
        }   // if

        return(ret_flag);
    };
#endif
  //
  // custom routines
  //

  // debug
  void Dump();

protected:
private:
  GetResponsePDU();                       //!< disabled:  default constructor
  GetResponsePDU(const GetResponsePDU &); //!< disabled:  copy operator
  GetResponsePDU &
  operator=(const GetResponsePDU &); //!< disabled:  assignment operator
};                                   // class GetResponsePDU

#endif // ifndef SNMP_RESPONSEPDU_H
