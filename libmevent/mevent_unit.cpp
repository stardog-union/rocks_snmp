/**
 * @file mevent_unit.cpp
 * @author matthewv
 * @date Created March 2, 2010
 * @date Copyright 2011, SmarterTravelMedia
 *
 * @brief Unit tests for MEventObj and MEventMgr objects
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>

#ifndef UNITTEST_H
#include "unittest.h"
#endif

#ifndef MEVENTMGR_H
#include "meventmgr.h"
#endif

/**
 * @brief Test object to execute the unit tests
 */

class MEventTester : public CPPUNIT_NS::TestFixture {
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  // default constructor
  MEventTester(void) {
    // put test name on log lines, default to debug level
    gLogging.Open(NULL, "MEventTester", LOG_USER, true);
    gLogLevel = LOG_DEBUG;
  };

  // virtual destructor
  virtual ~MEventTester() { return; };

protected:
  // default assignment operator: hidden so as not to be
  // inadvertantly used; someone wanting these will need to derive from this
  // class and make the appropriate methods public
  MEventTester &operator=(const MEventTester &);
  MEventTester(const MEventTester &);

  /****************************************************************
   *  cppUnit tester functions
   ****************************************************************/
public:
  void setUp() {
    // put test name on log lines of this test, default to debug level
    gLogging.Open(NULL, "MEventTester", LOG_USER, true);
    gLogLevel = LOG_DEBUG;
  };

#if 0
    void tearDown();
#endif

  void testOwnership();
  void testOwnership2();
  void testSimpleTimer();
  void testSimplePipe();

  CPPUNIT_TEST_SUITE(MEventTester);
  CPPUNIT_TEST(testOwnership);
  CPPUNIT_TEST(testOwnership2);
  CPPUNIT_TEST(testSimpleTimer);
  CPPUNIT_TEST(testSimplePipe);
  CPPUNIT_TEST_SUITE_END();

}; // class MEventTester

// this statically initializes cpp object on test list
CPPUNIT_TEST_SUITE_REGISTRATION(MEventTester);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CppUnit tests
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/****************************************************************
 * Function:  MEventTester::testOwnership
 *  Purpose:  Verfiy objects deleted at appropriate times by
 *         :   reference counts
 *Revisions:
 *   Date     By         Description (MEventTester::testOwnership)
 * -------- ---------- ------------------------------------------
 * 03/01/10  matthewv  Created.
 ****************************************************************/
void MEventTester::testOwnership() {
  MEventObj stack_obj;
  MEventObj *ptr;

  // validate that stack object not taken out by manager
  ptr = NULL;
  {
    MEventMgr mgr;

    CPPUNIT_ASSERT(mgr.obegin() == mgr.oend());
    CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());

    mgr.assign(stack_obj);

    CPPUNIT_ASSERT(mgr.obegin() != &mgr);
    CPPUNIT_ASSERT(mgr.oend() == &mgr);
    CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());
    CPPUNIT_ASSERT(mgr.tbegin() == &mgr);

    CPPUNIT_ASSERT(mgr.ObjectFirst() == &stack_obj);

    // verify stack object maintained by manager
    {
      MEventObjPtr obj_ptr;
      CircularList::o_iterator it;

      ptr = new MEventObj;
      obj_ptr = ptr;

      mgr.assign(obj_ptr);

      CPPUNIT_ASSERT(mgr.obegin() != mgr.oend());
      CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());

      it = mgr.obegin();
      CPPUNIT_ASSERT(it->GetMEventObj() == &stack_obj);

      ++it;
      CPPUNIT_ASSERT(it->GetMEventObj() == ptr);

      ++it;
      CPPUNIT_ASSERT(it->GetMEventObj() == NULL);
      CPPUNIT_ASSERT(it == mgr.oend());
    }
    // ptr has 4 pointers:  prev/next on two lists
    CPPUNIT_ASSERT(4 == ptr->RefCount());
    // stack_obj has extra ref for being on stack
    CPPUNIT_ASSERT(5 == stack_obj.RefCount());

    CircularList::o_iterator it;

    CPPUNIT_ASSERT(mgr.obegin() != mgr.oend());
    CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());

    it = mgr.obegin();
    CPPUNIT_ASSERT(it->GetMEventObj() == &stack_obj);
    ++it;
    CPPUNIT_ASSERT(it->GetMEventObj() == ptr);
    ++it;
    CPPUNIT_ASSERT(it->GetMEventObj() == NULL);
    CPPUNIT_ASSERT(it == mgr.oend());
  }

  // dangerous, testing a dangling pointer
  //    CPPUNIT_ASSERT(0==ptr->RefCount());
  // stack_obj has extra ref for being on stack
  CPPUNIT_ASSERT(1 == stack_obj.RefCount());
  CPPUNIT_ASSERT(stack_obj.tbegin() == NULL);
  CPPUNIT_ASSERT(stack_obj.obegin() == NULL);

  return;

} // MEventTester::testOwnership

