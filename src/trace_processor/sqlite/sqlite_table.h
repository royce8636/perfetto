/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_SQLITE_SQLITE_TABLE_H_
#define SRC_TRACE_PROCESSOR_SQLITE_SQLITE_TABLE_H_

#include <sqlite3.h>

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "perfetto/base/status.h"
#include "perfetto/ext/base/utils.h"
#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/sqlite/query_constraints.h"

namespace perfetto {
namespace trace_processor {

class TraceStorage;

// Abstract base class representing a SQLite virtual table. Implements the
// common bookeeping required across all tables and allows subclasses to
// implement a friendlier API than that required by SQLite.
class SqliteTable : public sqlite3_vtab {
 public:
  template <typename Context>
  using Factory =
      std::function<std::unique_ptr<SqliteTable>(sqlite3*, Context)>;

  // Custom opcodes used by subclasses of SqliteTable.
  // Stored here as we need a central repository of opcodes to prevent clashes
  // between different sub-classes.
  enum CustomFilterOpcode {
    kSourceGeqOpCode = SQLITE_INDEX_CONSTRAINT_FUNCTION + 1,
  };

  // Describes a column of this table.
  class Column {
   public:
    Column(size_t idx,
           std::string name,
           SqlValue::Type type,
           bool hidden = false);

    size_t index() const { return index_; }
    const std::string& name() const { return name_; }
    SqlValue::Type type() const { return type_; }

    bool hidden() const { return hidden_; }
    void set_hidden(bool hidden) { hidden_ = hidden; }

   private:
    size_t index_ = 0;
    std::string name_;
    SqlValue::Type type_ = SqlValue::Type::kNull;
    bool hidden_ = false;
  };

  // When set it logs all BestIndex and Filter actions on the console.
  static bool debug;

  // Public for unique_ptr destructor calls.
  virtual ~SqliteTable();

  // Abstract base class representing an SQLite Cursor. Presents a friendlier
  // API for subclasses to implement.
  class Cursor : public sqlite3_vtab_cursor {
   public:
    // Enum for the history of calls to Filter.
    enum class FilterHistory : uint32_t {
      // Indicates that constraint set passed is the different to the
      // previous Filter call.
      kDifferent = 0,

      // Indicates that the constraint set passed is the same as the previous
      // Filter call.
      // This can be useful for subclasses to perform optimizations on repeated
      // nested subqueries.
      kSame = 1,
    };

    explicit Cursor(SqliteTable* table);
    virtual ~Cursor();

    // Methods to be implemented by derived table classes.

    // Called to intialise the cursor with the constraints of the query.
    virtual base::Status Filter(const QueryConstraints& qc,
                                sqlite3_value**,
                                FilterHistory) = 0;

    // Called to forward the cursor to the next row in the table.
    virtual base::Status Next() = 0;

    // Called to check if the cursor has reached eof. Column will be called iff
    // this method returns true.
    virtual bool Eof() = 0;

    // Used to extract the value from the column at index |N|.
    virtual base::Status Column(sqlite3_context* context, int N) = 0;

   protected:
    Cursor(Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    Cursor(Cursor&&) noexcept = default;
    Cursor& operator=(Cursor&&) = default;

   private:
    friend class SqliteTable;

    int SetStatusAndReturn(base::Status status) {
      return table_->SetStatusAndReturn(status);
    }

    SqliteTable* table_ = nullptr;
  };

  // The schema of the table. Created by subclasses to allow the table class to
  // do filtering and inform SQLite about the CREATE table statement.
  class Schema {
   public:
    Schema();
    Schema(std::vector<Column>, std::vector<size_t> primary_keys);

    // This class is explicitly copiable.
    Schema(const Schema&);
    Schema& operator=(const Schema& t);

    std::string ToCreateTableStmt() const;

    const std::vector<Column>& columns() const { return columns_; }
    std::vector<Column>* mutable_columns() { return &columns_; }

    const std::vector<size_t> primary_keys() { return primary_keys_; }

   private:
    // The names and types of the columns of the table.
    std::vector<Column> columns_;

    // The primary keys of the table given by an offset into |columns|.
    std::vector<size_t> primary_keys_;
  };

 protected:
  // Populated by a BestIndex call to allow subclasses to tweak SQLite's
  // handling of sets of constraints.
  struct BestIndexInfo {
    // Contains bools which indicate whether SQLite should omit double checking
    // the constraint at that index.
    //
    // If there are no constraints, SQLite will be told it can omit checking for
    // the whole query.
    std::vector<bool> sqlite_omit_constraint;

    // Indicates that SQLite should not double check the result of the order by
    // clause.
    //
    // If there are no order by clauses, this value will be ignored and SQLite
    // will be told that it can omit double checking (i.e. this value will
    // implicitly be taken to be true).
    bool sqlite_omit_order_by = false;

    // Stores the estimated cost of this query.
    double estimated_cost = 0;

