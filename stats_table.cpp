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
 * Initialize the data members.
 * @date Created 05/21/12
 * @author matthewv
 */
StatsTable::StatsTable(
    SnmpAgentPtr & Agent,
    const std::shared_ptr<rocksdb::Statistics> & stats,
    unsigned TableId,
    const std::string & TableName)
    : m_Agent(Agent), m_Stats(stats), m_TableName(TableName)

{
  int idx;
  OidVector_t row_oid, null_oid;
  row_oid.push_back(0);
  m_TablePrefix.push_back(TableId);

  //
  // Retrieve the statistics map once immediately to get the
  //  map and its std::string members allocated and initialized
  //

  m_Tickers.resize(rocksdb::TickersNameMap.size());
  m_Values.resize(rocksdb::TickersNameMap.size(), 0);

  idx = 0;
  for (auto ticker : rocksdb::TickersNameMap) {
    std::shared_ptr<SnmpValCounterPtr64> new_counter;
    SnmpValStringPtr new_string;

    SnmpValUnsigned64Ptr new_item;
    SnmpValInfPtr shared;

    m_Tickers[idx] = ticker.first;

    row_oid[0] = idx + 1;

    new_counter=std::make_shared<SnmpValCounterPtr64>(1, &m_Values[idx]);
    new_counter->InsertTablePrefix(Agent->GetOidPrefix(), m_TablePrefix, null_oid, row_oid);
    shared = new_counter->GetSnmpValInfPtr();
    Agent->AddVariable(shared);

    std::string local_name = m_TableName;
    local_name.append(".");
    local_name.append(ticker.second);
    new_string = std::make_shared<SnmpValString>(2);
    new_string->InsertTablePrefix(Agent->GetOidPrefix(), m_TablePrefix, null_oid, row_oid);
    new_string->assign(local_name.c_str());
    shared = new_string->GetSnmpValInfPtr();
    Agent->AddVariable(shared);

    ++idx;
  }

    return;

}   // StatsTable::StatsTable
