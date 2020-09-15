/**
 * @file request_response_buf.h
 * @author matthewv
 * @date June 9, 2011
 * @date Copyright 2011-2012
 *
 * @brief Declarations for a reusable buffer for RequestResponse
 */

#ifndef REQUEST_RESPONSE_BUF_H
#define REQUEST_RESPONSE_BUF_H

#include <queue>
#include <string>

#include "reader_writer.h"

typedef std::shared_ptr<class RequestResponseBuf> RequestResponseBufPtr;
typedef std::queue<RequestResponseBufPtr> RequestResponseQueue_t;


class RequestResponseBuf : public ReaderWriterBuf
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:
    enum
    {
        eResponseSizeIncrement=4096,     //!< size to grow response buffer
    };

protected:
    struct iovec m_RequestVec;           //!< single string requests
    size_t m_RequestSize;                //!< total size of the request
    size_t m_RequestSent;                //!< bytes sent so far
    std::string m_RequestString;         //!< copy of string, if not static

    struct iovec m_ResponseVec;          //!< single buffer response
    size_t m_ResponseIn;                 //!< bytes received so far
    char * m_ResponseBuf;
    size_t m_ResponseBufLen;

    std::string m_ResponseEnding;        //!< string that marks end of response
private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:
    //
    // standard interface for ReaderWriterBuf
    //

    RequestResponseBuf();

    virtual ~RequestResponseBuf();


    virtual const struct iovec * WriteIovec() {return(&m_RequestVec);};

    virtual int WriteIovecCnt() {return(1);};

    virtual size_t WriteLen() {return(m_RequestSent);};

    virtual void WriteMarkLen(size_t Written) {m_RequestSent+=Written;};

    virtual size_t WriteEnd() {return(m_RequestSize);};


    virtual const struct iovec * ReadIovec();

    virtual int ReadIovecCnt() {return(1);};

    virtual size_t ReadLen() {return(m_ResponseIn);};

    virtual void ReadMarkLen(size_t Read) {m_ResponseIn+=Read;};

    virtual size_t ReadMinimum();


    //
    // custom routines
    //

    /// accessor
    const char * GetResponseBuf() const {return(m_ResponseBuf);};

    /// clear, setup for next
    virtual void Reset();

    /// request is a static string
    void StaticRequestString(const char * RequestString, int RequestLength=-1);

    /// request is something we need to copy locally
    void DynamicRequestString(const char * RequestString);

    /// set the string that marks the end of the packet
    void SetResponseEndsString(const char * EndString)
    {if (NULL!=EndString) m_ResponseEnding=EndString;};

    /// debug
    virtual void Dump() {printf("len %zd, \'%*.*s\'\n",
                                m_ResponseIn, (int)m_ResponseIn, (int)m_ResponseIn, m_ResponseBuf);};

protected:

private:
    RequestResponseBuf(const RequestResponseBuf & );             //!< disabled:  copy operator
    RequestResponseBuf & operator=(const RequestResponseBuf &);  //!< disabled:  assignment operator
};  // class RequestResponseBuf



#endif // ifndef REQUEST_RESPONSE_BUF_H
