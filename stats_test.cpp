/**
 * @file stats_test.cpp
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2020
 *
 * @brief Executable for testing snmp side channel to rocksdb statistics
 */

#include <unistd.h>
#include <stdio.h>

#include "stats_table.h"
#include "rocksdb/statistics.h"

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

        StatsTable table;


        // StatsTable adds its components to SnmpAgent ... and gets refreshed how?
        std::shared_ptr<rocksdb::Statistics> stats = rocksdb::CreateDBStatistics();
        ret_flag = table.AddTable(stats, 1, "test_stats");

        printf("Type any key to continue ...");
        getchar();

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
