/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_TRACE_PROCESSOR_SQLITE_DB_SQLITE_TABLE_H_
#define SRC_TRACE_PROCESSOR_SQLITE_DB_SQLITE_TABLE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "perfetto/base/compiler.h"
#include "perfetto/base/status.h"
#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/db/column/types.h"
#include "src/trace_processor/db/runtime_table.h"
#include "src/trace_processor/db/table.h"
#include "src/trace_processor/perfetto_sql/intrinsics/table_functions/static_table_function.h"
#include "src/trace_processor/sqlite/query_cache.h"
#include "src/trace_processor/sqlite/query_constraints.h"
#include "src/trace_processor/sqlite/sqlite_table.h"
#include "src/trace_processor/sqlite/sqlite_utils.h"

namespace perfetto {
namespace trace_processor {

struct DbSqliteTableContext {
  enum class Computation {
    // Table is statically defined.
    kStatic,

    // Table is defined as a function.
    kTableFunction,

    // Table is defined in runtime.
    kRuntime
  };
  DbSqliteTableContext(QueryCache* query_cache,
                       const Table* table,
                       Table::Schema schema);
  DbSqliteTableContext(QueryCache* query_cache,
                       std::function<RuntimeTable*(std::string)> get_table,
                       std::function<void(std::string)> erase_table);
  DbSqliteTableContext(QueryCache* query_cache,
                       std::unique_ptr<StaticTableFunction> table);

  QueryCache* cache;
  Computation computation;

  // Only valid when computation == TableComputation::kStatic.
  const Table* static_table = nullptr;
  Table::Schema static_schema;

  // Only valid when computation == TableComputation::kRuntime.
  // Those functions implement the interactions with
  // PerfettoSqlEngine::runtime_tables_ to get the |runtime_table_| and erase it
  // from the map when |this| is destroyed.
  std::function<RuntimeTable*(std::string)> get_runtime_table;
  std::function<void(std::string)> erase_runtime_table;

  // Only valid when computation == TableComputation::kTableFunction.
  std::unique_ptr<StaticTableFunction> static_table_function;
};

// Implements the SQLite table interface for db tables.
class DbSqliteTable final
    : public TypedSqliteTable<DbSqliteTable,
                              std::unique_ptr<DbSqliteTableContext>> {
 public:
  using Context = DbSqliteTableContext;
  using TableComputation = Context::Computation;

  class Cursor final : public SqliteTable::BaseCursor {
   public:
    Cursor(DbSqliteTable*, QueryCache*);
    ~Cursor() final;

    Cursor(Cursor&&) noexcept = default;
    Cursor& operator=(Cursor&&) = default;

    // Implementation of SqliteTable::Cursor.
    base::Status Filter(const QueryConstraints& qc,
                        sqlite3_value** argv,
                        FilterHistory);

    PERFETTO_ALWAYS_INLINE void Next() {
      if (mode_ == Mode::kSingleRow) {
        eof_ = true;
      } else {
        eof_ = !++*iterator_;
      }
    }

    PERFETTO_ALWAYS_INLINE bool Eof() const { return eof_; }

    PERFETTO_ALWAYS_INLINE void Column(sqlite3_context* ctx,
                                       int raw_col) const {
      auto column = static_cast<uint32_t>(raw_col);
      SqlValue value = mode_ == Mode::kSingleRow
                           ? SourceTable()->columns()[column].Get(*single_row_)
                           : iterator_->Get(column);
      // We can say kSqliteStatic for strings because all strings are expected
      // to come from the string pool. Thus they will be valid for the lifetime
      // of trace processor. Similarily, for bytes, we can also use
      // kSqliteStatic because for our iterator will hold onto the pointer as
      // long as we don't call Next(). However, that only happens when Next() is
      // called on the Cursor itself, at which point SQLite no longer cares
      // about the bytes pointer.
      sqlite_utils::ReportSqlValue(ctx, value, sqlite_utils::kSqliteStatic,
                                   sqlite_utils::kSqliteStatic);
    }

   private:
    enum class Mode {
      kSingleRow,
      kTable,
    };

    // Tries to create a sorted table to cache in |sorted_cache_table_| if the
    // constraint set matches the requirements.
    void TryCacheCreateSortedTable(const QueryConstraints&, FilterHistory);

    const Table* SourceTable() const {
      // Try and use the sorted cache table (if it exists) to speed up the
      // sorting. Otherwise, just use the original table.
      return sorted_cache_table_ ? &*sorted_cache_table_ : upstream_table_;
    }

    static base::Status ExtractTableFunctionArguments(
        const Table::Schema&,
        std::vector<Constraint>& constraints,
        std::vector<SqlValue>& function_arguments);

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    DbSqliteTable* db_sqlite_table_ = nullptr;
    QueryCache* cache_ = nullptr;

    const Table* upstream_table_ = nullptr;

    // Only valid for |db_sqlite_table_->computation_| ==
    // TableComputation::kDynamic.
    std::unique_ptr<Table> dynamic_table_;

    // Only valid for Mode::kSingleRow.
    std::optional<uint32_t> single_row_;

    // Only valid for Mode::kTable.
    std::optional<Table::Iterator> iterator_;

    bool eof_ = true;

    // Stores a sorted version of |db_table_| sorted on a repeated equals
    // constraint. This allows speeding up repeated subqueries in joins
    // significantly.
    std::shared_ptr<Table> sorted_cache_table_;

    // Stores the count of repeated equality queries to decide whether it is
    // wortwhile to sort |db_table_| to create |sorted_cache_table_|.
    uint32_t repeated_cache_count_ = 0;

    Mode mode_ = Mode::kSingleRow;

    std::vector<Constraint> constraints_;
    std::vector<Order> orders_;
    std::vector<SqlValue> table_function_arguments_;
  };
  struct QueryCost {
    double cost;
    uint32_t rows;
  };

  DbSqliteTable(sqlite3*, Context* context);
  virtual ~DbSqliteTable() final;

  // Table implementation.
  base::Status Init(int, const char* const*, SqliteTable::Schema*) final;
  std::unique_ptr<SqliteTable::BaseCursor> CreateCursor() final;
  base::Status ModifyConstraints(QueryConstraints*) final;
  int BestIndex(const QueryConstraints&, BestIndexInfo*) final;

  // These static functions are useful to allow other callers to make use
  // of them.
  static SqliteTable::Schema ComputeSchema(const Table::Schema&,
                                           const char* table_name);
  static void ModifyConstraints(const Table::Schema&, QueryConstraints*);
  static void BestIndex(const Table::Schema&,
                        uint32_t row_count,
                        const QueryConstraints&,
                        BestIndexInfo*);

  // static for testing.
  static QueryCost EstimateCost(const Table::Schema&,
                                uint32_t row_count,
                                const QueryConstraints& qc);

 private:
  static base::Status ValidateTableFunctionArguments(const Table::Schema&,
                                                     const QueryConstraints&);

  Context* context_ = nullptr;

  // Only valid after Init has completed.
  Table::Schema schema_;
  RuntimeTable* runtime_table_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_SQLITE_DB_SQLITE_TABLE_H_
