/**
 * @file request_response.h
 * @author matthewv
 * @date June 8, 2011
 * @date Copyright 2011
 *
 * @brief Declarations for a socket based request/response protocol machine
 */

#ifndef REQUEST_RESPONSE_H
#define REQUEST_RESPONSE_H

#include <queue>

#include "tcp_event.h"

#include "request_response_buf.h"

typedef std::shared_ptr<class RequestResponse> RequestResponsePtr;


/**
 * Write Request, read Response
 *
 * Use events from TcpEventSocket
 */

class RequestResponse : public TcpEventSocket
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:

    /// nodes and edges from TcpEventSocket

protected:
    RequestResponseQueue_t m_ReqQueue;    //<! ordered list of requests to send
    RequestResponseBufPtr m_CurRequest;   //<! request being processed currently

    RequestResponseQueue_t m_ReqNotify;   //<! ordered list of requests that completed
    bool m_ReqNotifyLock;                 //<! (engine single threaded) flag to "lock" notifications
private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:

    RequestResponse(unsigned IpHostOrder, unsigned PortHostOrder);

    virtual ~RequestResponse();

    /// debug
    void Dump();

    /// get data from handle
    void AddRequest(RequestResponseBufPtr & Buffer);
    void AddRequest(RequestResponseBuf & Buffer)
    {RequestResponseBufPtr ptr(&Buffer); AddRequest(ptr);};

    //
    // statemachine callbacks
    //
    //    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr & Caller, bool PreNotify);

    //
    // meventobj callbacks
    //

    /// allows initialization by independent event thread where appropriate
    virtual void ThreadInit(MEventMgrPtr & Mgr);

    /// External callback used when time value expires
    virtual void TimerCallback();

    /// External callback when handle contains error flag
    virtual bool ErrorCallback();


protected:
    /// start another request if one not already pending
    void ProcessNextRequest();

    /// request sent, get the response ... please
    void ProcessCurrentResponse();

    /// send notification in order, knowing the recursive nature of mevent
    void ProcessRequestNotifications();

private:
    RequestResponse();                                     //!< disabled:  use 2 integer constructor
    RequestResponse(const RequestResponse & );             //!< disabled:  copy operator
    RequestResponse & operator=(const RequestResponse &);  //!< disabled:  assignment operator

};  // RequestResponse



#endif // ifndef REQUEST_RESPONSE_H
