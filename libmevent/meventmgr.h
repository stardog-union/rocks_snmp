/**
 * @file meventmgr.h
 * @author matthewv
 * @date February 22, 2010
 * @date Copyright 2011
 *
 * @brief Declarations for epoll based event classes
 */

#ifndef MEVENTMGR_H
#define MEVENTMGR_H

#include <chrono>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "meventobj.h"

/**
 * Object that manages lists of MEventObj.
 * MEventMgr activates various MEventObj callback functions as events occur.
 * - Derived from CircularList so it can be anchor to timer list
 */
class MEventMgr {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
  /// list of state nodes
  enum MEventMgrNode_e {
    MM_NODE_INITIALIZED = 1, //!< object initialized properly
    MM_NODE_RUNNING = 2,     //!< epoll loop is executing
    MM_NODE_SHUTDOWN = 3,    //!< epoll loop is exiting
    MM_NODE_STOPPED = 4,     //!< epoll loop terminated
  };

  /// list of state edges
  enum MEventMgrEdge_e {
    MM_EDGE_START = 1,      //!< begin epoll loop
    MM_EDGE_SHUTDOWN = 2,   //!< end epoll loop
    MM_EDGE_TASK_START = 3, //!< increment task tracking
    MM_EDGE_TASK_END = 4,   //!< decrement task tracking
    MM_EDGE_ERROR = 5,      //!< error happened
  };

protected:
  bool m_Running;    //!< true when while loop should be active
  bool m_EndStatus;  //!< true loop exited without errors
  int m_EpollFd;     //!< file handle used by epoll
  int m_SelfPipe[2]; //!< pipe to facilitate stopping epoll loop
  int m_TaskCount;   //!< when used, controls auto shutdown

  std::mutex m_AddMutex; //!< synchronization for adding to m_ThreadedAdd
  std::vector<MEventPtr> m_ThreadedAdd; //!< new events from non-manager threads

  std::set<MEventPtr> m_Events;
  std::multimap<std::chrono::steady_clock::time_point, MEventPtr> m_Timeouts;

  std::thread m_Thread;

  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  MEventMgr();

  virtual ~MEventMgr();

  void Close();

  /// test if initialization succeeded
  bool IsValid() const { return (-1 != m_EpollFd && m_EndStatus); };
  /// test if initialization succeeded
  operator bool() const { return (IsValid()); };

#if 0
    /// establish relationship with an object
    void assign(MEventPtr & Obj) {Obj.assign(this);};
    /// establish relationship with an object
    void assign(MEventObj * Ptr)
        {if (NULL!=Ptr) Ptr->assign(this);};
    /// establish relationship with an object
    void assign(MEventPtr & Ptr)
        {if (NULL!=Ptr.get()) Ptr->assign(this);};
#endif
  /// Thread safe means to add event to manager
  bool AddEvent(MEventPtr &Ptr);

  /// Return MEventObj pointer if derived class is MEventObj, NULL otherwise
  virtual class MEventObj *GetMEventObj() { return (NULL); };

  //
  // timer only functions
  //
  /// establish timer on object
  bool TimerCreate(MEventPtr &Obj);
  /// repeat previous timer on object
  bool TimerRepeat(MEventPtr &Obj);

  /// accessor to first item on timer list
  MEventPtr TimerFirst() { return (m_Timeouts.begin()->second); }
  /// accessor to end of timer list
  MEventPtr TimerLast() { return (m_Timeouts.end()->second); };

  /// accessor to first item on object list
  MEventPtr ObjectFirst() { return (*m_Events.begin()); };
  /// accessor to end of object list
  MEventPtr ObjectLast() { return (*m_Events.end()); };

  //
  // execution control
  //

  /// Single thread model start
  bool StartSingle();

  /// Multi thread start
  bool StartThreaded();

  /// Stop for both thread models, non blocking
  void Stop(bool EndStatus = true);

  /// Wait for previous Stop() to complete, blocking
  bool ThreadWait() {
    m_Thread.join();
    return (m_EndStatus);
  };

  //
  // Object event requests
  //

  // set/reset epoll monitoring for reading / writing
  bool UpdateEpoll(MEventPtr &Obj, bool NewReadFlag, bool NewWriteFlag);

  /// take object off the epoll list
  bool ReleaseRequests(MEventPtr &Obj);

  //
  // StateMachine interface
  //
  /// Public routine to receive Edge notification
  //    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr &
  //    Caller,
  //                                  bool PreNotify);

  /// debug
  //    void Dump() {printf("MEventMgr:\n   m_RefCount=%u\n", m_RefCount);};

protected:
  /// release all attached events
  void PurgeEvents();

  /// Callback used when timer value expires
  virtual void TimerExpired(){};

  /// ThreadControl's entry point
  virtual void *ThreadStart();

  /// process messages sent to manager via pipe
  void ReceiveMgrMessage(unsigned Flags);

  /// copy items from m_ThreadAdd to main object list
  void AddEventList();

private:
  MEventMgr(const MEventMgr &);            //!< disabled:  copy operator
  MEventMgr &operator=(const MEventMgr &); //!< disabled:  assignment operator

}; // class MEventMgr

#endif // ifndef MEVENTMGR_H