    // Estimated row count.
    int64_t estimated_rows = 0;
  };

  struct RegistrationFlags {
    // Specifies whether the table can also be written to.
    bool writable = false;

    enum TableType {
      // A table which automatically exists in the main schema and cannot be
      // created with CREATE VIRTUAL TABLE.
      // Note: the name value here matches the naming in the vtable docs of
      // SQLite.
      kEponymousOnly,

      // A table which automatically exists in the main schema and can also be
      // created with CREATE VIRTUAL TABLE.
      // Note: the name value here matches the naming in the vtable docs of
      // SQLite.
      kEponymous,

      // A table which must be explicitly created using a CREATE VIRTUAL TABLE
      // statement (i.e. does exist automatically) but does not have any
      // backing state beyond the arguments passed to it.
      kExplicitCreateStateless,
    };
    TableType type = TableType::kEponymousOnly;

    // Whether the table requires some number of hidden constraints to be passed
    // to be able to the queried (i.e. a SELECT * FROM table would not work).
    bool requires_hidden_constraints = false;
  };

  SqliteTable();

  // Called by derived classes to register themselves with the SQLite db.
  // Note: this function is inlined here because we use the TTable template to
  // devirtualise the function calls.
  template <typename TTable, typename Context = const TraceStorage*>
  static void Register(sqlite3* db,
                       Context ctx,
                       const std::string& module_name,
                       RegistrationFlags flags) {
    using TCursor = typename TTable::Cursor;

    struct TableDescriptor {
      SqliteTable::Factory<Context> factory;
      Context context;
      sqlite3_module module = {};
    };

    std::unique_ptr<TableDescriptor> desc(new TableDescriptor());
    desc->context = std::move(ctx);
    desc->factory = GetFactory<TTable, Context>();
    sqlite3_module* module = &desc->module;
    memset(module, 0, sizeof(*module));

    auto create_fn = [](sqlite3* xdb, void* arg, int argc,
                        const char* const* argv, sqlite3_vtab** tab,
                        char** pzErr) {
      auto* xdesc = static_cast<TableDescriptor*>(arg);
      auto table = xdesc->factory(xdb, std::move(xdesc->context));

      // SQLite guarantees that argv[0] will be the "module" name: this is the
      // same as |table_name| passed to the Register function.
      table->module_name_ = argv[0];

      // SQLite guarantees that argv[2] contains the name of the table: for
      // non-arg taking tables, this will be the same as |table_name| but for
      // arg-taking tables, this will be the table name as defined by the user
      // in the CREATE VIRTUAL TABLE call.
      table->name_ = argv[2];

      Schema schema;
      base::Status status = table->Init(argc, argv, &schema);
      if (!status.ok()) {
        *pzErr = sqlite3_mprintf("%s", status.c_message());
        return SQLITE_ERROR;
      }

      auto create_stmt = schema.ToCreateTableStmt();
      PERFETTO_DLOG("Create table statement: %s", create_stmt.c_str());

      int res = sqlite3_declare_vtab(xdb, create_stmt.c_str());
      if (res != SQLITE_OK)
        return res;

      // Freed in xDisconnect().
      table->schema_ = std::move(schema);
      *tab = table.release();

      return SQLITE_OK;
    };
    auto destroy_fn = [](sqlite3_vtab* t) {
      delete static_cast<TTable*>(t);
      return SQLITE_OK;
    };

    switch (flags.type) {
      case RegistrationFlags::kEponymousOnly:
        module->xCreate = nullptr;
        break;
      case RegistrationFlags::kEponymous:
        module->xCreate = create_fn;
        break;
      case RegistrationFlags::kExplicitCreateStateless:
        // TODO(lalitm): this is not accurate as we're basically creating an
        // eponymous table. Change this to be a different function once we can
        // do so easily.
        module->xCreate = create_fn;
        break;
    }
    module->xConnect = create_fn;
    module->xDisconnect = destroy_fn;
    module->xDestroy = destroy_fn;
    module->xOpen = [](sqlite3_vtab* t, sqlite3_vtab_cursor** c) {
      return static_cast<SqliteTable*>(t)->OpenInternal(c);
    };
    module->xClose = [](sqlite3_vtab_cursor* c) {
      delete static_cast<TCursor*>(c);
      return SQLITE_OK;
    };
    module->xBestIndex = [](sqlite3_vtab* t, sqlite3_index_info* i) {
      return static_cast<TTable*>(t)->BestIndexInternal(i);
    };
    module->xFilter = [](sqlite3_vtab_cursor* vc, int i, const char* s, int a,
                         sqlite3_value** v) {
      bool is_cached =
          static_cast<Cursor*>(vc)->table_->ReadConstraints(i, s, a);

      auto history = is_cached ? Cursor::FilterHistory::kSame
                               : Cursor::FilterHistory::kDifferent;
      auto* cursor = static_cast<TCursor*>(vc);
      return cursor->SetStatusAndReturn(cursor->Filter(
          static_cast<Cursor*>(vc)->table_->qc_cache_, v, history));
    };
    module->xNext = [](sqlite3_vtab_cursor* c) {
      auto* cursor = static_cast<TCursor*>(c);
      return cursor->SetStatusAndReturn(cursor->Next());
    };
    module->xEof = [](sqlite3_vtab_cursor* c) {
      return static_cast<int>(static_cast<TCursor*>(c)->Eof());
    };
    module->xColumn = [](sqlite3_vtab_cursor* c, sqlite3_context* a, int b) {
      auto* cursor = static_cast<TCursor*>(c);
      return cursor->SetStatusAndReturn(cursor->Column(a, b));
    };
    module->xRowid = [](sqlite3_vtab_cursor*, sqlite3_int64*) {
      return SQLITE_ERROR;
    };
    module->xFindFunction =
        [](sqlite3_vtab* t, int, const char* name,
           void (**fn)(sqlite3_context*, int, sqlite3_value**), void** args) {
          return static_cast<TTable*>(t)->FindFunction(name, fn, args);
        };

    if (flags.writable) {
      module->xUpdate = [](sqlite3_vtab* t, int a, sqlite3_value** v,
                           sqlite3_int64* r) {
        auto* table = static_cast<TTable*>(t);
        return table->SetStatusAndReturn(table->Update(a, v, r));
      };
    }

    int res = sqlite3_create_module_v2(
        db, module_name.c_str(), module, desc.release(),
        [](void* arg) { delete static_cast<TableDescriptor*>(arg); });
    PERFETTO_CHECK(res == SQLITE_OK);

    // Register virtual tables into an internal 'perfetto_tables' table. This is
    // used for iterating through all the tables during a database export. Note
    // that virtual tables which requires explicit CREATE statements or require
    // hidden constraints cannot be inserted.
    bool explicit_create =
        flags.type == RegistrationFlags::kExplicitCreateStateless;
    if (!explicit_create && !flags.requires_hidden_constraints) {
      char* insert_sql =
          sqlite3_mprintf("INSERT INTO perfetto_tables(name) VALUES('%q')",
                          module_name.c_str());
      char* error = nullptr;
      sqlite3_exec(db, insert_sql, nullptr, nullptr, &error);
      sqlite3_free(insert_sql);
      if (error) {
        PERFETTO_ELOG("Error registering table: %s", error);
        sqlite3_free(error);
      }
    }
  }

