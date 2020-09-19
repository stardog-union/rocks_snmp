/**
 * @file stats_table.h
 * @author matthewv
 * @date May 21, 2012
 * @date Copyright 2012-2014
 *
 * @brief Classes to hold leveldb to snmp conversion objects
 */

#ifndef STATS_TABLE_H
#define STATS_TABLE_H

#include "val_integer64.h"
#include "val_string.h"
#include "snmp_agent.h"
#include "rocksdb/statistics.h"


class StatsTable
{
    /****************************************************************
    *  Member objects
    ****************************************************************/
public:

protected:
  SnmpAgentPtr m_Agent;          //!< snmp manager instance
  OidVector_t m_TablePrefix;
  const std::shared_ptr<rocksdb::Statistics> m_Stats;
  const std::string m_TableName;

    std::vector<rocksdb::Tickers> m_Tickers;
    std::vector<uint64_t> m_Values;

private:

    /****************************************************************
    *  Member functions
    ****************************************************************/
public:

    StatsTable(SnmpAgentPtr & Agent, const std::shared_ptr<rocksdb::Statistics> & stats, unsigned TableId, const std::string & name);

    virtual ~StatsTable() {};

    /// debug
    void Dump();

protected:


private:
    StatsTable();                             //!< disabled:  default constructor
    StatsTable(const StatsTable & );             //!< disabled:  copy operator
    StatsTable & operator=(const StatsTable &);  //!< disabled:  assignment operator

};  // StatsTable


#endif // ifndef STATS_TABLE_H
