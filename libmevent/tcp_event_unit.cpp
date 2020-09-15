/**
 * @file tcp_event_unit.cpp
 * @author matthewv
 * @date Created May 6, 2011
 * @date Copyright 2011, SmarterTravelMedia
 *
 * @brief Unit tests for TcpEventSocket class
 */

#ifndef UNITTEST_H
#include "unittest.h"
#endif

#ifndef TCP_EVENT_H
#include "tcp_event.h"
#endif

#ifndef MEVENTMGR_H
#include "meventmgr.h"
#endif

/**
 * @brief Test object to execute the unit tests
 */

class TcpEventSocketTester : public CPPUNIT_NS::TestFixture {
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  // default constructor
  TcpEventSocketTester(void) {
    // put test name on log lines, default to debug level
    gLogging.Open(NULL, "TcpEventSocketTester", LOG_USER, true);
    gLogLevel = LOG_DEBUG;
  };

  // virtual destructor
  virtual ~TcpEventSocketTester() { return; };

protected:
  // default assignment operator: hidden so as not to be
  // inadvertantly used; someone wanting these will need to derive from this
  // class and make the appropriate methods public
  TcpEventSocketTester &operator=(const TcpEventSocketTester &);
  TcpEventSocketTester(const TcpEventSocketTester &);

  /****************************************************************
   *  cppUnit tester functions
   ****************************************************************/
public:
  void setUp() {
    // put test name on log lines of this test, default to debug level
    gLogging.Open(NULL, "TcpEventSocketTester", LOG_USER, true);
    gLogLevel = LOG_DEBUG;
  };

#if 0
    void tearDown();
#endif

  void testConnect();

  CPPUNIT_TEST_SUITE(TcpEventSocketTester);
  CPPUNIT_TEST(testConnect);
  CPPUNIT_TEST_SUITE_END();

}; // class TcpEventSocketTester

// this statically initializes cpp object on test list
CPPUNIT_TEST_SUITE_REGISTRATION(TcpEventSocketTester);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CppUnit tests
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/**
 * UNIT TEST class to stop tcp socket operations at a particular edge condition
 * @date 02/28/11  matthewv  Created
 */

class objTestSocket : public TcpEventSocket {
public:
  bool m_IsGood;              //!< flag to pass error back to main thread
  unsigned int m_EndingState; //!< state that marks end of test

  objTestSocket(unsigned int EndingState = 0)
      : m_IsGood(true), m_EndingState(EndingState){};

  virtual void SetState(unsigned int StateId) {
    // pass event down the object
    TcpEventSocket::SetState(StateId);

    // stop the test?
    if (m_EndingState == StateId || TcpEventSocket::TS_NODE_ERROR == StateId) {
      m_IsGood = (m_EndingState == StateId);
      m_MgrPtr->Stop();
    } // if

    return;

  } // StateChange

  virtual bool ErrorCallback() {
    Logging(LOG_ERR, "%s:  error callback happened", __func__);
    m_MgrPtr->Stop();
    m_IsGood = false;

    return (false);
  } // ErrorCallback

  virtual void TimerCallback() {
    // not expecting any timers
    Logging(LOG_ERR, "%s:  timer callback happened", __func__);
    m_MgrPtr->Stop();
    m_IsGood = false;
  };

}; // class objTestSocket

/**
 * Perform simple connect and close
 * @date 05/06/11  matthewv  Created
 */
void TcpEventSocketTester::testConnect() {
  MEventMgr mgr;
  objTestSocket tconnect(TcpEventSocket::TS_NODE_ESTABLISHED);
  bool ret_flag;

  tconnect.SetIntervalMS(20000);

  mgr.StartThreaded();

  // attempt the connect (71.126.247.230)
  tconnect.ConnectHostOrder(&mgr, 0x477ef7e6, 80);

  // how to wait for manager?
  ret_flag = mgr.ThreadWait();

  CPPUNIT_ASSERT(true == ret_flag);
  CPPUNIT_ASSERT(true == tconnect.m_IsGood);

  return;

} // TcpEventSocketTester::testConnect
