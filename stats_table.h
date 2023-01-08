/**
 * @file stats_table.h
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2012-present
 *
 * @brief
 */

#ifndef STATS_TABLE_H
#define STATS_TABLE_H

#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"

class StatsTable {
  /****************************************************************
   *  Member objects
   ****************************************************************/
public:
protected:

private:
  /****************************************************************
   *  Member functions
   ****************************************************************/
public:
  StatsTable() {};

  virtual ~StatsTable() {};

  static StatsTable* NewStatsTable(bool StartWorker = true);

  virtual bool AddTable(const std::shared_ptr<rocksdb::Statistics> &stats,
                        unsigned TableId, const std::string &name) = 0;

  virtual bool AddTable(const std::shared_ptr<rocksdb::Cache> &cache,
                        unsigned TableId, const std::string &name) = 0;

  virtual bool AddTable(rocksdb::DB * dbase,
                        unsigned TableId, const std::string &name) = 0;

private:
  StatsTable(const StatsTable &);            //!< disabled:  copy operator
  StatsTable &operator=(const StatsTable &); //!< disabled:  assignment operator

}; // StatsTable

#endif // ifndef STATS_TABLE_H
