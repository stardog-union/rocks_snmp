/**
 * @file meventmgr.cpp
 * @author matthewv
 * @date Created February 22, 2010
 * @date Copyright 2011
 *
 * @brief Event object and event list (manager)
 */

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "meventmgr.h"
#include "logging.h"

// exists in 2.6.17 and after, but not in all headers
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0
#endif

/**
 * Initialize the data members.  On ERROR, leave m_EpollFd set to -1 as flag
 * @date 02/26/10  matthewv  Created
 */
MEventMgr::MEventMgr() {
  m_Running = false;
  m_EndStatus = true;

  m_SelfPipe[0] = -1;
  m_SelfPipe[1] = -1;

  m_TaskCount = 0;

  // step one, create epoll kernel object
  m_EpollFd = epoll_create(100);

  if (-1 == m_EpollFd) {
    Logging(LOG_ERR, "%s: error creating epoll descriptor (errno=%d)", __func__,
            errno);
    m_EndStatus = false;
  } // if

  // step two, create an administrative pipe
  if (IsValid()) {
    int ret_val;

    ret_val = pipe(m_SelfPipe);

    // two point five, pipe needs to be installed in epoll
    if (0 == ret_val) {
      struct epoll_event event;

      ret_val = fcntl(m_SelfPipe[0], F_SETFL, O_NONBLOCK);
      ret_val = fcntl(m_SelfPipe[1], F_SETFL, O_NONBLOCK);

      memset(&event, 0, sizeof(event));
      event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
      event.data.fd = m_SelfPipe[0];

      ret_val = epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, m_SelfPipe[0], &event);

      if (0 != ret_val) {
        Logging(LOG_ERR, "%s: error adding control pipe (errno=%d)", __func__,
                errno);
        Close();
      } // if
    }   // if
    else {
      Logging(LOG_ERR, "%s: error creating admin pipe (errno=%d)", __func__,
              errno);

      Close();
    } // else
  }   // if

  return;

} // MEventMgr::MEventMgr

/**
 * Release resources
 * @date 02/26/10  matthewv  Created
 */
MEventMgr::~MEventMgr() {
  Close();

  PurgeEvents();

  return;

} // MEventMgr::~MEventMgr

/**
 * Clear event objects
 * @date 02/12/12 Created
 * @author matthewv
 */
void MEventMgr::PurgeEvents() {

  // clear all nodes on object and timing lists
  m_Events.clear();
  m_Timeouts.clear();

  return;

} // MEventMgr::PurgeEvents

/**
 * Place an event on the thread safe add list
 *  i.e. this is an asynchronous add
 * @date 03/01/11  matthewv  Created
 */
bool MEventMgr::AddEvent(MEventPtr &Ptr) {
  bool ret_flag;

  ret_flag = true;

  if (Ptr) {
    std::lock_guard<std::mutex> lock(m_AddMutex);
    int ret_val;

    // debug:  m_ThreadedAdd.DumpObjList();
    m_ThreadedAdd.push_back(Ptr);
    // debug:  m_ThreadedAdd.DumpObjList();

    ret_val = write(m_SelfPipe[1], "a", 1);

    if (-1 == ret_val) {
      ret_flag = false;
      Logging(LOG_ERR, "%s: write to manager pipe failed (errno=%d)", __func__,
              errno);
    } // if
  }   // if
  else {
    ret_flag = false;
    Logging(LOG_ERR, "%s: bad function param", __func__);
  } // else

  return (ret_flag);

} // MEventMgr::AddEvent

bool MEventMgr::TimerCreate(MEventPtr Obj) {
  bool ret_flag = {true};
  std::chrono::steady_clock::time_point new_point;

  if (0 != Obj->GetIntervalMS() ) {
    Logging(LOG_ERR, "MEventMgr::%s:  GetIntervalMS %u", __func__, Obj->GetIntervalMS());

    new_point = std::chrono::steady_clock::now() + Obj->GetInterval();

    auto it = m_Timeouts.insert(
      std::pair<std::chrono::steady_clock::time_point, MEventPtr>(new_point,
                                                                  Obj));

    // we do NOT remove previous timepoints for this object.
    //  that occurs during a walk of m_TimePoints later
    if (m_Timeouts.end() != it) {
      Obj->SetNextTimeout(new_point);
    } else {
      ret_flag = false;
    }
  }

  return ret_flag;

} // MEventMgr::TimerCreate

