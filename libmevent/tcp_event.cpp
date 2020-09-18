/**
 * @file tcp_event.cpp
 * @author matthewv
 * @date May 6, 2011
 * @date Copyright 2011, SmarterTravelMedia
 *
 * @brief Implementation of event/state machine based tcp socket
 */

#include <arpa/inet.h>
#include <memory.h>
#include <sys/fcntl.h>

#include "meventmgr.h"
#include "tcp_event.h"
#include "util/logging.h"

/**
 * Initialize the data members.
 * @date 05/06/11  matthewv  Created
 */
TcpEventSocket::TcpEventSocket() : m_NetIp(0), m_NetPort(0), m_NoLinger(false) {
  SetState(TS_NODE_CLOSED);
} // TcpEventSocket::TcpEventSocket

/**
 * Release resources
 * @date 05/06/11  matthewv  Created
 */
TcpEventSocket::~TcpEventSocket() {
  Close();

} // TcpEventSocket::~TcpEventSocket

/**
 * Initiate a tcp connection given host order data
 * @date 05/06/11  matthewv  Created
 * @returns false if failure, true on successful connection initiation
 */
bool TcpEventSocket::ConnectHostOrder(
    MEventMgr *Manager,           //!< manager object to own this connection
    unsigned IpHostOrder,         //!< host ordered IP address as unsigned
    unsigned short PortHostOrder) //!< host ordered port address as short
{
  bool good;
  unsigned net_ip;
  unsigned short net_port;

  net_ip = htonl(IpHostOrder);
  net_port = htons(PortHostOrder);

  good = ConnectNetOrder(Manager, net_ip, net_port);

  return (good);

} // TcpEventSocket::ConnectHostOrder

/**
 * Initiate a tcp connection given network order data
 * @date 05/06/11  matthewv  Created
 * @returns false if failure, true on successful connection initiation
 */
bool TcpEventSocket::ConnectNetOrder(
    MEventMgr *Manager,          //!< manager object to own this connection
    unsigned IpNetOrder,         //!< network ordered IP address as unsigned
    unsigned short PortNetOrder) //!< network ordered port address as short
{
  bool good;

  if (NULL != Manager) {
    good = true;
    Close();

    // save the key info
    m_NetIp = IpNetOrder;
    m_NetPort = PortNetOrder;

    // start connect sequence on assigned event manager
    MEventPtr shared = GetMEventPtr();
    good = Manager->AddEvent(shared);
  } // if
  else {
    good = false;
    Logging(LOG_ERR, "%s: No assigned event manager.", __func__);
  } // else

  return (good);

} // TcpEventSocket::ConnectNetOrder

/**
 * First step in asynchronous connection
 * @date 05/09/11  matthewv  Created
 */
void TcpEventSocket::ThreadInit(MEventMgrPtr &Mgr) {
  // let derived objects setup as needed, puts "this" on Mgr's object list
  //  (also puts it on timer list if objects time interval set)
  ReaderWriter::ThreadInit(Mgr);

  // Initiate connection code, will set state and send events
  if (0 != m_NetIp && 0 != m_NetPort) {
    SendEdge(TS_EDGE_IP_GIVEN);
  } // if

  // for now do nothing if no ip/port
  else {
  } // else

  return;

} // TcpEventSocket::ThreadInit

/**
 * A read or write request took too long.
 * @date Created 05/13/11
 * @author matthewv
 */
void TcpEventSocket::TimerCallback() {
  // tell the world we ran out of time
  SendEdge(TS_EDGE_TIMEOUT);

  // unless derived class eats the event, EdgeNotifications will
  //  close the socket

} // ReaderWriter::TimerCallback

/**
 * Setup a new socket and initiate connect
 * @date 05/09/11  matthewv  Created
 */
void TcpEventSocket::Connect() {
  int ret_val, err_num;
  bool good;

  // reset our socket state, be sure watches know the
  //  the connection changed
  SendEdge(TS_EDGE_CLOSE_REQUEST);

  // do protocol specific connect
  good = InitiateConnect(ret_val, err_num);

  if (good) {
    // connect() called, review its state
    if (0 != ret_val && EINPROGRESS == err_num) {
      // set up to await Write enabled
      RequestWrite();
      SetState(TS_NODE_CONNECTING);
      SendEdge(TS_EDGE_WRITE_WAIT);

    } // if

    // unlikely, maybe on localhost call?
    else if (0 == ret_val) {
      // connected state
      SetState(TS_NODE_ESTABLISHED);
      SendEdge(TS_EDGE_CONNECTED);
    } // else if

    else {
      // error
      Logging(LOG_ERR, "%s:  connect() failed [err_num=%d, fd=%d].", __func__,
              err_num, m_Handle);
      SendEdge(TS_EDGE_ERROR);
      ErrorCallback();
    } // else
  }   // if

  return;

} // TcpEventSocket::Connect

/**
 * Routine to virtualize first steps of connect() so non-tcp sockets possible
 * @date 01/16/12 Created
 * @author matthewv
 * @returns true if RetVal & Errno are valid, false if member values bad
 */
