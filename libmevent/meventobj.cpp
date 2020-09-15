/**
 * @file meventobj.cpp
 * @author matthewv
 * @date Created February 22, 2010
 * @date Copyright 2011
 *
 * @brief Event object
 */

#include <errno.h>
#include <memory.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "meventmgr.h"
#include "util/logging.h"

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//
// MEventObj
//     Base class for deriving classes that use epoll for
//     event notification.
//
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

/**
 * @brief Object creation
 * @date 03/05/10 matthewv Created
 */
MEventObj::MEventObj() {
  Init();

  return;
} // MEventObj::MEventObj

/**
 * Release resources
 * @date 03/05/10 matthewv Created
 */
MEventObj::~MEventObj() {
  if (-1 != m_Handle) {
    close(m_Handle);
    m_Handle = -1;
  } // if

  return;

} // MEventObj::~MEventObj

/**
 * @brief Common initialization used by multiple constructors
 * @date 02/28/11 matthewv Created
 */
void MEventObj::Reset() {
  if (-1 != m_Handle) {
    // close should remove from epoll list
    close(m_Handle);
    m_Handle = -1;
  } // if

  m_ForRead = false;
  m_ForWrite = false;

  return;

} // MEventObj::MEventObj

/**
 * Put this object on to the management lists of the given parent
 * @date 03/02/10  matthewv  Created
 */
void MEventObj::AssignMgr(MEventMgrPtr &Parent) // parent to manage this object
{
  if (Parent && Parent->IsValid()) {
    m_MgrPtr = Parent;

    // place on timer list if m_Interval set
    if (0 != m_Interval.count()) {
      MEventPtr shared = GetMEventPtr();
      m_MgrPtr->TimerCreate(shared);
    }
  } // if
  else {
    Logging(LOG_ERR, "%s: param error invalid MEventMgr", __func__);
  } // else

  return;

} // MEventObj::assign

/**
 * Take this object off of lists.  And release parent pointer.
 * @date 03/02/10  matthewv  Created
 */
void MEventObj::release() {
  if (NULL != m_MgrPtr.get()) {
    MEventPtr shared = GetMEventPtr();
    m_MgrPtr->ReleaseRequests(shared);
  }

  m_MgrPtr.reset();

  return;

} // MEventObj::release

/**
 * Save requested time interval and add/update object on manager's timer list
 *
 * @returns true if timer setup succeeded
 * @date 03/18/10  Created
 * @author matthewv
 */
bool MEventObj::SetTimer(const std::chrono::milliseconds &Interval) {
  bool ret_flag;
  MEventPtr shared = GetMEventPtr();

  m_Interval = Interval;

  ret_flag = m_MgrPtr->TimerCreate(shared);

  if (!ret_flag) {
    Logging(LOG_ERR, "%s: unable to add timer to list", __func__);
  } // if

  return (ret_flag);

} // MEventObj::SetTimer

/**
 * Save requested time interval and add/update object on manager's timer list
 *
 * @returns true if timer setup succeeded
 * @date 03/18/10  Created
 * @author matthewv
 */
bool MEventObj::RestartTimer() {
  bool ret_flag = {true};
  MEventPtr shared = GetMEventPtr();

  // base new time off last interval and timeout
  if (0 != m_NextTimeout.time_since_epoch().count() && m_Interval.count()) {
    ret_flag = m_MgrPtr->TimerRepeat(shared);
  } // if

  // bad use of ResetTimer ... make something up
  else if (m_Interval.count()) {
    ret_flag = m_MgrPtr->TimerCreate(shared);
  } else {
    ret_flag = false;
  }

  return (ret_flag);

} // MEventObj::ResetTimer

/**
 * Mark this object as desiring read event monitoring
 *
 * @returns true upon successful mark
 *
 * @date 03/18/11  matthewv  Created
 */
bool MEventObj::RequestRead(
    bool SetRead) //!< flag to set / release epoll monitoring for reads
{
  bool ret_flag;

  ret_flag = true;

  if (m_ForRead != SetRead) {
    if (NULL != m_MgrPtr.get()) {
      // set the flag after the call to tell between
      //  add and modify
      MEventPtr shared = GetMEventPtr();
      ret_flag = m_MgrPtr->UpdateEpoll(shared, SetRead, m_ForWrite);
      if (ret_flag)
        m_ForRead = SetRead;
    } // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: no management object assigned", __func__);
    } // else
  }   // if

  return (ret_flag);

} // MEventObj::RequestRead

/**
 * Mark this object as desiring write event monitoring
 *
 * @returns true upon successful mark
 *
 * @date 03/18/11  matthewv  Created
 */
bool MEventObj::RequestWrite(
    bool SetWrite) //!< true to set the write flag in epoll
{
  bool ret_flag;

  ret_flag = true;

  if (m_ForWrite != SetWrite) {
    if (NULL != m_MgrPtr.get()) {
      // set the flag after the call to tell between
      //  add and modify
      MEventPtr shared = GetMEventPtr();
      ret_flag = m_MgrPtr->UpdateEpoll(shared, m_ForRead, SetWrite);
      if (ret_flag)
        m_ForWrite = SetWrite;
    } // if
    else {
      ret_flag = false;
      Logging(LOG_ERR, "%s: no management object assigned", __func__);
    } // else
  }   // if

  return (ret_flag);

} // MEventObj::RequestWrite

/// External callback when timer expires
void MEventObj::TimerCallback() {
  if (NULL != m_MgrPtr.get())
    m_MgrPtr->Stop();

  return;
} // MEventObj::WriteAvailCallback

/// External callback when handle contains error flag
bool MEventObj::ErrorCallback() {
  if (NULL != m_MgrPtr.get())
    m_MgrPtr->Stop();

  return (false);
} // MEventObj::ErrorCallback

/// External callback when handle contains read flag
bool MEventObj::ReadAvailCallback() {
  if (NULL != m_MgrPtr.get())
    m_MgrPtr->Stop();

  return (false);
} // MEventObj::ReadAvailCallback

/// External callback when handle contains write flag
bool MEventObj::WriteAvailCallback() {
  if (NULL != m_MgrPtr.get())
    m_MgrPtr->Stop();

  return (false);
} // MEventObj::WriteAvailCallback

/// External callback when handle contains HUP and/or RDHUP flag
bool MEventObj::CloseCallback(int) {

  return (true);

} // MEventObj::CloseCallback