bool MEventMgr::TimerRepeat(MEventPtr &Obj) {
  bool ret_flag = {true};
  std::chrono::steady_clock::time_point new_point;

  if (0 != Obj->GetIntervalMS() ) {
    Logging(LOG_ERR, "MEventMgr::%s:  GetIntervalMS %u", __func__, Obj->GetIntervalMS());

    new_point = Obj->GetNextTimeout() + Obj->GetInterval();

    auto it = m_Timeouts.insert(
      std::pair<std::chrono::steady_clock::time_point, MEventPtr>(new_point,
                                                                  Obj));

    // we do NOT remove previous timepoints for this object.
    //  that occurs during a walk of m_TimePoints later
    if (m_Timeouts.end() != it) {
      Obj->SetNextTimeout(new_point);
    } else {
      ret_flag = false;
    }
  }

  return ret_flag;

} // MEventMgr::TimerRepeat

/**
 * Close the file descriptor related resources (epoll and pipe)
 * @date 02/26/10  matthewv  Created
 */
void MEventMgr::Close() {
  if (-1 != m_SelfPipe[0]) {
    close(m_SelfPipe[0]);
    m_SelfPipe[0] = -1;
  } // if

  if (-1 != m_SelfPipe[1]) {
    close(m_SelfPipe[1]);
    m_SelfPipe[1] = -1;
  } // if

  if (-1 != m_EpollFd) {
    close(m_EpollFd);
    m_EpollFd = -1;
  } // if

  return;

} // MEventMgr::Close

/**
 * @brief Begin event loop processing.  Caller's thread operates the loop.
 * @returns false if exit due to an error
 * @date 03/18/10  matthewv  Created
 */
bool MEventMgr::StartSingle() {
  bool ret_flag;

  ret_flag = true;

  if (IsValid()) {
    std::chrono::steady_clock::time_point now;
    int num_ready;
    struct epoll_event events[10];

    // make sure epoll has "self" object for breaking loop
    //  - in case of signals
    //  - other threads requesting stop
    //  - other threads requesting objects added

    // callbacks can set running to "false" to stop execution
    m_Running = true;
    num_ready = 0;

    // test run flag on each cycle
    while (m_Running && m_EndStatus) {
      int loop;

      // 1. all pending epoll events from prior loop
      for (loop = 0; loop < num_ready; ++loop) {
        // is this a message to the manager
        if (m_SelfPipe[0] == events[loop].data.fd) {
          ReceiveMgrMessage(events[loop].events);
        } // if

        // assume there is a pointer for us to use
        else {
          MEventPtr event;
          bool again;

          again = true;

          // hold an MEventObjPtr in case object releases self
          event = ((MEventObj *)(events[loop].data.ptr))->GetMEventPtr();

          // First: call error on flag (1st because it may overpower read /
          // write)
          if (EPOLLERR & events[loop].events)
            again = event->ErrorCallback();

          // Second: call read avail on flag (2nd because input can overrun)
          if (again && (EPOLLIN & events[loop].events))
            again = event->ReadAvailCallback();

          // Third: call write avail on flag because
          if (again && (EPOLLOUT & events[loop].events))
            again = event->WriteAvailCallback();

          // Fourth: call connection close
          if ((EPOLLRDHUP | EPOLLHUP) & events[loop].events)
            again = event->CloseCallback((EPOLLRDHUP | EPOLLHUP) &
                                         events[loop].events);
        } // else

      } // if

      // 2. get time, any timed events overdue?
      //   (supports potential, non-MEventObj items)
      // Process only ONE event per loop.  Assumption is that
      //  fd based events are more critical than timeouts
      now = std::chrono::steady_clock::now();
      while (m_Timeouts.size() && m_Timeouts.begin()->first < now) {
        std::chrono::steady_clock::time_point point = m_Timeouts.begin()->first;
        MEventPtr event = m_Timeouts.begin()->second;
        m_Timeouts.erase(m_Timeouts.begin());

        // this might be an old timeout, check first
        if (point == event->GetNextTimeout()) {
          event->SetLastTimeout(now);

          // execute timer callback
          event->TimerExpired();
        } // if
      }   // if

      num_ready = 0;

      // 3. set up timed call to epoll
      int milliseconds;

      // use closest timeout
      milliseconds = -1;
      if (m_Timeouts.size()) {
        now = std::chrono::steady_clock::now();
        std::chrono::milliseconds ms;

        if (now < m_Timeouts.begin()->first) {
          ms = std::chrono::duration_cast<std::chrono::milliseconds>(
              m_Timeouts.begin()->first - now);
          milliseconds = ms.count();
        } // if
        else {
          milliseconds = 0;
        } // else
      }   // if

      if (m_Running && m_EndStatus)
        num_ready = epoll_wait(m_EpollFd, events, 10, milliseconds);
    } // while
  }   // if
  else {
    Logging(LOG_ERR, "%s:  Start called on bad MEventMgr object.", __func__);
    ret_flag = false;
  } // else

  m_EndStatus = m_EndStatus && ret_flag;

  // purge objects
  PurgeEvents();

  return (m_EndStatus);

} // MEventMgr::StartSingle