  // Methods to be implemented by derived table classes.
  virtual base::Status Init(int argc, const char* const* argv, Schema*) = 0;
  virtual std::unique_ptr<Cursor> CreateCursor() = 0;
  virtual int BestIndex(const QueryConstraints& qc, BestIndexInfo* info) = 0;

  // Optional metods to implement.
  using FindFunctionFn = void (*)(sqlite3_context*, int, sqlite3_value**);
  virtual base::Status ModifyConstraints(QueryConstraints* qc);
  virtual int FindFunction(const char* name, FindFunctionFn* fn, void** args);

  // At registration time, the function should also pass true for |read_write|.
  virtual base::Status Update(int, sqlite3_value**, sqlite3_int64*);

  const Schema& schema() const { return schema_; }
  const std::string& module_name() const { return module_name_; }
  const std::string& name() const { return name_; }

 private:
  template <typename TableType, typename Context>
  static Factory<Context> GetFactory() {
    return [](sqlite3* db, Context ctx) {
      return std::unique_ptr<SqliteTable>(new TableType(db, std::move(ctx)));
    };
  }

  bool ReadConstraints(int idxNum, const char* idxStr, int argc);

  // Overriden functions from sqlite3_vtab.
  int OpenInternal(sqlite3_vtab_cursor**);
  int BestIndexInternal(sqlite3_index_info*);

  int SetStatusAndReturn(base::Status status) {
    if (!status.ok()) {
      sqlite3_free(zErrMsg);
      zErrMsg = sqlite3_mprintf("%s", status.c_message());
      return SQLITE_ERROR;
    }
    return SQLITE_OK;
  }

  SqliteTable(const SqliteTable&) = delete;
  SqliteTable& operator=(const SqliteTable&) = delete;

  // This name of the table. For tables created using CREATE VIRTUAL TABLE, this
  // will be the name of the table specified by the query. For automatically
  // created tables, this will be the same as the module name passed to
  // RegisterTable.
  std::string name_;

  // The module name is the name passed to RegisterTable. This is differs from
  // the table name (|name_|) where the table was created using CREATE VIRTUAL
  // TABLE.
  std::string module_name_;

  Schema schema_;

  QueryConstraints qc_cache_;
  int qc_hash_ = 0;
  int best_index_num_ = 0;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_SQLITE_SQLITE_TABLE_H_