/****************************************************************
 * Function:  MEventTester::testOwnership2
 *  Purpose:  Validate ownership list linkages
 *         :
 *Revisions:
 *   Date     By         Description (MEventTester::testOwnership2)
 * -------- ---------- ------------------------------------------
 * 03/05/10  matthewv  Created.
 ****************************************************************/
void MEventTester::testOwnership2() {
  MEventMgr mgr;
  CircularList::o_iterator it;
  MEventObj *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;

  CPPUNIT_ASSERT(mgr.obegin() == mgr.oend());
  CPPUNIT_ASSERT(mgr.obegin() == &mgr);
  CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());
  CPPUNIT_ASSERT(mgr.tbegin() == &mgr);

  ptr1 = new MEventObj;
  ptr2 = new MEventObj;
  ptr3 = new MEventObj;
  ptr4 = new MEventObj;
  ptr5 = new MEventObj;

  // put all five on list, test forward and backward
  mgr.assign(ptr1);
  ptr2->assign(&mgr);
  mgr.assign(ptr3);
  ptr4->assign(&mgr);
  mgr.assign(ptr5);

  it = mgr.obegin();
  CPPUNIT_ASSERT(&it == ptr1);
  ++it;
  CPPUNIT_ASSERT(&it == ptr2);
  ++it;
  CPPUNIT_ASSERT(&it == ptr3);
  ++it;
  CPPUNIT_ASSERT(&it == ptr4);
  ++it;
  CPPUNIT_ASSERT(&it == ptr5);
  ++it;
  CPPUNIT_ASSERT(&it == &mgr);
  --it;
  CPPUNIT_ASSERT(&it == ptr5);
  --it;
  CPPUNIT_ASSERT(&it == ptr4);
  --it;
  CPPUNIT_ASSERT(&it == ptr3);
  --it;
  CPPUNIT_ASSERT(&it == ptr2);
  --it;
  CPPUNIT_ASSERT(&it == ptr1);
  --it;
  CPPUNIT_ASSERT(&it == &mgr);

  // delete ends
  delete ptr1;
  it = mgr.obegin();
  CPPUNIT_ASSERT(&it == ptr2);
  ++it;
  CPPUNIT_ASSERT(&it == ptr3);
  --it;
  CPPUNIT_ASSERT(&it == ptr2);
  --it;
  CPPUNIT_ASSERT(&it == &mgr);

  delete ptr5;
  it = mgr.oend();
  CPPUNIT_ASSERT(&it == &mgr);
  --it;
  CPPUNIT_ASSERT(&it == ptr4);
  --it;
  CPPUNIT_ASSERT(&it == ptr3);
  ++it;
  CPPUNIT_ASSERT(&it == ptr4);
  ++it;
  CPPUNIT_ASSERT(&it == &mgr);

  // delete middle
  delete ptr3;
  it = mgr.obegin();
  CPPUNIT_ASSERT(&it == ptr2);
  ++it;
  CPPUNIT_ASSERT(&it == ptr4);
  ++it;
  CPPUNIT_ASSERT(&it == &mgr);
  --it;
  CPPUNIT_ASSERT(&it == ptr4);
  --it;
  CPPUNIT_ASSERT(&it == ptr2);
  --it;
  CPPUNIT_ASSERT(&it == &mgr);

  it.clear();

  // test reference counts
  CPPUNIT_ASSERT(4 == ptr2->RefCount());
  CPPUNIT_ASSERT(4 == ptr4->RefCount());
  // 7 -> stack, two parent, 4 link
  CPPUNIT_ASSERT(7 == mgr.RefCount());

  return;

} // MEventTester::testOwnership2

