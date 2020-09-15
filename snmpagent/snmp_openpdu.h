/**
 * @file snmp_openpdu.h
 * @author matthewv
 * @date July 9, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX open pdu object (rfc 2741, January 2000)
 */

#ifndef SNMP_OPENPDU_H
#define SNMP_OPENPDU_H

#include "snmp_pdu.h"

typedef std::shared_ptr<class OpenPDU> OpenPDUPtr;

/**
 * Buffer for sending agentx-Open-PDU packet
 * @date created 07/09/11
 */
class OpenPDU : public ReaderWriterBuf
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:

protected:
    struct iovec m_OpenPduVec[6];       //!< m_Header [0], timeout [1], oid [2], strlen [3],
                                        //!<    string [4], pad[5]
    size_t m_OpenPduSent;               //!< bytes received so far

    PduHeader m_Header;                 //!< first 20 bytes of packet, contains length
    PduTimeoutOid m_TimeoutOid;
    const unsigned * m_Oid;
    unsigned m_OidLen;
    const char * m_Description;
    unsigned m_DescLen;                 //!< string length
    unsigned m_PadLen;                  //!< number of pad characters needed to align Desc

private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:
    //
    // standard interface for ReaderWriterBuf
    //

    OpenPDU(class SnmpAgent & Agent);

    virtual ~OpenPDU();


    //
    virtual const struct iovec * WriteIovec();

    virtual int WriteIovecCnt() {return(6);};

    virtual size_t WriteLen() {return(m_OpenPduSent);};

    virtual void WriteMarkLen(size_t Written) {m_OpenPduSent+=Written;};

    virtual size_t WriteEnd()
    {return(sizeof(m_Header)+sizeof(m_TimeoutOid)+m_OidLen*4+4+m_DescLen+m_PadLen);};


    //
    // custom routines
    //

    void Dump();

protected:

private:
    OpenPDU();                             //!< disabled:  default constructor
    OpenPDU(const OpenPDU & );             //!< disabled:  copy operator
    OpenPDU & operator=(const OpenPDU &);  //!< disabled:  assignment operator
};  // class OpenPDU



#endif // ifndef SNMP_OPENPDU_H
