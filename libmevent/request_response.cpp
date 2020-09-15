/**
 * @file request_response.cpp
 * @author matthewv
 * @date June 8, 2011
 * @date Copyright 2011
 *
 * @brief Implementation of a socket based request/response protocol machine
 */

#include <errno.h>
#include <arpa/inet.h>

#include "request_response.h"
#include "meventmgr.h"
#include "util/logging.h"


/**
 * Initialize the data members.
 * @date 06/08/11  matthewv  Created
 * @author matthewv
 */
RequestResponse::RequestResponse(
    unsigned IpHostOrder,                //!< zero or host order ip address
    unsigned PortHostOrder)              //!< zero or host order tcp port
    : m_ReqNotifyLock(false)
{
    m_NetIp=htonl(IpHostOrder);
    m_NetPort=htons(PortHostOrder);

    return;

}   // RequestResponse::RequestResponse


/**
 * Release resources
 * @date 06/08/11  matthewv  Created
 * @author matthewv
 */
RequestResponse::~RequestResponse()
{

}   // RequestResponse::~RequestResponse


/**
 * Add a new request to the fifo
 * @date Created 06/08/11
 * @author matthewv
 */
void
RequestResponse::AddRequest(
    RequestResponseBufPtr & Buffer) //!< buffer contain request (and to receive response)
{
    // validate inputs
    if (NULL!=Buffer.get())
    {
        // put it on queue, then try to start it
        m_ReqQueue.push(Buffer);
        ProcessNextRequest();
    }   // if
    else
    {
        Logging(LOG_ERR, "%s:  null Buffer passed.",
                __func__);
    }   // else

    return;

}   // RequestResponse::AddRequest


/**
 * First step in asynchronous connection.
 *  Initial implementation provides no benefit.  TcpEventSocket
 *  contains all needed code.
 * @date 06/08/11  matthewv  Created
 */
void
RequestResponse::ThreadInit(
    MEventMgrPtr & Mgr)
{
    // TcpEventSocket contains code to start the socket if
    //  the ip and port are already set
    TcpEventSocket::ThreadInit(Mgr);

    return;

}   // RequestResponse::ThreadInit


/**
 * A read or write request took too long, or inactive too long
 * @date Created 06/08/11
 * @author matthewv
 */
void
RequestResponse::TimerCallback()
{

    TcpEventSocket::TimerCallback();

}   // RequestResponse::TimerCallback


/**
 * event logic saw an error on the file descriptor
 * @date Created 06/08/11
 * @author matthewv
 * @returns true if simultaneous Read/Write callbacks should proceed
 */
bool
RequestResponse::ErrorCallback()
{
    bool ret_flag;

    ret_flag=TcpEventSocket::ErrorCallback();

    return(ret_flag);

}   // RequestResponse::ErrorCallback


/**
 * @brief Turn edge notifications into State changes
 *
 * @date Created 05/13/11
 * @author matthewv
 * @returns  true if edge handled to state transition
 */
#if 0
bool
RequestResponse::EdgeNotification(
    unsigned int EdgeId,               //!< what just happened, what graph edge are we walking
    StateMachinePtr & Caller,          //!< what state machine object initiated the edge
    bool PreNotify)                    //!< for watchers, is the before or after owner processes
{
    bool used;

    used=false;

    // only care about our own events
    if (this==Caller.get())
    {
        switch(EdgeId)
        {
            case TcpEventSocket::TS_EDGE_CONNECTED:
                used=TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
                ProcessNextRequest();
                break;

            // a write buffer fully sent
            case RW_EDGE_SENT:
                used=TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
                ProcessCurrentResponse();
                break;

            // a full response is available
            case RW_EDGE_RECEIVED:
                // used=TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);

                // save the request to later process its callbacks, once the next request
                //  has been initiated
                if (NULL!=m_CurRequest.get())
                {
                    m_ReqNotify.push(m_CurRequest);
                    m_CurRequest.reset();
                }   // if
                ProcessNextRequest();
                ProcessRequestNotifications();
                break;

            // clean up pending actions (this will need to change if
            //   retries are ever desired at this level)
            case RW_EDGE_CLOSED:
                // regular std::queue has no clear()
                while (m_ReqQueue.size()) m_ReqQueue.pop();
                m_CurRequest.reset();
                while (m_ReqNotify.size()) m_ReqNotify.pop();
                used=TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
                break;

            default:
                // send down a level.  If not used then it is an error
                used=TcpEventSocket::EdgeNotification(EdgeId, Caller, PreNotify);
                if (!used)
                {
                    Logging(LOG_ERR, "%s: unknown edge value passed [EdgeId=%u]",
                            __PRETTY_FUNCTION__, EdgeId);
                    SendEdge(RW_EDGE_ERROR);
                }   // if
                break;
        }   // switch
    }   // if

    return(used);

}   // RequestResponse::EdgeNotifications
#endif

/**
 * Start another request (if previous complete/gone)
 * @date Created 06/08/11
 * @author matthewv
 */
void
RequestResponse::ProcessNextRequest()
{
    // if something is on queue and socket in desired state
    //  start the request
    if (NULL==m_CurRequest.get() && 0!=m_ReqQueue.size())
    {
        if (TcpEventSocket::TS_NODE_ESTABLISHED==GetState())
        {
            // get next buffer object
            m_CurRequest=m_ReqQueue.front();
            m_ReqQueue.pop();

            // setup write ... and write
            m_WriteBuf=m_CurRequest.get();
            WriteAvailCallback();
	}   // if
        else
	{
            Connect();
	}  // else
    }   // if

    return;

}   // RequestResponse::ProcessNextRequest


/**
 * Collect the response the request just sent
 * @date Created 06/08/11
 * @author matthewv
 */
void
RequestResponse::ProcessCurrentResponse()
{
    // if something is on queue and socket in desired state
    //  start the request
    if (NULL!=m_CurRequest.get())
    {
        // setup read
        m_ReadBuf=m_CurRequest.get();
        ReadAvailCallback();
    }   // if

    return;

}   // RequestResponse::ProcessCurrentResponse


/**
 * Notify completion list in proper order
 * @date Created 06/10/11
 * @author matthewv
 */
void
RequestResponse::ProcessRequestNotifications()
{
    // due to the nature of mevent, it is quit possible
    //  to reach this function in a recursive manner.
    //  This simple lock keeps the processing at the "parent"
    //  stack level, assuring that each request is fully
    //  notified before a subsequent one
    if (!m_ReqNotifyLock)
    {
        RequestResponseBufPtr buf;

        m_ReqNotifyLock=true;

        while (0!=m_ReqNotify.size())
        {

            buf=m_ReqNotify.front();
            m_ReqNotify.pop();

            buf->SendCompletion(RW_EDGE_RECEIVED);
        }   // if

        m_ReqNotifyLock=false;
    }   // if

    return;

}   // RequestResponse::ProcessRequestNotifications