/**
 * Kill the loop.  No more processing.
 * @date 02/19/11  matthewv  Created
 */
void MEventMgr::Stop(
    bool EndStatus) //<! optional true/false to report at object level
{
  int ret_val;

  m_Running = false;
  m_EndStatus = EndStatus;

  // this write is likely NOT needed, but redundancy does not hurt here
  //  (ok, will help if sent by independent thread)
  ret_val = write(m_SelfPipe[1], "x", 1);

  if (1 != ret_val) {
    Logging(LOG_ERR, "%s:  write failed (errno=%d)", __func__, errno);
  } // if

  return;

} // MEventMgr::Stop

bool MEventMgr::StartThreaded() {
  std::thread new_thread(&MEventMgr::StartSingle, this);
  m_Thread = std::move(new_thread);

  return true;
}

/**
 * Spawn a thread that executes Start()
 * @date 03/01/11  matthewv  Created
 */
void *MEventMgr::ThreadStart() {
  return (StartSingle() ? NULL : (void *)1);
} // MEventMgr::ThreadStart

/**
 * Read on or more command messages off the pipe
 * @date 03/01/11  matthewv  Created
 */
void MEventMgr::ReceiveMgrMessage(unsigned Flags) {
  if (Flags & EPOLLERR) {
    Logging(LOG_ERR, "%s: epoll stated error management pipe", __func__);
    m_Running = false;
    m_EndStatus = false;
  } // if

  // a series of one byte commands
  else if (Flags & EPOLLIN) {
    char command;
    int ret_val;

    do {
      ret_val = read(m_SelfPipe[0], &command, 1);

      if (1 == ret_val) {
        switch (command) {
        case 'x':
          m_Running = false;
          break;

        case 'a':
          AddEventList();
          break;

        default:
          m_Running = false;
          m_EndStatus = false;
          Logging(LOG_ERR, "%s: unknown management command \'%c\'", __func__,
                  command);
          break;
        } // switch
      }   // if

      else {
        if (EAGAIN != errno) {
          m_Running = false;
          m_EndStatus = false;
          Logging(LOG_ERR, "%s: error reading management pipe (errno=%d)",
                  __func__, errno);
        } // if
      }   // else
    } while (1 == ret_val);
  } // else

  return;

} // MEventMgr::ReceiveMgrMessage

/**
 * Move new events from m_ThreadAdd list to active list
 * @date 03/01/11  matthewv  Created
 */
void MEventMgr::AddEventList() {
  MEventPtr ptr;
  MEventMgrPtr mgr = GetMEventMgrPtr();

  std::lock_guard<std::mutex> lock(m_AddMutex);

  for (auto it : m_ThreadedAdd) {
    // object will init itself to "this" as parent
    it->ThreadInit(mgr);
    m_Events.insert(it);
  } // for
  m_ThreadedAdd.clear();

  return;

} // MEventMgr::ReceiveMgrMessage

/**
 * Update epoll for current monitoring requirements
 * @date Created 03/02/11
 * @author matthewv
 */
