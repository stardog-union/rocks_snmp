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

#include "meventmgr.h"
#include "rocksdb/statistics.h"
#include "snmp_agent.h"
#include "val_integer64.h"
#include "val_string.h"

class SnmpValTicker : public SnmpValCounter64 {
protected:
  rocksdb::Tickers m_Ticker;
  const std::shared_ptr<rocksdb::Statistics> m_Stats;

public:
  SnmpValTicker() = delete;
  SnmpValTicker(unsigned ID, rocksdb::Tickers Ticker,
                const std::shared_ptr<rocksdb::Statistics> Stats)
      : SnmpValCounter64(ID), m_Ticker(Ticker), m_Stats(Stats) {}

  virtual ~SnmpValTicker(){};

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {
    m_Unsigned64 = m_Stats->getTickerCount(m_Ticker);

    SnmpValCounter64::AppendToIovec(IoArray);
  }
}; // class SnmpValTicker

class StatsTable {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:
  MEventMgrPtr m_Mgr;
  SnmpAgentPtr m_Agent; //!< snmp manager instance

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  StatsTable();

  virtual ~StatsTable();

  bool AddTable(const std::shared_ptr<rocksdb::Statistics> &stats,
                unsigned TableId, const std::string &name);

  /// debug
  void Dump();

protected:
private:
  StatsTable(const StatsTable &);            //!< disabled:  copy operator
  StatsTable &operator=(const StatsTable &); //!< disabled:  assignment operator

}; // StatsTable

#endif // ifndef STATS_TABLE_H
