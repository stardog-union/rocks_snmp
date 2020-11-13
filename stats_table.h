/**
 * @file stats_table.h
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2012-2014
 *
 * @brief
 */

#ifndef STATS_TABLE_H
#define STATS_TABLE_H

#include "meventmgr.h"

#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"
#include "snmp_agent.h"
#include "val_integer64.h"
#include "val_string.h"

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
  StatsTable() = delete;
  StatsTable(bool StartWorker = true);

  virtual ~StatsTable();

  bool AddTable(const std::shared_ptr<rocksdb::Statistics> &stats,
                unsigned TableId, const std::string &name);

  bool AddTable(const std::shared_ptr<rocksdb::Cache> &cache,
                unsigned TableId, const std::string &name);

  bool AddTable(rocksdb::DB * dbase,
                unsigned TableId, const std::string &name);

  /// debug
  void Dump();

protected:
  void UpdateTableNameList(unsigned TableId, const std::string &name);

private:
  StatsTable(const StatsTable &);            //!< disabled:  copy operator
  StatsTable &operator=(const StatsTable &); //!< disabled:  assignment operator

}; // StatsTable

#endif // ifndef STATS_TABLE_H