bool MEventMgr::UpdateEpoll(
    MEventPtr &Obj,     //!< event object with active handle
    bool NewReadState,  //!< potentially changed read monitoring state
    bool NewWriteState) //!< potentially changed write monitoring state
{
  bool ret_flag;
  int handle;

  ret_flag = true;

  // validate parameters
  if (Obj)
    handle = Obj->GetFileHandle();
  else
    handle = -1;

  ret_flag = (-1 != handle && this == Obj->GetMgrPtr().get());

  // only do this work with good data, and different states
  if (ret_flag && (Obj->IsForRead() != NewReadState ||
                   Obj->IsForWrite() != NewWriteState)) {
    struct epoll_event event;
    int ret_val, operation, command;
    bool old_read, old_write;

    command = EPOLLERR | EPOLLHUP;

    if (NewReadState)
      command |= EPOLLIN | EPOLLRDHUP;

    if (NewWriteState)
      command |= EPOLLOUT;

    old_read = Obj->IsForRead();
    old_write = Obj->IsForWrite();

    // modify existing epoll
    if ((old_read || old_write) && (NewReadState || NewWriteState)) {
      operation = EPOLL_CTL_MOD;
    } // if

    // delete
    else if (!NewReadState && !NewWriteState) {
      operation = EPOLL_CTL_DEL;
    } // else if

    // add
    else {
      operation = EPOLL_CTL_ADD;
    } // else

    // all handles non-blocking by this library
    if (EPOLL_CTL_ADD == operation)
      ret_val = fcntl(handle, F_SETFL, O_NONBLOCK);
    else
      ret_val = 0;

    if (0 == ret_val) {
      memset(&event, 0, sizeof(event));
      event.events = command;
      event.data.ptr = Obj.get();

      ret_val = epoll_ctl(m_EpollFd, operation, handle, &event);

      if (0 != ret_val) {
        ret_flag = false;
        Logging(LOG_ERR,
                "%s: epoll_ctl failed (errno=%d, operation=0x%x, handle=%d, "
                "command=0x%x)",
                __func__, errno, operation, handle, event.events);
      } // if
    }   // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: fcntl failed to set O_NONBLOCK (errno=%d)",
              __func__, errno);
    } // else
  }   // if
  else if (!ret_flag) {
    ret_flag = false;
    Logging(LOG_ERR, "%s: function param bad or in bad state", __func__);
  } // else if

  return (ret_flag);

} // MEventMgr::UpdateEpoll

/**
 * Remove object
 * @date 03/02/11  matthewv  Created
 */
bool MEventMgr::ReleaseRequests(MEventPtr &Obj) {
  bool ret_flag;
  int handle;

  ret_flag = true;
  if (NULL != Obj)
    handle = Obj->GetFileHandle();
  else
    handle = -1;

  if (-1 != handle && this == Obj->GetMgrPtr().get()) {
    ret_flag = UpdateEpoll(Obj, false, false);
  } // if

#if 0
// can be called on incomplete objects as part of close
    else
    {
        ret_flag=false;
        Logging(LOG_ERR, "%s: function param bad or in bad state", __func__);
    }   // else
#endif

  return (ret_flag);

} // MEventMgr::ReleaseRequests

#if 0
/**
 * Receive "edge" messages from other objects
 * @date Created 06/15/11
 * @author matthewv
 */
bool
MEventMgr::EdgeNotification(
    unsigned int EdgeId,               //!< what just happened, what graph edge are we walking
    StateMachinePtr & Caller,          //!< what state machine object initiated the edge
    bool PreNotify)                    //!< for watchers, is the before or after owner processes
{
    bool used;

    used=false;

    switch(EdgeId)
    {
        case MM_EDGE_TASK_START:
            ++m_TaskCount;
            used=true;
            break;

        case MM_EDGE_TASK_END:
            if (0<m_TaskCount) --m_TaskCount;
            if (0==m_TaskCount && m_Running) m_Running=false;
            used=true;
            break;

        default:
            // send down a level.  If not used then it is an error
            used=StateMachine::EdgeNotification(EdgeId, Caller, PreNotify);
            if (!used)
            {
                Logging(LOG_ERR, "%s: unknown edge value passed [EdgeId=%u]",
                        __func__, EdgeId);
//                m_MgrPtr->Stop();
            }   // if
            break;

    }   // switch

    return(used);

}   // UserStatus::EdgeNotification
#endif
