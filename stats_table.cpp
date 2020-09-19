/**
 * @file stats_table.cpp
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2012-2014
 *
 * @brief
 */


#include "stats_table.h"

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

// .1.3.6.1.4 is implied in communications
static unsigned sAgentPrefix[]={1,38693,5};
static SnmpAgent::SnmpAgentId sAgentId={sAgentPrefix, sizeof(sAgentPrefix)/sizeof(sAgentPrefix[0]),
                               "RocksMonitor"};


/**
 * Initialize the data members.
 * @date Created 05/21/12
 * @author matthewv
 */
StatsTable::StatsTable() {

  m_Mgr.StartThreaded();

  m_Agent = std::make_shared<SnmpAgent>(sAgentId, 0x7f000001, 705);
  MEventPtr mo_sa = m_Agent->GetMEventPtr();

  m_Mgr.AddEvent(mo_sa);
}

StatsTable::~StatsTable() {
  m_Mgr.Stop();
  m_Mgr.ThreadWait();
} // StatsTable::~StatsTable


bool StatsTable::AddTable(
    const std::shared_ptr<rocksdb::Statistics> & stats,
    unsigned TableId,
    const std::string & TableName) {

  SnmpValInfPtr shared;
  SnmpValStringPtr new_string;
  OidVector_t table_prefix = {TableId};
  int idx;
  OidVector_t row_oid, null_oid;
  row_oid.push_back(0);

  //
  // Put a table name in snmp tree
  //
  new_string = std::make_shared<SnmpValString>(0);
  new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix, null_oid, null_oid);
  new_string->assign(TableName.c_str());
  shared = new_string->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  //
  // Retrieve the statistics map once immediately to get the
  //  map and its std::string members allocated and initialized
  //

  idx = 0;
  for (auto ticker : rocksdb::TickersNameMap) {
    std::shared_ptr<SnmpValTicker> new_counter;

    row_oid[0] = idx + 1;

    new_counter=std::make_shared<SnmpValTicker>(1, ticker.first, stats);
    new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix, null_oid, row_oid);
    shared = new_counter->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);

    new_string = std::make_shared<SnmpValString>(2);
    new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix, null_oid, row_oid);
    new_string->assign(ticker.second.c_str());
    shared = new_string->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);

    ++idx;
  }

  return true;

}   // StatsTable::StatsTable