/**
 * UNIT TEST class for validating timer operation
 * @date 03/05/10  matthewv  Created
 */
class objSimpleTimer : public MEventObj {
public:
  Timer m_Pushed, m_Popped;

  objSimpleTimer() { m_Pushed.SetNow(); } // objSimpleTimer

  virtual void TimerCallback() {
    int count;
    t_iterator it;
    Timer compare;
    bool ret_flag;

    // test how long between creating object and this call
    //  -> is it "close" to 2 seconds?
    ret_flag = m_Popped.SetNow();
    CPPUNIT_ASSERT(ret_flag);

    m_Popped.Subtract(m_Pushed);
    m_Popped.Dump();
    // low side of test
    compare.AddMS(1990);
    CPPUNIT_ASSERT(m_Popped.IsGreater(compare));

    // high side of test
    compare.clear();
    compare.AddMS(2010);
    CPPUNIT_ASSERT(m_Popped.IsLess(compare));

    // count number of timer events still active
    for (count = 0, it = m_MgrPtr->tbegin(); m_MgrPtr->tend() != it;
         ++it, ++count)
      ;
    CPPUNIT_ASSERT(1 == count);

    // was last timeout set?
    CPPUNIT_ASSERT(m_LastTimeout.IsSet());

    m_MgrPtr->Stop();
  };

}; // class objSimpleTimer

void MEventTester::testSimpleTimer() {
  MEventMgr mgr;
  CircularList::o_iterator it;
  MEventObjPtr ptr1, ptr2;

  CPPUNIT_ASSERT(mgr.obegin() == mgr.oend());
  CPPUNIT_ASSERT(mgr.tbegin() == mgr.tend());

  // objSimpleTimer defined above
  ptr1 = new objSimpleTimer;
  ptr2 = new objSimpleTimer;

  // put two timers on the queue, call loop
  ptr1->SetIntervalMS(2000);
  mgr.assign(ptr1);
  ptr2->SetIntervalMS(10000);
  mgr.assign(ptr2);

  mgr.StartSingle();

  // test reference counts-> 4 self, 1 ptr?
  CPPUNIT_ASSERT(5 == ptr1->RefCount());
  CPPUNIT_ASSERT(5 == ptr2->RefCount());
  // 7: stack, two parent, 4 list pointers
  CPPUNIT_ASSERT(7 == mgr.RefCount());

  return;

} // MEventTester::testSimpleTimer

/**
 * UNIT TEST class for validating I/O operation via a pipe
 * @date 02/28/11  matthewv  Created
 */
class objSimplePipeOut : public MEventObj {
public:
  bool m_IsGood; //!< flag to pass error back to main thread
  unsigned m_Sent;

  objSimplePipeOut(int Handle, int MilliSec) : m_IsGood(true), m_Sent(0) {
    SetFileHandle(Handle);
    SetIntervalMS(MilliSec);
  } // objSimplePipe

  virtual void ThreadInit(MEventMgrPtr &Mgr) {
    // put self on object list
    Mgr->assign(this);

    if (-1 != m_Handle)
      RequestWrite();
  } // ThreadInit

  virtual bool WriteAvailCallback() {
    bool good;
    struct iovec vec;
    char buffer[1024];
    ssize_t written;

    good = true;

    vec.iov_base = buffer;
    vec.iov_len = sizeof(buffer);

    written = writev(m_Handle, &vec, 1);

    if (-1 != written) {
      m_Sent += written;

      // writing complete at 48K
      if ((48 * sizeof(buffer)) <= m_Sent) {
        // assuming caller holds one pointer
        release();
      } // if
    }   // if

    // error returned
    else {
      Logging(LOG_ERR, "%s:  writev error (errno=%d)", __func__, errno);

      m_IsGood = false;
      m_MgrPtr->Stop();
      good = false;
    } // else

    return (good);

  } // WriteAvailCallback

