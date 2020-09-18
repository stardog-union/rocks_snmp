/**
 * @file stats_test.cpp
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2020
 *
 * @brief Executable for testing snmp side channel to rocksdb statistics
 */

#include <unistd.h>

#include "stats_table.h"

#include "meventmgr.h"
#include "snmp_agent.h"

#include "rocksdb/statistics.h"

MEventMgr gEventMgr;

/**
 *  Enterprise:  1.3.6.1.4.1
 *Agent prefix:  38693  matthewv.com enterprise
 *                     .1 generic
 *                       .1 /proc/diskstats
 *                     .2 hellodirect
 *                       .1 log monitors
 *                       .2 tomcat5 gc log
 *                     .3 stm
 *                       .1 log monitors
 *                       .2 haproxy monitors
 *                       .3 memcached
 *                     .4 riak
 *                       .1 diskstats
 *                     .5 rocksdb
 *                       .1 ticker statistics
 *                         .x instance id
 */

static unsigned sAgentPrefix[]={1,3,6,1,4,1,38693,5};
static SnmpAgent::SnmpAgentId sAgentId={sAgentPrefix, sizeof(sAgentPrefix)/sizeof(sAgentPrefix[0]),
                               "RiakMonitor"};

//static const unsigned sDiskTable[]={1,1};
//static SnmpOid sDiskTableId={sDiskTable, sizeof(sDiskTable)/sizeof(sDiskTable[0])};

/**
 * Start a statistics event loop that snmpd can reach
 * @date created 09/16/20
 * @author matthewv
 */
int
main(int argc, char ** argv)
{
  int ret_val;
    const char * progname="stats_test.e";

    ret_val=0;

    gLogging.Open(NULL, progname, LOG_USER, true);
    gLogLevel=LOG_DEBUG;

    Logging(LOG_ERR, "%s: %s starting", __func__, *argv);

    // command
    if (1==argc)
    {
        bool ret_flag;

        // get events running before adding logs
        gEventMgr.StartThreaded();

        SnmpAgentPtr sa = std::make_shared<SnmpAgent>(sAgentId, 0x7f000001, 705);
        //        MEventPtr mo_sa = std::static_pointer_cast<MEventObj>(sa);
        MEventPtr mo_sa = sa->GetMEventPtr();

        // StatsTable adds its components to SnmpAgent ... and gets refreshed how?
        std::shared_ptr<rocksdb::Statistics> stats = rocksdb::CreateDBStatistics();
        StatsTable st(sa, stats, 1, "test_stats");

        gEventMgr.AddEvent(mo_sa);

        //        LevelMonitor db(sa, 4);

        // sit and wait ... no clean ending at this time
        ret_flag=gEventMgr.ThreadWait();
        ret_val=(ret_flag ? 0 : 1);
    }
    else
    {
        Logging(LOG_ERR, "%s: Command error:  command",
                __func__);

        ret_val=1;
    }   // else

    Logging(LOG_ERR, "%s: %s exiting [ret_val=%d]",
            __func__, *argv, ret_val);

    return(ret_val);

}   // main


/**
 * Signal handler to stop global event manager
 * @date Created 05/21/12
 * @author matthewv
 */
void
SendStop(
    int SigNum)
{
    Logging(LOG_ERR, "%s:  received signal %d, stopping",
            __func__, SigNum);

    gEventMgr.Stop(true);

    return;

}   // SendStop
