/**
 * @file http_request.h
 * @author matthewv
 * @date March 25, 2011
 * @date Copyright 2011, SmarterTravelMedia
 *
 * @brief Declarations for epoll based event classes
 */

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <iterator>
#include <sys/time.h>

#ifndef TIMER_H
#include "timer.h"
#endif

#ifndef MEVENTLIST_H
#include "meventlist.h"
#endif

#ifndef STATEMACHINE_H
#include "statemachine.h"
#endif

typedef RefPtr<class HttpRequest> HttpRequestPtr;

/*! \class HttpRequest

*/

/**
 */

/**
 * Request / Retrieve a single http page / object
 *
 * Use event manager and dns cache to asynchronously request
 * URL content

\dot
digraph HttpRequest {
a [ label="Url Given"];
b [ label="Request Ip"];
c [ label="Wait DNS"];
d [ label="Connect Ip"];
e [ label="Error"];
f [ label="Timeout"];
g [ label="Send Request"];
h [ label="Get Response"];
i [ label="Notify Watcher"];

a -> b [label="url good"];
a -> e [label="error"];
a -> d [label="have IP"];
b -> c [label="wait IP"];
b -> d [label="have IP"];
c -> d [label="have IP"];
c -> e [label="error"];
c -> f [label="timeout"];
d -> e [label="error"];
d -> f [label="timeout"];
d -> g [label="writable"];
g -> g [label="writable"];
g -> e [label="error"];
g -> f [label="timeout"];
g -> h [label="sent"];
h -> h [label="readable"];
h -> e [label="error"];
h -> f [label="timeout"];
h -> i [label="received"];
e -> i [label="failed"];
f -> i [label="failed"];
}
\enddot
 */
class HttpRequest : public StateMachine, public MEventObj {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  /// list of state nodes
  enum HttpRequestNode_e {
    HR_NODE_BASE = 100,      //!< first enumeration for reference, not used
    HR_NODE_URL = 101,       //!< url request call occurred
    HR_NODE_REQUESTIP = 102, //!< convert domain to ip list
    HR_NODE_DNSWAIT = 103,   //!< waiting for domain to resolve
    HR_NODE_CONNECT = 104,   //!< attempting connection
    HR_NODE_ERROR = 105,     //!< parameter or OS error
    HR_NODE_TIMEOUT = 106,   //!< something took too long
    HR_NODE_REQUEST = 107,   //!< sending GET request
    HR_NODE_RESPONSE = 108,  //!< awaiting response
    HR_NODE_COMPLETE = 109,  //!< done, notify watchers
  };

  /// list of state edges
  enum HttpRequestEdge_e {
    HR_EDGE_URLGOOD = 101,  //!< url parse is successful
    HR_EDGE_HAVE_IP = 102,  //!< obtained IP address
    HR_EDGE_WAIT_IP = 103,  //!< waiting on dns response
    HR_EDGE_ERROR = 104,    //!< bad url given
    HR_EDGE_TIMEOUT = 105,  //!< something took too long
    HR_EDGE_WRITABLE = 106, //!< can write to connection
    HR_EDGE_SENT = 107,     //!< request completely sent
    HR_EDGE_READABLE = 108, //!< data available on socket
    HR_EDGE_RECEIVED = 109, //!< response completely received
    HR_EDGE_FAILED = 110,   //!< processing failed to complete
  };

protected:
  ParsedUrl m_Url;   //!< page requested
  std::vector m_Ips; //!< list of potential IP addresses

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  HttpRequest();

  virtual ~HttpRequest();

  /// initiate lookup of a page
  bool GetHttpPage(const char *Url, unsigned Ip = 0, StateMachine *Watcher);

  /// debug
  void Dump();

protected:
private:
  HttpRequest(const HttpRequest &); //!< disabled:  copy operator
  HttpRequest &
  operator=(const HttpRequest &); //!< disabled:  assignment operator

}; // HttpRequest

#endif // ifndef HTTP_REQUEST_H
