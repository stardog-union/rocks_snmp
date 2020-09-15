/**
 * @file snmp_agent_unit.cpp
 * @author matthewv
 * @date Created July 2, 2011
 * @date Copyright 2011-2012
 *
 * @brief Unit tests for SnmpAgent class
 */

#ifndef UNITTEST_H
    #include "unittest.h"
#endif

#ifndef SNMP_AGENT_H
    #include "snmp_agent.h"
#endif

#ifndef VAL_INTEGER_H
    #include "val_integer.h"
#endif

#ifndef MEVENTMGR_H
    #include "meventmgr.h"
#endif

/**
 * @brief Test object to execute the unit tests
 */

class SnmpAgentTester : public CPPUNIT_NS::TestFixture
{
    /****************************************************************
    *  Member functions
    ****************************************************************/
public:
    // default constructor
    SnmpAgentTester(void) {};

    // virtual destructor
    virtual ~SnmpAgentTester() {return;};

protected:
    // default assignment operator: hidden so as not to be
    // inadvertantly used; someone wanting these will need to derive from this
    // class and make the appropriate methods public
    SnmpAgentTester & operator=( const SnmpAgentTester& );
    SnmpAgentTester(const SnmpAgentTester & );

    /****************************************************************
    *  cppUnit tester functions
    ****************************************************************/
public:
    void setUp()
    {
        // put test name on log lines of this test, default to debug level
        gLogging.Open(NULL, "SnmpAgentTester", LOG_USER, true);
        gLogLevel=LOG_DEBUG;
    };

#if 0
    void tearDown();
#endif

    void testAgentOpen();
    void testGauge32();

    CPPUNIT_TEST_SUITE( SnmpAgentTester );
//      CPPUNIT_TEST( testAgentOpen );
      CPPUNIT_TEST( testGauge32 );
    CPPUNIT_TEST_SUITE_END();

};  // class SnmpAgentTester

// this statically initializes cpp object on test list
CPPUNIT_TEST_SUITE_REGISTRATION( SnmpAgentTester );


   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   //
   // CppUnit tests
   //
   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

class objAgentOpen : public SnmpAgent
{
public:
    objAgentOpen(SnmpAgentId & AgentId, unsigned IpHostOrder, unsigned PortHostOrder,
              StartupListObject ** Startup=NULL)
        : SnmpAgent(AgentId, IpHostOrder, PortHostOrder, Startup) {};

    virtual ~objAgentOpen() {};

    // stop machine once registered
    virtual bool EdgeNotification(unsigned int EdgeId, StateMachinePtr & Caller, bool PreNotify)
        {
            bool ret_flag;

            ret_flag=SnmpAgent::EdgeNotification(EdgeId, Caller, PreNotify);

            // test for shutdown
            if (SA_NODE_REGISTERED==GetState())
            {
                m_MgrPtr->Stop();
            }   // if

            return(ret_flag);
        }   // EdgeNotification

};  // class objAgentOpen

/**
 * Test attaching to active snmp agentX master
 * @date 07/02/11  matthewv  Created
 */
void
SnmpAgentTester::testAgentOpen()
{
    MEventMgr mgr;
    const unsigned agent_prefix[]={1,38693,1,3};
    SnmpAgent::SnmpAgentId id={agent_prefix, sizeof(agent_prefix)/sizeof(agent_prefix[0]),
                               "SnmpAgentTester"};
    objAgentOpen sa(id, 0x7f000001, 705);
    bool ret_flag;

    mgr.StartThreaded();
    mgr.AddEvent(sa);

    // how to wait for manager?
    ret_flag=mgr.ThreadWait();

    CPPUNIT_ASSERT(true==ret_flag);
//    CPPUNIT_ASSERT(true==test.m_IsGood);

    return;

}   // SnmpAgentTester::testAgentOpen


/**
 * Test creation of static unsigned int gauge
 * @date 09/25/11  matthewv  Created
 */

static StartupListObject * sGaugeList=NULL;
static const unsigned sGauge32[]={0, 1};
static SnmpOid sGauge32Id={sGauge32, sizeof(sGauge32)/sizeof(sGauge32[0])};

SnmpValStaticGauge32 gStaticTestGauge32(&sGaugeList, sGauge32Id);


void
SnmpAgentTester::testGauge32()
{

    MEventMgr mgr;
    const unsigned agent_prefix[]={1,38693,1,3};
    SnmpAgent::SnmpAgentId id={agent_prefix, sizeof(agent_prefix)/sizeof(agent_prefix[0]),
                               "SnmpAgentTester"};
    SnmpAgent sa(id, 0x7f000001, 705, &sGaugeList);
    bool ret_flag;

    mgr.StartThreaded();
    mgr.AddEvent(sa);

    // how to wait for manager?
    ret_flag=mgr.ThreadWait();

    CPPUNIT_ASSERT(true==ret_flag);
//    CPPUNIT_ASSERT(true==test.m_IsGood);

    return;

}   // SnmpAgentTester::testGauge32




#if 0
/**
 * Perform test of attaching to mysql handlersocket
 * @date 06/08/11  matthewv  Created
 */
void
SnmpAgentTester::testHandlersocket()
{
    MEventMgr mgr;
    // galaxy 207.154.57.30
    SnmpAgent rr(0xcf9a391e, 9998);
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

}   // SnmpAgentTester::testHandlersocket

#endif