bool TcpEventSocket::InitiateConnect(
    int &RetVal, //!< [output] value returned by connect()
    int &Errno)  //!< [output] errno after connect()
{
  bool ret_flag;

  ret_flag = false;
  RetVal = -1;
  Errno = 0;

  // validate connection info
  if (0 != m_NetIp && 0 != m_NetPort) {
    m_Handle = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 != m_Handle) {
      struct sockaddr_in sa;

      SocketOption(O_NONBLOCK);

      // bind the server side
      memset(&sa, 0, sizeof(sa));
      sa.sin_family = AF_INET;
      sa.sin_port = m_NetPort;

      sa.sin_addr.s_addr = m_NetIp;

      // now do the connect
      RetVal = connect(m_Handle, (struct sockaddr *)&sa, sizeof(sa));
      Errno = errno;
      ret_flag = true;
    } // if

    else {
      Logging(LOG_ERR,
              "%s: socket() failed with %d, m_NetIp 0x%x, m_NetPort %d",
              __func__, errno, m_NetIp, m_NetPort);
    } // if

  } // if
  else {
    Logging(LOG_ERR, "%s: zero ip and/or port [ip=%u, port=%hu]", __func__,
            m_NetIp, m_NetPort);
    SendEdge(TS_EDGE_ERROR);
    ErrorCallback();
  } // else

  return (ret_flag);

} // TcpEventSocket::InitiateConnect

/**
 * Close existing socket, notify
 * @date 05/09/11  matthewv  Created
 */
void TcpEventSocket::Close() {
  if (-1 != m_Handle) {
    int ret_val;

    if (m_NoLinger) {
      struct linger linger = {1, 0};

      // const char * for Windows
      ret_val = setsockopt(m_Handle, SOL_SOCKET, SO_LINGER,
                           (const char *)&linger, sizeof(linger));

      if (0 != ret_val) {
        Logging(LOG_ERR, "%s: Error with setsockopt for SO_LINGER [errno=%d]",
                __func__, errno);
      } // if
    }   // if

    // go by full spec and perform shutdown
    ret_val = shutdown(m_Handle, SHUT_RDWR);
    if (0 != ret_val) {
      Logging(LOG_ERR, "%s: Error with shutdown() [errno=%d]", __func__, errno);
    } // if

    // this will close the socket descriptor and send messages to
    //  change state
    ReaderWriter::Close();

    SetState(TS_NODE_CLOSED);
  } // if

  return;

} // TcpEventSocket::Close

/**
 * @brief Change fcntl options cleanly
 *
 * @date Created 01/31/05
 * @author matthewv
 * @returns true if successful, false on failure
 */
bool TcpEventSocket::SocketOption(int Option, bool Set) {
  bool ret_flag = true;

  int ret_val;

  ret_val = fcntl(m_Handle, F_GETFL, 0);
  if (-1 != ret_val) {
    if (Set)
      ret_val |= Option;
    else
      ret_val &= ~Option;

    ret_val = fcntl(m_Handle, F_SETFL, ret_val);

    if (-1 == ret_val) {
      Logging(LOG_ERR, "%s: F_SETFL failed [errno=%d, fd=%d].", __func__, errno,
              m_Handle);
      ret_flag = false;
    } // if
  }   // if
  else {
    Logging(LOG_ERR, "%s: F_GETFL failed [errno=%d, fd=%d].", __func__, errno,
            m_Handle);
    ret_flag = false;
  } // else

  return (ret_flag);

}; // TcpEventSocket::SocketOption

/**
 * @brief Turn edge notifications into State changes
 *
 * @date Created 05/09/11
 * @author matthewv
 * @returns  true if edge handled to state transition
 */
bool
TcpEventSocket::EdgeNotification(
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
            case TS_EDGE_IP_GIVEN:
                Connect();
                used=true;
                break;

            case TS_EDGE_CONNECTED:
              SetState(TS_NODE_ESTABLISHED);
                used=true;
                break;

            case TS_EDGE_WRITE_WAIT:
                used=true;
                break;

            case TS_EDGE_WRITABLE:
                used=true;
                break;

            case TS_EDGE_ERROR:
                              SetState(TS_NODE_ERROR);
              SendEdge(TS_EDGE_CLOSE_REQUEST);
                break;

            case TS_EDGE_TIMEOUT:
              SetState(TS_NODE_TIMEOUT);
              SendEdge(TS_EDGE_CLOSE_REQUEST);
                break;

            case TS_EDGE_READ_WAIT:
                break;

            case TS_EDGE_READABLE:
                break;

            case TS_EDGE_CLOSE_REQUEST:
                if (-1!=m_Handle)
                    Close();
                used=true;
                break;

            //
            // reader_writer events to watch
            //
            case RW_EDGE_WRITABLE:
                if (TS_NODE_CONNECTING==m_CurNode)
                  SendEdge(TS_EDGE_CONNECTED);
                else
                  SendEdge(TS_EDGE_WRITABLE);
                used=true;
                break;

            // a write buffer fully sent
            case RW_EDGE_SENT:
                used=true;
                break;


            default:
                // send down a level.  If not used then it is an error
                used=ReaderWriter::EdgeNotification(EdgeId, Caller, PreNotify);
                if (!used && TS_EDGE_FIRST < EdgeId && TS_EDGE_LAST > EdgeId)
                {
                    Logging(LOG_ERR, "%s: unknown edge value passed [EdgeId=%u]",
                            __PRETTY_FUNCTION__, EdgeId);
                    ErrorCallback();
                }   // if
                break;
        }   // switch
    }   // if

    else
    {
        used=ReaderWriter::EdgeNotification(EdgeId, Caller, PreNotify);
    }   // else

    return(used);

}   // TcpEventSocket::EdgeNotifications
