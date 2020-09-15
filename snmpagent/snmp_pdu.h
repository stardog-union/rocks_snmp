/**
 * @file snmp_pdu.h
 * @author matthewv
 * @date July 4, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for snmp agentX pdu objects (rfc 2741, January 2000)
 */
#include <memory>

#ifndef SNMP_PDU_H
#define SNMP_PDU_H

#include "reader_writer.h"

/**
 * AgentX PDU type codes
 * @date 07/09/11
 */
enum PduTypeCodes
{
    eOpenPDU=1,
    eClosePDU=2,
    eRegisterPDU=3,
    eUnregisterPDU=4,
    eGetPDU=5,
    eGetNextPDU=6,
    eGetBulkPDU=7,
    eTestSetPDU=8,
    eCommitSetPDU=9,
    eUndoSetPDU=10,
    eCleanupSetPDU=11,
    eNotifyPDU=12,
    ePingPDU=13,

    eResponsePDU=18
};


/**
 * AgentX Response PDU error codes
 * @date 10/02/11
 */
enum PduErrorCodes
{
    // rfc 1905
    eNoError=0,
    eTooBig=1,
    eNoSuchName=2,
    eBadValue=3,
    eReadOnly=4,
    eGenErr=5,
    eNoAccess=6,
    eWrongType=7,
    eWrongLength=8,
    eWrongEncoding=9,
    eWrongValue=10,  // there are more

    // AgentX defined errors
    eNoAgentXError=0,
    eOpenFailed=256,
    eNotOpen=257,
    eIndexWrongType=258,
    eIndexAlreadyAllocated=259,
    eIndexNoneAvailable=260,
    eIndexNotAllocated=261,
    eUnsupportedContext=262,
    eDuplicateRegistration=263,
    eUnknownRegistration=264,
    eUnknownAgentCaps=265,
    eParseError=266,
    eRequestDenied=267,
    eProcessingError=268,

};



/**
 * AgentX PDU Header
 * @date January 2000, rfc 2741
 */
struct PduHeader
{
    unsigned char m_Version;    //!< version of AgentX protocol (1 for jan 2000)
    unsigned char m_Type;       //!< pdu type: PduTypeCodes above
    unsigned char m_Flags;      //!< bitmask, bit 0 as least significant
    unsigned char m_Reserved;   //!< unused, keeps 32 bit alignment
    unsigned m_SessionID;       //!< master assigned to agent communication channel
    unsigned m_TransactionID;   //!< master assigned to multi-packet transactions
    union
    {
        unsigned u;             //!< normal packet id is simple unsigned
        char c[4];              //!< SnmpAgent's admin requests hide data in c[3]
    } m_PacketID;               //!< unique id to this packet
    unsigned m_PayloadLength;   //!< length of packet, less header, multiple of 4
} __attribute__ ((packed));

/**
 * AgentX oid descriptor with timeout prefix
 * @date 07/09/11
 */
struct PduTimeoutOid
{
    unsigned char m_Timeout;     //!< time in seconds
    unsigned char m_Priority;    //!< default is 127
    unsigned char m_RangeSubId;  //!<
    unsigned char m_Reserved1;   //!< padded space
    unsigned char m_SubIdLen;    //!<
    unsigned char m_Prefix;      //!<
    unsigned char m_Index;       //!<
    unsigned char m_Reserved2;   //!< padded space
} __attribute__ ((packed));


/**
 * AgentX oid prefix
 * @date 10/02/11
 */
struct PduSubId
{
    unsigned char m_SubIdLen;    //!<
    unsigned char m_Prefix;      //!<
    unsigned char m_Index;       //!<
    unsigned char m_Reserved2;   //!< padded space
} __attribute__ ((packed));


/**
 * AgentX response structure
 * @date 07/10/11
 */
struct PduResponse
{
    unsigned m_SysUpTime;        //!< uptime in messages from master to agent, otherwise 0
    unsigned short m_Error;      //!< error code
    unsigned short m_Index;      //!< 1 based index to variable that failed
} __attribute__ ((packed));

/**
 * Padding suffix used on strings to achieve dword alignment
 * @date 07/09/11
 */
struct PduString
{
    unsigned char m_Padding[4];
};

extern PduString gSnmpAgentPadString;

typedef std::shared_ptr<class PduInboundBuf> PduInboundBufPtr;

/**
 * Buffer for receiving PDUs sent from master to agent (any type)
 * @date created 07/04/11
 */
class PduInboundBuf : public ReaderWriterBuf
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:
    enum
    {
        eResponseSizeIncrement=128,     //!< size to grow response buffer
    };

protected:
    struct iovec m_InboundVec[2];       //!< m_Header [0], additional [1]
    size_t m_InboundReceived;           //!< bytes received so far
    char * m_InboundBuf;                //!< optional buffer for data after header
    size_t m_InboundBufLen;

    PduHeader m_Header;                 //!< first 20 bytes of packet, contains length
private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:
    //
    // standard interface for ReaderWriterBuf
    //

    PduInboundBuf();

    virtual ~PduInboundBuf();


    //
    virtual const struct iovec * ReadIovec();

    virtual int ReadIovecCnt() {return(2);};

    virtual size_t ReadLen() {return(m_InboundReceived);};

    virtual void ReadMarkLen(size_t Read) {m_InboundReceived+=Read;};

    virtual size_t ReadMinimum();


    //
    // custom routines
    //

    /// accessor
    const char * GetInboundBuf() const {return(m_InboundBuf);};

    /// pdu type accessor
    unsigned GetPduType() const {return(m_Header.m_Type);};

    /// extra our original request type from the packet id of response
    unsigned GetResponseType() const {return(m_Header.m_PacketID.c[3]);};

    /// clear previous info, setup for new packet
    void Reset();

    PduHeader & GetHeader() {return(m_Header);};

    /// debug
    virtual void Dump();

protected:

private:
    PduInboundBuf(const PduInboundBuf & );             //!< disabled:  copy operator
    PduInboundBuf & operator=(const PduInboundBuf &);  //!< disabled:  assignment operator
};  // class PduInboundBuf



#endif // ifndef SNMP_PDU_H
