/**
 * @file stats_table_impl.h
 * @author matthewv
 * @date Jan 8, 2023
 * @date Copyright 2012-2014
 *
 * @brief
 */

#ifndef STATS_TABLE_IMPL_H
#define STATS_TABLE_IMPL__H

#include "libmevent/meventmgr.h"

#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"
#include "snmpagent/snmp_agent.h"

class StatsTableImpl : public StatsTable {
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
  StatsTableImpl() = delete;
  StatsTableImpl(bool StartWorker = true);

  virtual ~StatsTableImpl();

  bool AddTable(const std::shared_ptr<rocksdb::Statistics> &stats,
                unsigned TableId, const std::string &name) override;

  bool AddTable(const std::shared_ptr<rocksdb::Cache> &cache,
                unsigned TableId, const std::string &name) override;

  bool AddTable(rocksdb::DB * dbase,
                unsigned TableId, const std::string &name) override;

protected:
  void UpdateTableNameList(unsigned TableId, const std::string &name);

private:
  StatsTableImpl(const StatsTableImpl &);            //!< disabled:  copy operator
  StatsTableImpl &operator=(const StatsTableImpl &); //!< disabled:  assignment operator

}; // StatsTable

#endif // ifndef STATS_TABLE_IMPL_H
