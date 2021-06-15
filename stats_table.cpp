/**
 * @file stats_table.cpp
 * @author matthewv
 * @date Sept 16, 2020
 * @date Copyright 2012-2014
 *
 * @brief
 */

#include "stats_table.h"
#include "snmpagent/val_integer64.h"

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
static unsigned sAgentPrefix[] = {1, 38693, 5};
static SnmpAgent::SnmpAgentId sAgentId = {
    sAgentPrefix, sizeof(sAgentPrefix) / sizeof(sAgentPrefix[0]),
    "RocksMonitor"};

/**
 * Initialize the data members.
 * @date Created 05/21/12
 * @author matthewv
 */
StatsTable::StatsTable(bool StartWorker) {

  // everything is a "make_shared" object in libmevent & snmpagent world
  m_Mgr = std::make_shared<MEventMgr>();

  if (StartWorker) {
    m_Mgr->StartThreaded();
  } // if

  m_Agent = std::make_shared<SnmpAgent>(sAgentId, 0x7f000001, 705);
  MEventPtr mo_sa = m_Agent->GetMEventPtr();

  m_Mgr->AddEvent(mo_sa);
}

StatsTable::~StatsTable() {
  m_Mgr->Stop();
  m_Mgr->ThreadWait();
} // StatsTable::~StatsTable

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

bool StatsTable::AddTable(const std::shared_ptr<rocksdb::Statistics> &stats,
                          unsigned TableId, const std::string &TableName) {

  SnmpValInfPtr shared;
  SnmpValStringPtr new_string;
  OidVector_t table_prefix = {TableId};
  int idx;
  OidVector_t row_oid, null_oid;

  UpdateTableNameList(TableId, TableName);

  //
  // Retrieve the statistics map once immediately to get the
  //  map and its std::string members allocated and initialized
  //
  row_oid.push_back(0);

  idx = 0;
  for (auto ticker : rocksdb::TickersNameMap) {
    std::shared_ptr<SnmpValTicker> new_counter;

    row_oid[0] = ticker.first;

    new_counter = std::make_shared<SnmpValTicker>(1, ticker.first, stats);
    new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                   null_oid, row_oid);
    shared = new_counter->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);

    new_string = std::make_shared<SnmpValString>(2);
    new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                  null_oid, row_oid);
    new_string->assign(ticker.second.c_str());
    shared = new_string->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);

    ++idx;
  }

  return true;

} // StatsTable::AddTable (statistics)


typedef size_t (rocksdb::Cache::*CacheGetFunction)(void) const;

class CacheValCounter64 : public SnmpValUnsigned64 {
public:


  CacheValCounter64() = delete;

  CacheValCounter64(const SnmpOid &Oid, const std::shared_ptr<rocksdb::Cache> & cache,
                    CacheGetFunction Func)
    : SnmpValUnsigned64(Oid, gVarCounter64), cache_weak(cache), value_func(Func) {};

  CacheValCounter64(const OidVector_t &Oid, const std::shared_ptr<rocksdb::Cache> & cache,
                    CacheGetFunction Func)
    : SnmpValUnsigned64(Oid, gVarCounter64), cache_weak(cache), value_func(Func) {};

  CacheValCounter64(unsigned ID, const std::shared_ptr<rocksdb::Cache> & cache,
                    CacheGetFunction Func)
    : SnmpValUnsigned64(ID, gVarCounter64), cache_weak(cache), value_func(Func) {};

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {
    std::shared_ptr<rocksdb::Cache> strong_ptr;

    strong_ptr = cache_weak.lock();

    if (strong_ptr) {
      m_Unsigned64 = (uint64_t)((*strong_ptr).*value_func)();
    } else {
      m_Unsigned64 = 0;
    }

    SnmpValUnsigned64::AppendToIovec(IoArray);
  };

protected:
  const std::weak_ptr<rocksdb::Cache> cache_weak;
  CacheGetFunction value_func;

};  // CacheValCounter64


bool StatsTable::AddTable(const std::shared_ptr<rocksdb::Cache> &cache,
                          unsigned TableId, const std::string &TableName) {

  SnmpValInfPtr shared;
  SnmpValStringPtr new_string;
  OidVector_t table_prefix = {TableId};
  OidVector_t row_oid, null_oid;
  std::shared_ptr<CacheValCounter64> new_counter;

  UpdateTableNameList(TableId, TableName);

  //
  // Build the string elements and value
  //  map and its std::string members allocated and initialized
  //
  row_oid.push_back(0);


  // GetCapacity
  row_oid[0] = 0;
  new_counter = std::make_shared<CacheValCounter64>(1, cache, &rocksdb::Cache::GetCapacity);
  new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                 null_oid, row_oid);
  shared = new_counter->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  new_string = std::make_shared<SnmpValString>(2);
  new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                null_oid, row_oid);
  new_string->assign("rocksdb.cache.get.capacity");
  shared = new_string->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  // GetUsage (two versions of GetUsage.  static_cast to say want void param version)
  row_oid[0] = 1;
  CacheGetFunction xx = static_cast<size_t(rocksdb::Cache::*)(void) const>(&rocksdb::Cache::GetUsage);
  new_counter = std::make_shared<CacheValCounter64>(1, cache, xx);

  new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                 null_oid, row_oid);
  shared = new_counter->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  new_string = std::make_shared<SnmpValString>(2);
  new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                null_oid, row_oid);
  new_string->assign("rocksdb.cache.get.usage");
  shared = new_string->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  // GetPinnedUsage
  row_oid[0] = 2;
  new_counter = std::make_shared<CacheValCounter64>(1, cache, &rocksdb::Cache::GetPinnedUsage);
  new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                 null_oid, row_oid);
  shared = new_counter->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  new_string = std::make_shared<SnmpValString>(2);
  new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                null_oid, row_oid);
  new_string->assign("rocksdb.cache.get.pinned.usage");
  shared = new_string->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

  return true;

} // StatsTable::AddTable (cache)


