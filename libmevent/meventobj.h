/**
 * @file meventobj.h
 * @author matthewv
 *
 * @brief Declarations for epoll based event classes
 */

#ifndef MEVENTOBJ_H
#define MEVENTOBJ_H

#include <chrono>
#include <memory>

#include "statemachine.h"

typedef std::shared_ptr<class MEventObj> MEventPtr;
typedef std::shared_ptr<class MEventMgr> MEventMgrPtr;

/**
 * Single event instance, usually tied to one file descriptor
 *
 * The MEventMgr object manages a list of MEventObj objects.  This
 * is a base class for deriving classes that use epoll for event
 * notification.
 */
class MEventObj : public StateMachine {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:
  MEventPtr m_ParentPtr; //!< parent that "owns" this object, or NULL
  MEventMgrPtr m_MgrPtr; //!< parent that is a manager, or NULL

  std::chrono::milliseconds m_Interval; //!< most recent interval used
  std::chrono::steady_clock::time_point m_NextTimeout;
  std::chrono::steady_clock::time_point
      m_LastTimeout; //!< set if previous event was timeout,
                     //    cleared if any other event occurs
  int m_Handle;      //!< file descriptor

  bool m_ForRead;  //!< reading was requested
  bool m_ForWrite; //!< writing was requested

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  MEventObj();

  virtual ~MEventObj();

  /// common initialization used by all constructors
  void Init() {
    m_Handle = -1;
    Reset();
  };

  MEventPtr GetMEventPtr() {
    return std::static_pointer_cast<MEventObj>(GetStateMachinePtr());
  }

  // accessors
  /// Return MEventObj pointer if derived class is MEventObj, NULL otherwise
  virtual class MEventObj *GetMEventObj() { return (this); };

  /// establish the file descriptor (read / write / both)
  void SetFileHandle(int Handle) { m_Handle = Handle; };

  /// return current file descriptor
  int GetFileHandle() const { return (m_Handle); };

  /// set default interval for read/write timeout in milliseconds
  void SetIntervalMS(int TimeMS) {
    m_Interval = std::chrono::milliseconds(TimeMS);
  };

  /// retrieve default interval for read/write timeout in milliseconds
  int GetIntervalMS() const { return (m_Interval.count()); };
  std::chrono::steady_clock::duration GetInterval() const {
    return m_Interval;
  };

  void SetNextTimeout(std::chrono::steady_clock::time_point t_point) {
    m_NextTimeout = t_point;
  }
  std::chrono::steady_clock::time_point GetNextTimeout() const {
    return m_NextTimeout;
  };

  /// retrieve parent pointer
  MEventMgrPtr GetMgrPtr() { return (m_MgrPtr); };

  /// get current state on epoll
  bool IsForRead() const { return (m_ForRead); };

  /// get current state on epoll
  bool IsForWrite() const { return (m_ForWrite); };

  // object ownership control

  /// place on manager's list
  void AssignMgr(MEventMgrPtr &Object);

  // get off lists and throw away any pending pointers
  virtual void release();

  /// establish new timer interval
  bool SetTimer(const std::chrono::milliseconds &Interval);

  /// helper function for setting a new timer
  bool SetTimerMS(unsigned Millisec) {
    std::chrono::milliseconds ms(Millisec);
    return (SetTimer(ms));
  };

  /// reuse old timer interval
  bool RestartTimer();

  /// Erase timer value, eliminating reuse option
  bool ClearTimer();

  /// Note when most recent timeout occurred
  void SetLastTimeout(const std::chrono::steady_clock::time_point &Now) {
    m_LastTimeout = Now;
  };

  // read / write calls will automatically reuse old timer
  //  if established in prior call
  bool RequestRead(bool SetRead = true);

  bool RequestWrite(bool SetWrite = true);

  //
  // callbacks
  //

  /// allows initialization by independent event thread where appropriate
  virtual void ThreadInit(MEventMgrPtr &Mgr) { AssignMgr(Mgr); };

  /// External callback used when time value expires
  virtual void TimerCallback();

  /// External callback when handle contains error flag
  virtual bool ErrorCallback();

  /// External callback when handle contains read flag
  virtual bool ReadAvailCallback();

  /// External callback when handle contains write flag
  virtual bool WriteAvailCallback();

  /// External callback when handle contains HUP and/or RDHUP flag
  virtual bool CloseCallback(int);

  /// debug
  // void Dump() {printf("MEventObj:\n   m_RefCount=%u\n", m_RefCount);};

  /// Internal callback used when timer value expires
  virtual void TimerExpired() {
    SetLastTimeout();
    TimerCallback();
  };

  /// update this variable to now
  void SetLastTimeout() { m_LastTimeout = std::chrono::steady_clock::now(); };

  /// object's handle needs to be cleared
  void Reset();

private:
  MEventObj(const MEventObj &);            //!< disabled:  copy operator
  MEventObj &operator=(const MEventObj &); //!< disabled:  assignment operator

}; // MEventObj

#endif // ifndef MEVENTOBJ_H
