// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Benchmark for the SQLite VTable interface.
// This benchmark measures the speed-of-light obtainable through a SQLite
// virtual table. The code here implements an ideal virtual table which fetches
// data in blocks and serves the xNext/xCol requests by just advancing a pointer
// in a buffer. This is to have a fair estimate w.r.t. cache-misses and pointer
// chasing of what an upper-bound can be for a virtual table implementation.

#include <array>
#include <random>

#include <benchmark/benchmark.h>
#include <sqlite3.h>

#include "perfetto/base/compiler.h"
#include "src/trace_processor/sqlite/scoped_db.h"

namespace {

using benchmark::Counter;
using perfetto::trace_processor::ScopedDb;
using perfetto::trace_processor::ScopedStmt;

bool IsBenchmarkFunctionalOnly() {
  return getenv("BENCHMARK_FUNCTIONAL_TEST_ONLY") != nullptr;
}

void BenchmarkArgs(benchmark::internal::Benchmark* b) {
  if (IsBenchmarkFunctionalOnly()) {
    b->Ranges({{1024, 1024}, {1, 1}});
  } else {
    b->RangeMultiplier(2)->Ranges({{1024, 1024 * 128}, {1, 8}});
  }
}

class BenchmarkCursor : public sqlite3_vtab_cursor {
 public:
  explicit BenchmarkCursor(size_t num_cols, size_t batch_size)
      : num_cols_(num_cols), batch_size_(batch_size), rnd_engine_(kRandomSeed) {
    column_buffer_.resize(num_cols);
    for (auto& col : column_buffer_)
      col.resize(batch_size);
    RandomFill();
  }
  PERFETTO_NO_INLINE int Next();
  PERFETTO_NO_INLINE int Column(sqlite3_context* ctx, int);
  PERFETTO_NO_INLINE int Eof();
  void RandomFill();

 private:
  size_t num_cols_;
  size_t batch_size_;
  static constexpr uint32_t kRandomSeed = 476;

  uint32_t row_ = 0;
  using ColBatch = std::vector<int64_t>;
  std::vector<ColBatch> column_buffer_;

  std::minstd_rand0 rnd_engine_;
};

void BenchmarkCursor::RandomFill() {
  for (size_t col = 0; col < num_cols_; col++) {
    for (size_t row = 0; row < batch_size_; row++) {
      column_buffer_[col][row] = static_cast<int64_t>(rnd_engine_());
    }
  }
}

int BenchmarkCursor::Next() {
  row_ = (row_ + 1) % batch_size_;
  if (row_ == 0)
    RandomFill();
  return SQLITE_OK;
}

int BenchmarkCursor::Eof() {
  return false;
}

int BenchmarkCursor::Column(sqlite3_context* ctx, int col_int) {
  const auto col = static_cast<size_t>(col_int);
  PERFETTO_CHECK(col < column_buffer_.size());
  sqlite3_result_int64(ctx, column_buffer_[col][row_]);
  return SQLITE_OK;
}

static void BM_SqliteStepAndResult(benchmark::State& state) {
  struct BenchmarkVtab : public sqlite3_vtab {
    size_t num_cols;
    size_t batch_size;
  };

  sqlite3_initialize();

  // Make sure the module outlives the ScopedDb. SQLite calls xDisconnect in
  // the database close function and so this struct needs to be available then.
  sqlite3_module module{};

  ScopedDb db;
  sqlite3* raw_db = nullptr;
  PERFETTO_CHECK(sqlite3_open(":memory:", &raw_db) == SQLITE_OK);
  db.reset(raw_db);

  auto create_fn = [](sqlite3* xdb, void* aux, int, const char* const*,
                      sqlite3_vtab** tab, char**) {
    benchmark::State& _state = *static_cast<benchmark::State*>(aux);
    size_t num_cols = static_cast<size_t>(_state.range(1));
    std::string sql = "CREATE TABLE x(";
    for (size_t col = 0; col < num_cols; col++)
      sql += "c" + std::to_string(col) + " BIG INT,";
    sql[sql.size() - 1] = ')';
    int res = sqlite3_declare_vtab(xdb, sql.c_str());
    PERFETTO_CHECK(res == SQLITE_OK);
    auto* vtab = new BenchmarkVtab();
    vtab->num_cols = num_cols;
    vtab->batch_size = num_cols;
    *tab = vtab;
    return SQLITE_OK;
  };

  auto destroy_fn = [](sqlite3_vtab* t) {
    delete static_cast<BenchmarkVtab*>(t);
    return SQLITE_OK;
  };

  module.xCreate = create_fn;
  module.xConnect = create_fn;
  module.xDisconnect = destroy_fn;
  module.xDestroy = destroy_fn;

  module.xOpen = [](sqlite3_vtab* tab, sqlite3_vtab_cursor** c) {
    auto* vtab = static_cast<BenchmarkVtab*>(tab);
    *c = new BenchmarkCursor(vtab->num_cols, vtab->batch_size);
    return SQLITE_OK;
  };
  module.xBestIndex = [](sqlite3_vtab*, sqlite3_index_info* idx) {
    idx->orderByConsumed = true;
    for (int i = 0; i < idx->nConstraint; ++i) {
      idx->aConstraintUsage[i].omit = true;
    }
    return SQLITE_OK;
  };
  module.xClose = [](sqlite3_vtab_cursor* c) {
    delete static_cast<BenchmarkCursor*>(c);
    return SQLITE_OK;
  };
  module.xFilter = [](sqlite3_vtab_cursor*, int, const char*, int,
                      sqlite3_value**) { return SQLITE_OK; };
  module.xNext = [](sqlite3_vtab_cursor* c) {
    return static_cast<BenchmarkCursor*>(c)->Next();
  };
  module.xEof = [](sqlite3_vtab_cursor* c) {
    return static_cast<BenchmarkCursor*>(c)->Eof();
  };
  module.xColumn = [](sqlite3_vtab_cursor* c, sqlite3_context* a, int b) {
    return static_cast<BenchmarkCursor*>(c)->Column(a, b);
  };
  int res =
      sqlite3_create_module_v2(*db, "benchmark", &module, &state, nullptr);
  PERFETTO_CHECK(res == SQLITE_OK);

  ScopedStmt stmt;
  sqlite3_stmt* raw_stmt;
  std::string sql = "SELECT * from benchmark";
  int err = sqlite3_prepare_v2(*db, sql.c_str(), static_cast<int>(sql.size()),
                               &raw_stmt, nullptr);
  PERFETTO_CHECK(err == SQLITE_OK);
  stmt.reset(raw_stmt);
  size_t batch_size = static_cast<size_t>(state.range(0));
  size_t num_cols = static_cast<size_t>(state.range(1));

  int64_t value = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < batch_size; i++) {
      PERFETTO_CHECK(sqlite3_step(*stmt) == SQLITE_ROW);
      for (int col = 0; col < static_cast<int>(num_cols); col++) {
        value ^= sqlite3_column_int64(*stmt, col);
      }
    }
    PERFETTO_CHECK(value != 42);
  }

  state.counters["rows"] = Counter(static_cast<double>(batch_size),
                                   Counter::kIsIterationInvariantRate);
}

BENCHMARK(BM_SqliteStepAndResult)->Apply(BenchmarkArgs);

}  // namespace