void StatsTable::UpdateTableNameList(unsigned TableId, const std::string &TableName) {
  SnmpValInfPtr shared;
  SnmpValStringPtr new_string;
  OidVector_t table_prefix = {TableId};
  OidVector_t row_oid, null_oid;

  //
  // Put a table name in snmp tree
  //
  row_oid.push_back(0);
  row_oid.push_back(TableId);
  new_string = std::make_shared<SnmpValString>(0);
  new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), null_oid, null_oid,
                                row_oid);
  new_string->assign(TableName.c_str());
  shared = new_string->GetSnmpValInfPtr();
  m_Agent->AddVariable(shared);

} // StatsTable::UpdateTableNameList


class RocksValCounter64 : public SnmpValUnsigned64 {
public:


  RocksValCounter64() = delete;

  RocksValCounter64(const SnmpOid &Oid, rocksdb::DB * DBptr, const char * Property)
    : SnmpValUnsigned64(Oid, gVarCounter64), dbase(DBptr), property(Property) {}

  RocksValCounter64(const OidVector_t &Oid, rocksdb::DB * DBptr, const char * Property)
    : SnmpValUnsigned64(Oid, gVarCounter64), dbase(DBptr), property(Property) {}

  RocksValCounter64(unsigned ID, rocksdb::DB * DBptr, const char * Property)
    : SnmpValUnsigned64(ID, gVarCounter64), dbase(DBptr), property(Property) {}

  void AppendToIovec(std::vector<struct iovec> &IoArray) override {

    if (nullptr != dbase) {
      bool flag;
      flag = dbase->GetAggregatedIntProperty(property, &m_Unsigned64);
      if (!flag) {
        m_Unsigned64 = 0;
      }
    } else {
      m_Unsigned64 = 0;
    }

    SnmpValUnsigned64::AppendToIovec(IoArray);
  };

protected:
  rocksdb::DB * dbase = nullptr;
  std::string property;

};  // RocksValCounter64


bool StatsTable::AddTable(rocksdb::DB * DBase,
                          unsigned TableId, const std::string &TableName) {

  SnmpValInfPtr shared;
  SnmpValStringPtr new_string;
  OidVector_t table_prefix = {TableId};
  OidVector_t row_oid, null_oid;
  std::shared_ptr<RocksValCounter64> new_counter;

  UpdateTableNameList(TableId, TableName);

  //
  // Build the string elements and value
  //  map and its std::string members allocated and initialized
  //
  row_oid.push_back(0);

  std::map<uint64_t, const char *> int_properties
  {
    // partial list of variables available per rocksdb/db.h
    {0, "rocksdb.estimate-table-readers-mem"},
    {1, "rocksdb.background-errors"},
    {2, "rocksdb.cur-size-active-mem-table"},
    {3, "rocksdb.cur-size-all-mem-tables"},
    {4, "rocksdb.size-all-mem-tables"},
    {5, "rocksdb.num-snapshots"},
    {6, "rocksdb.estimate-live-data-size"},
    {7, "rocksdb.total-sst-files-size"},
    {8, "rocksdb.live-sst-files-size"},
    {9, "rocksdb.block-cache-capacity"},
    {10,"rocksdb.block-cache-usage"},
    {11,"rocksdb.block-cache-pinned-usage"},
    {12,"rocksdb.table-cache-capacity"},
    {13,"rocksdb.table-cache-usage"},
    {14,"rocksdb.is-write-stopped"}
  };

  for (auto item : int_properties) {
    row_oid[0] = item.first;
    new_counter = std::make_shared<RocksValCounter64>(1, DBase, item.second);
    new_counter->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                   null_oid, row_oid);
    shared = new_counter->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);

    new_string = std::make_shared<SnmpValString>(2);
    new_string->InsertTablePrefix(m_Agent->GetOidPrefix(), table_prefix,
                                  null_oid, row_oid);
    new_string->assign(item.second);
    shared = new_string->GetSnmpValInfPtr();
    m_Agent->AddVariable(shared);
  } // for

  return true;

} // StatsTable::AddTable (db)
