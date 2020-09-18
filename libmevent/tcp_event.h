/**
 * @file tcp_event.h
 * @author matthewv
 * @date April 21, 2011
 * @date Copyright 2011
 *
 * @brief Declarations for event/state machine based tcp socket
 */

#ifndef TCP_EVENT_H
#define TCP_EVENT_H

#include "reader_writer.h"

typedef std::shared_ptr<class TcpEventSocket> TcpEventSocketPtr;

/*! \class TcpEventSocket

*/

/**
 * Connect / Read / Write with a tcp socket
 *
 * Use event manager to handle operations asynchronously

\dot
digraph TcpEventSocket {
a [ label="Closed"];
b [ label="Connecting"];
c [ label="Established"];
d [ label="Reading"];
e [ label="Error"];
f [ label="Timeout"];
g [ label="Writing"];
h [ label="Read and Write"];

a -> b [label="address given"];
b -> e [label="error"];
b -> f [label="timeout"];
b -> c [label="writable"];
c -> d [label="read request"];
d -> e [label="error"];
d -> f [label="timeout"];
d -> d [label="readable"];
d -> c [label="read done"];
d -> h [label="write request"];
c -> g [label="write request"];
g -> e [label="error"];
g -> f [label="timeout"];
g -> g [label="writable"];
g -> c [label="write done"];
g -> h [label="read request"];
h -> e [label="error"];
h -> f [label="timeout"];
h -> h [label="writable"];
h -> h [label="readable"];
h -> d [label="write done"];
h -> g [label="read done"];
c -> a [label="close request"];
e -> a [label="failed"];
f -> a [label="failed"];
}
\enddot
 */
class TcpEventSocket : public ReaderWriter {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  /// list of state nodes
  enum TcpEventSocketNode_e {
    TS_NODE_CLOSED = 200,      //!< socket / connection closed
    TS_NODE_CONNECTING = 201,  //!< connection attempt in process
    TS_NODE_ESTABLISHED = 202, //!< connection successful, doing nothing
    TS_NODE_READING = 203,     //!< awaiting data for read
    TS_NODE_WRITING = 204,     //!< awaiting to write
    TS_NODE_READ_WRITE = 205,  //!< awaiting both read and write
    TS_NODE_ERROR = 206,       //!< unexpected error
    TS_NODE_TIMEOUT = 207,     //!< something took too long
  };

  /// list of state edges
  enum TcpEventSocketEdge_e {
    TS_EDGE_FIRST = 200,
    TS_EDGE_IP_GIVEN = 201,      //!< obtained IP address
    TS_EDGE_CONNECTED = 202,     //!< obtained IP address
    TS_EDGE_WRITE_WAIT = 203,    //!< waiting on dns response
    TS_EDGE_WRITABLE = 204,      //!< waiting on dns response
    TS_EDGE_ERROR = 205,         //!< bad url given
    TS_EDGE_TIMEOUT = 206,       //!< something took too long
    TS_EDGE_READ_WAIT = 207,     //!< can write to connection
    TS_EDGE_READABLE = 208,      //!< data available on socket
    TS_EDGE_CLOSE_REQUEST = 209, //!< asking for socket close

    TS_EDGE_LAST = 210
  };

protected:
  unsigned m_NetIp;         //!< 0 or active IP address (for reporting)
  unsigned short m_NetPort; //!< 0 or active IP port (for reporting)

  bool m_NoLinger; //!< true to reset connection on close
private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  TcpEventSocket();

  virtual ~TcpEventSocket();

  /// connect by giving host ordered IP and port
  bool ConnectHostOrder(MEventMgr *Manager, unsigned IpHostOrder,
                        unsigned short PortHostOrder);

  /// connect by giving network ordered IP and port
  bool ConnectNetOrder(MEventMgr *Manager, unsigned IpNetOrder,
                       unsigned short PortNetOrder);

  /// set whether to force close or close socket normally
  void SetNoLinger(bool Flag = true) { m_NoLinger = Flag; };

  bool SocketOption(int Option, bool Set = true);

  /// debug
  void Dump();

  //
  // statemachine callbacks
  //
  bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &
                                Caller, bool PreNotify) override;

  //
  // Callbacks
  //

  /// allows initialization by independent event thread where appropriate
  virtual void ThreadInit(MEventMgrPtr &Mgr);

  /// External callback used when time value expires
  virtual void TimerCallback();

protected:
  /// internal routine for initiating tcp connect and setting state
  void Connect();

  virtual bool InitiateConnect(int &RetVal, int &Errno);

  /// internal routine to close socket
  virtual void Close();

private:
  TcpEventSocket(const TcpEventSocket &); //!< disabled:  copy operator
  TcpEventSocket &
  operator=(const TcpEventSocket &); //!< disabled:  assignment operator

}; // TcpEventSocket

#endif // ifndef TCP_EVENT_H
