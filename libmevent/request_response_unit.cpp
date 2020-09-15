/**
 * @file request_response_unit.cpp
 * @author matthewv
 * @date Created May 6, 2011
 * @date Copyright 2011, SmarterTravelMedia
 *
 * @brief Unit tests for RequestResponse class
 */

#ifndef UNITTEST_H
    #include "unittest.h"
#endif

#ifndef REQUEST_RESPONSE_H
    #include "request_response.h"
#endif

#ifndef MEVENTMGR_H
    #include "meventmgr.h"
#endif

/**
 * @brief Test object to execute the unit tests
 */

class RequestResponseTester : public CPPUNIT_NS::TestFixture
{
    /****************************************************************
    *  Member functions
    ****************************************************************/
public:
    // default constructor
    RequestResponseTester(void)
    {
        // put test name on log lines, default to debug level
        gLogging.Open(NULL, "RequestResponseTester", LOG_USER, true);
        gLogLevel=LOG_DEBUG;
    };

    // virtual destructor
    virtual ~RequestResponseTester() {return;};

protected:
    // default assignment operator: hidden so as not to be
    // inadvertantly used; someone wanting these will need to derive from this
    // class and make the appropriate methods public
    RequestResponseTester & operator=( const RequestResponseTester& );
    RequestResponseTester(const RequestResponseTester & );

    /****************************************************************
    *  cppUnit tester functions
    ****************************************************************/
public:
    void setUp()
    {
        // put test name on log lines of this test, default to debug level
        gLogging.Open(NULL, "RequestResponseTester", LOG_USER, true);
        gLogLevel=LOG_DEBUG;
    };

#if 0
    void tearDown();
#endif

    void testMemcache();
    void testHandlersocket();

    CPPUNIT_TEST_SUITE( RequestResponseTester );
      CPPUNIT_TEST( testMemcache );
      CPPUNIT_TEST( testHandlersocket );
    CPPUNIT_TEST_SUITE_END();

};  // class RequestResponseTester

// this statically initializes cpp object on test list
CPPUNIT_TEST_SUITE_REGISTRATION( RequestResponseTester );


   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   //
   // CppUnit tests
   //
   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////



/**
 * UNIT TEST class containing a test request to memcache, then exit on response
 * @date 06/08/11  matthewv  Created
 */

class objTestRequest : public MEventObj
{
public:
    bool m_IsGood;               //!< flag to pass error back to main thread
    RequestResponsePtr m_Server; //!< object to process request
    RequestResponseBufPtr m_Buf; //!< buffer for test data
    const char ** m_Requests;
    std::string m_ResponseEnd;
    int m_Count;

    objTestRequest(
        RequestResponse & RR,
        const char ** Requests,
        const char * ResponseEnd) 
        : m_IsGood(true), m_Server(RR), m_Requests(Requests),
          m_ResponseEnd(ResponseEnd), m_Count(0)
    {
    };

    void
    ThreadInit(
        MEventMgrPtr & Mgr)
    {
        const char ** iter;

        // TcpEventSocket contains code to start the socket if
        //  the ip and port are already set
        MEventObj::ThreadInit(Mgr);


        for (iter=m_Requests; NULL!=*iter; ++iter, ++m_Count)
        {
            m_Buf=new RequestResponseBuf;

            if (NULL!=m_Buf.get())
            {
                StateMachinePtr this_ptr;
                this_ptr=this;

                m_Buf->StaticRequestString(*iter);
                m_Buf->SetResponseEndsString(m_ResponseEnd.c_str());

                m_Buf->AddCompletion(this_ptr);
                m_Server->AddRequest(m_Buf);
            }
            else
            {
                Logging(LOG_ERR, "%s:  out of memory error.",
                        __func__);
                m_MgrPtr->Stop();
                m_IsGood=false;
            }   // else
        }   // for
        
        return;

    }   // ThreadInit


    bool 
    EdgeNotification(
        unsigned int EdgeId,               //!< what just happened, what graph edge are we walking
        StateMachinePtr & Caller,          //!< what state machine object initiated the edge
        bool PreNotify)                    //!< for watchers, is the before or after owner processes
        {
            switch(EdgeId)
            {
                case ReaderWriter::RW_EDGE_RECEIVED:
                {
                    RequestResponseBuf * buf;
                    buf=dynamic_cast<RequestResponseBuf *>(Caller.get());
                    if (NULL!=buf)
                        buf->Dump();

                    --m_Count;
                    if (0==m_Count)
                        m_MgrPtr->Stop();
                    break;
                }
            }   // switch

            return(true);
        };

    virtual bool ErrorCallback()
    {
        Logging(LOG_ERR, "%s:  error callback happened",
                __func__);
        m_MgrPtr->Stop();
        m_IsGood=false;

        return(false);
    }   // ErrorCallback


    virtual void TimerCallback()
    {
        // not expecting any timers
        Logging(LOG_ERR, "%s:  timer callback happened",
                __func__);
        m_MgrPtr->Stop();
        m_IsGood=false;
    };

};  // class objTestRequest


/**
 * Perform test of attaching to stm memcache instance
 * @date 06/08/11  matthewv  Created
 */
void
RequestResponseTester::testMemcache()
{
    MEventMgr mgr;
    // app22.www.inm 75.98.73.157
    RequestResponse rr(0x4b62499d, 11211);
    const char * test_msgs[]={"stats\n",NULL};
    objTestRequest test(rr,test_msgs,"END\r\n");
    bool ret_flag;

    mgr.StartThreaded();
    mgr.AddEvent(rr);
    mgr.AddEvent(test);

    // how to wait for manager?
    ret_flag=mgr.ThreadWait();

    CPPUNIT_ASSERT(true==ret_flag);
    CPPUNIT_ASSERT(true==test.m_IsGood);

    return;

}   // RequestResponseTester::testConnect


/**
 * Perform test of attaching to mysql handlersocket
 * @date 06/08/11  matthewv  Created
 */
void
RequestResponseTester::testHandlersocket()
{
    MEventMgr mgr;
    // galaxy 207.154.57.30
    RequestResponse rr(0xcf9a391e, 9998);
    const char * test_msgs[]=
        {
            "P\t1\tstm_node1__user_node\tsubscriptions\tPRIMARY\tuser_id,content_id,parent_content_id\n",
            "1\t>\t3\t0\t0\t0\n",
            "1\t>\t3\t1\t2473393\t0\n",
            "1\t>\t3\t1\t2473393\t0\t10\t0\n",
            NULL
        };
    objTestRequest test(rr,test_msgs,"\n");
    bool ret_flag;

    mgr.StartThreaded();
    mgr.AddEvent(rr);
    mgr.AddEvent(test);

    // how to wait for manager?
    ret_flag=mgr.ThreadWait();

    CPPUNIT_ASSERT(true==ret_flag);
    CPPUNIT_ASSERT(true==test.m_IsGood);

    return;

}   // RequestResponseTester::testHandlersocket