  virtual bool ErrorCallback() {
    m_IsGood = false;
    Logging(LOG_ERR, "%s:  error callback happened", __func__);
    m_MgrPtr->Stop();
    return (false);
  } // ErrorCallback

  virtual void TimerCallback() {
    m_IsGood = false;
    Logging(LOG_ERR, "%s:  timer callback happened", __func__);
    m_MgrPtr->Stop();
  };

}; // class objSimplePipeOut

class objSimplePipeIn : public MEventObj {
public:
  bool m_IsGood; //!< flag to pass error back to main thread
  unsigned m_Received;

  objSimplePipeIn(int Handle, int MilliSec) : m_IsGood(true), m_Received(0) {
    SetFileHandle(Handle);
    SetIntervalMS(MilliSec);
  } // objSimplePipe

  virtual void ThreadInit(MEventMgrPtr &Mgr) {
    // put self on object list
    assign(Mgr.get());

    if (-1 != m_Handle)
      RequestRead();
  } // ThreadInit

  virtual bool ReadAvailCallback() {
    bool good;
    struct iovec vec;
    char buffer[1024];
    ssize_t red;

    good = true;

    vec.iov_base = buffer;
    vec.iov_len = sizeof(buffer);

    red = readv(m_Handle, &vec, 1);

    if (-1 != red) {
      m_Received += red;

      // writing complete at 48K
      if ((48 * sizeof(buffer)) <= m_Received) {
        m_MgrPtr->Stop();

        // assuming caller holds one pointer
        release();
      } // if
    }   // if

    // error returned
    else {
      Logging(LOG_ERR, "%s:  readv error (errno=%d)", __func__, errno);
      CPPUNIT_ASSERT(EAGAIN != errno);
      CPPUNIT_ASSERT(false);

      m_MgrPtr->Stop();
      good = false;
    } // else

    return (good);

  } // ReadAvailCallback

  virtual bool ErrorCallback() {
    m_IsGood = false;
    Logging(LOG_ERR, "%s:  error callback happened", __func__);
    m_MgrPtr->Stop();
    return (false);
  } // ErrorCallback

  virtual void TimerCallback() {
    // not expecting any timers
    m_IsGood = false;
    Logging(LOG_ERR, "%s:  timer callback happened", __func__);
    m_MgrPtr->Stop();
  };

}; // class objSimplePipeIn

void MEventTester::testSimplePipe() {
  MEventMgr mgr;
  MEventObj test_limit;
  int handles[2], ret_val;
  bool ret_flag;

  // set up a nonblocking pipe
  ret_val = pipe(handles);
  CPPUNIT_ASSERT(-1 != ret_val);

  ret_val = fcntl(handles[0], F_SETFL, O_NONBLOCK);
  CPPUNIT_ASSERT(-1 != ret_val);

  ret_val = fcntl(handles[1], F_SETFL, O_NONBLOCK);
  CPPUNIT_ASSERT(-1 != ret_val);

  // put a timer on the event queue, then start it
  //  on independent thread
  test_limit.SetIntervalMS(2000);
  mgr.assign(test_limit);

  mgr.StartThreaded();

  // add read object first (so nothing happens)
  //  100ms time out
  objSimplePipeIn in_pipe(handles[0], 100);
  mgr.AddEvent(in_pipe);

  // add write object (packets should fly)
  objSimplePipeOut out_pipe(handles[1], 100);
  mgr.AddEvent(out_pipe);

  // how to wait for manager?
  ret_flag = mgr.ThreadWait();

  CPPUNIT_ASSERT(true == ret_flag);
  CPPUNIT_ASSERT(true == in_pipe.m_IsGood);
  CPPUNIT_ASSERT(true == out_pipe.m_IsGood);
  CPPUNIT_ASSERT((48 * 1024) == out_pipe.m_Sent);
  CPPUNIT_ASSERT((48 * 1024) == in_pipe.m_Received);
  return;

} // MEventTester::testSimplePipe
