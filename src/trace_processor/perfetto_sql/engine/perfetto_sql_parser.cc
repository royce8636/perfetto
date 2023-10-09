/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "src/trace_processor/perfetto_sql/engine/perfetto_sql_parser.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "perfetto/base/logging.h"
#include "perfetto/base/status.h"
#include "perfetto/ext/base/flat_hash_map.h"
#include "perfetto/ext/base/status_or.h"
#include "perfetto/ext/base/string_utils.h"
#include "src/trace_processor/perfetto_sql/engine/perfetto_sql_preprocessor.h"
#include "src/trace_processor/sqlite/sql_source.h"
#include "src/trace_processor/sqlite/sqlite_tokenizer.h"
#include "src/trace_processor/util/status_macros.h"

namespace perfetto {
namespace trace_processor {
namespace {

using Token = SqliteTokenizer::Token;
using Statement = PerfettoSqlParser::Statement;

enum class State {
  kStmtStart,
  kCreate,
  kInclude,
  kIncludePerfetto,
  kCreateOr,
  kCreateOrReplace,
  kCreateOrReplacePerfetto,
  kCreatePerfetto,
  kPassthrough,
};

bool KeywordEqual(std::string_view expected, std::string_view actual) {
  PERFETTO_DCHECK(std::all_of(expected.begin(), expected.end(), islower));
  return std::equal(expected.begin(), expected.end(), actual.begin(),
                    actual.end(),
                    [](char a, char b) { return a == tolower(b); });
}

bool TokenIsSqliteKeyword(std::string_view keyword, SqliteTokenizer::Token t) {
  return t.token_type == SqliteTokenType::TK_GENERIC_KEYWORD &&
         KeywordEqual(keyword, t.str);
}

bool TokenIsCustomKeyword(std::string_view keyword, SqliteTokenizer::Token t) {
  return t.token_type == SqliteTokenType::TK_ID && KeywordEqual(keyword, t.str);
}

bool IsValidModuleWord(const std::string& word) {
  for (const char& c : word) {
    if (!std::isalnum(c) && (c != '_') && !std::islower(c)) {
      return false;
    }
  }
  return true;
}

bool ValidateModuleName(const std::string& name) {
  std::vector<std::string> packages = base::SplitString(name, ".");
  return std::find_if(packages.begin(), packages.end(),
                      std::not_fn(IsValidModuleWord)) == packages.end();
}

std::string SerializeArgs(std::vector<std::pair<SqlSource, SqlSource>> args) {
  bool comma = false;
  std::string serialized;
  for (const auto& [name, type] : args) {
    if (comma) {
      serialized.append(", ");
    }
    comma = true;
    serialized.append(name.sql().c_str());
    serialized.push_back(' ');
    serialized.append(type.sql().c_str());
  }
  return serialized;
}

}  // namespace

PerfettoSqlParser::PerfettoSqlParser(
    SqlSource source,
    const base::FlatHashMap<std::string, PerfettoSqlPreprocessor::Macro>&
        macros)
    : preprocessor_(std::move(source), macros),
      tokenizer_(SqlSource::FromTraceProcessorImplementation("")) {}

bool PerfettoSqlParser::Next() {
  PERFETTO_CHECK(status_.ok());

  if (!preprocessor_.NextStatement()) {
    status_ = preprocessor_.status();
    return false;
  }
  tokenizer_.Reset(preprocessor_.statement());

  State state = State::kStmtStart;
  std::optional<Token> first_non_space_token;
  for (Token token = tokenizer_.Next();; token = tokenizer_.Next()) {
    // Space should always be completely ignored by any logic below as it will
    // never change the current state in the state machine.
    if (token.token_type == SqliteTokenType::TK_SPACE) {
      continue;
    }

    if (token.IsTerminal()) {
      // If we have a non-space character we've seen, just return all the stuff
      // after that point.
      if (first_non_space_token) {
        statement_ = SqliteSql{};
        statement_sql_ = tokenizer_.Substr(*first_non_space_token, token);
        return true;
      }
      // This means we've seen a semi-colon without any non-space content. Just
      // try and find the next statement as this "statement" is a noop.
      if (token.token_type == SqliteTokenType::TK_SEMI) {
        continue;
      }
      // This means we've reached the end of the SQL.
      PERFETTO_DCHECK(token.str.empty());
      return false;
    }

    // If we've not seen a space character, keep track of the current position.
    if (!first_non_space_token) {
      first_non_space_token = token;
    }

    switch (state) {
      case State::kPassthrough:
        statement_ = SqliteSql{};
        statement_sql_ = preprocessor_.statement();
        return true;
      case State::kStmtStart:
        if (TokenIsSqliteKeyword("create", token)) {
          state = State::kCreate;
        } else if (TokenIsCustomKeyword("include", token)) {
          state = State::kInclude;
        } else {
          state = State::kPassthrough;
        }
        break;
      case State::kInclude:
        if (TokenIsCustomKeyword("perfetto", token)) {
          state = State::kIncludePerfetto;
        } else {
          return ErrorAtToken(token,
                              "Use 'INCLUDE PERFETTO MODULE {include_key}'.");
        }
        break;
      case State::kIncludePerfetto:
        if (TokenIsCustomKeyword("module", token)) {
          return ParseIncludePerfettoModule(*first_non_space_token);
        } else {
          return ErrorAtToken(token,
                              "Use 'INCLUDE PERFETTO MODULE {include_key}'.");
        }
      case State::kCreate:
        if (TokenIsSqliteKeyword("trigger", token)) {
          // TODO(lalitm): add this to the "errors" documentation page
          // explaining why this is the case.
          return ErrorAtToken(
              token, "Creating triggers is not supported in PerfettoSQL.");
        }
        if (TokenIsCustomKeyword("perfetto", token)) {
          state = State::kCreatePerfetto;
        } else if (TokenIsSqliteKeyword("or", token)) {
          state = State::kCreateOr;
        } else {
          state = State::kPassthrough;
        }
        break;
      case State::kCreateOr:
        state = TokenIsSqliteKeyword("replace", token) ? State::kCreateOrReplace
                                                       : State::kPassthrough;
        break;
      case State::kCreateOrReplace:
        state = TokenIsCustomKeyword("perfetto", token)
                    ? State::kCreateOrReplacePerfetto
                    : State::kPassthrough;
        break;
      case State::kCreateOrReplacePerfetto:
      case State::kCreatePerfetto:
        if (TokenIsCustomKeyword("function", token)) {
          return ParseCreatePerfettoFunction(
              state == State::kCreateOrReplacePerfetto, *first_non_space_token);
        }
        if (TokenIsSqliteKeyword("table", token)) {
          return ParseCreatePerfettoTable(*first_non_space_token);
        }
        if (TokenIsCustomKeyword("macro", token)) {
          return ParseCreatePerfettoMacro(state ==
                                          State::kCreateOrReplacePerfetto);
        }
        base::StackString<1024> err(
            "Expected 'FUNCTION', 'TABLE' or 'MACRO' after 'CREATE PERFETTO', "
            "received '%*s'.",
            static_cast<int>(token.str.size()), token.str.data());
        return ErrorAtToken(token, err.c_str());
    }
  }
}

bool PerfettoSqlParser::ParseIncludePerfettoModule(
    Token first_non_space_token) {
  auto tok = tokenizer_.NextNonWhitespace();
  auto terminal = tokenizer_.NextTerminal();
  std::string key = tokenizer_.Substr(tok, terminal).sql();

  if (!ValidateModuleName(key)) {
    base::StackString<1024> err(
        "Only alphanumeric characters, dots and underscores allowed in include "
        "keys: '%s'",
        key.c_str());
    return ErrorAtToken(tok, err.c_str());
  }

  statement_ = Include{key};
  statement_sql_ = tokenizer_.Substr(first_non_space_token, terminal);
  return true;
}

bool PerfettoSqlParser::ParseCreatePerfettoTable(Token first_non_space_token) {
  Token table_name = tokenizer_.NextNonWhitespace();
  if (table_name.token_type != SqliteTokenType::TK_ID) {
    base::StackString<1024> err("Invalid table name %.*s",
                                static_cast<int>(table_name.str.size()),
                                table_name.str.data());
    return ErrorAtToken(table_name, err.c_str());
  }
  std::string name(table_name.str);

  auto token = tokenizer_.NextNonWhitespace();
  if (!TokenIsSqliteKeyword("as", token)) {
    base::StackString<1024> err(
        "Expected 'AS' after table_name, received "
        "%*s.",
        static_cast<int>(token.str.size()), token.str.data());
    return ErrorAtToken(token, err.c_str());
  }

  Token first = tokenizer_.NextNonWhitespace();
  Token terminal = tokenizer_.NextTerminal();
  statement_ = CreateTable{std::move(name), tokenizer_.Substr(first, terminal)};
  statement_sql_ = tokenizer_.Substr(first_non_space_token, terminal);
  return true;
}

bool PerfettoSqlParser::ParseCreatePerfettoFunction(
    bool replace,
    Token first_non_space_token) {
  std::string prototype;
  Token function_name = tokenizer_.NextNonWhitespace();
  if (function_name.token_type != SqliteTokenType::TK_ID) {
    // TODO(lalitm): add a link to create function documentation.
    base::StackString<1024> err("Invalid function name %.*s",
                                static_cast<int>(function_name.str.size()),
                                function_name.str.data());
    return ErrorAtToken(function_name, err.c_str());
  }
  prototype.append(function_name.str);

  // TK_LP == '(' (i.e. left parenthesis).
  if (Token lp = tokenizer_.NextNonWhitespace();
      lp.token_type != SqliteTokenType::TK_LP) {
    // TODO(lalitm): add a link to create function documentation.
    return ErrorAtToken(lp, "Malformed function prototype: '(' expected");
  }

  std::vector<Argument> args;
  if (!ParseArgumentDefinitions(args)) {
    return false;
  }

  prototype.push_back('(');
  prototype.append(SerializeArgs(args));
  prototype.push_back(')');

  if (Token returns = tokenizer_.NextNonWhitespace();
      !TokenIsCustomKeyword("returns", returns)) {
    // TODO(lalitm): add a link to create function documentation.
    return ErrorAtToken(returns, "Expected keyword 'returns'");
  }

  Token ret_token = tokenizer_.NextNonWhitespace();
  std::string ret;
  bool table_return = TokenIsSqliteKeyword("table", ret_token);
  if (table_return) {
    if (Token lp = tokenizer_.NextNonWhitespace();
        lp.token_type != SqliteTokenType::TK_LP) {
      // TODO(lalitm): add a link to create function documentation.
      return ErrorAtToken(lp, "Malformed table return: '(' expected");
    }
    // Table function return.
    std::vector<Argument> ret_args;
    if (!ParseArgumentDefinitions(ret_args)) {
      return false;
    }
    ret = SerializeArgs(ret_args);
  } else if (ret_token.token_type != SqliteTokenType::TK_ID) {
    // TODO(lalitm): add a link to create function documentation.
    return ErrorAtToken(ret_token, "Invalid return type");
  } else {
    // Scalar function return.
    ret = ret_token.str;
  }

  if (Token as_token = tokenizer_.NextNonWhitespace();
      !TokenIsSqliteKeyword("as", as_token)) {
    // TODO(lalitm): add a link to create function documentation.
    return ErrorAtToken(as_token, "Expected keyword 'as'");
  }

  Token first = tokenizer_.NextNonWhitespace();
  Token terminal = tokenizer_.NextTerminal();
  statement_ = CreateFunction{replace, std::move(prototype), std::move(ret),
                              tokenizer_.Substr(first, terminal), table_return};
  statement_sql_ = tokenizer_.Substr(first_non_space_token, terminal);
  return true;
}

bool PerfettoSqlParser::ParseCreatePerfettoMacro(bool replace) {
  Token name = tokenizer_.NextNonWhitespace();
  if (name.token_type != SqliteTokenType::TK_ID) {
    // TODO(lalitm): add a link to create macro documentation.
    base::StackString<1024> err("Invalid macro name %.*s",
                                static_cast<int>(name.str.size()),
                                name.str.data());
    return ErrorAtToken(name, err.c_str());
  }

  // TK_LP == '(' (i.e. left parenthesis).
  if (Token lp = tokenizer_.NextNonWhitespace();
      lp.token_type != SqliteTokenType::TK_LP) {
    // TODO(lalitm): add a link to create macro documentation.
    return ErrorAtToken(lp, "Malformed macro prototype: '(' expected");
  }

  std::vector<Argument> args;
  if (!ParseArgumentDefinitions(args)) {
    return false;
  }

  if (Token returns = tokenizer_.NextNonWhitespace();
      !TokenIsCustomKeyword("returns", returns)) {
    // TODO(lalitm): add a link to create macro documentation.
    return ErrorAtToken(returns, "Expected keyword 'returns'");
  }

  Token returns_value = tokenizer_.NextNonWhitespace();
  if (returns_value.token_type != SqliteTokenType::TK_ID) {
    // TODO(lalitm): add a link to create function documentation.
    return ErrorAtToken(returns_value, "Expected return type");
  }

  if (Token as_token = tokenizer_.NextNonWhitespace();
      !TokenIsSqliteKeyword("as", as_token)) {
    // TODO(lalitm): add a link to create macro documentation.
    return ErrorAtToken(as_token, "Expected keyword 'as'");
  }

  Token first = tokenizer_.NextNonWhitespace();
  Token tok = tokenizer_.NextTerminal();
  statement_ = CreateMacro{
      replace, tokenizer_.SubstrToken(name), std::move(args),
      tokenizer_.SubstrToken(returns_value), tokenizer_.Substr(first, tok)};
  return true;
}

bool PerfettoSqlParser::ParseArgumentDefinitions(std::vector<Argument>& res) {
  enum TokenType {
    kIdOrRp,
    kId,
    kType,
    kCommaOrRp,
  };

  std::optional<Token> id = std::nullopt;
  TokenType expected = kIdOrRp;
  for (Token tok = tokenizer_.NextNonWhitespace();;
       tok = tokenizer_.NextNonWhitespace()) {
    // Keywords can be used as names accidentally so have an explicit error
    // message for those.
    if (tok.token_type == SqliteTokenType::TK_GENERIC_KEYWORD) {
      base::StackString<1024> err(
          "Malformed function/macro prototype: %.*s is a SQL keyword so "
          "cannot appear in a prototype",
          static_cast<int>(tok.str.size()), tok.str.data());
      return ErrorAtToken(tok, err.c_str());
    }
    if (expected == kCommaOrRp) {
      PERFETTO_CHECK(expected == kCommaOrRp);
      if (tok.token_type == SqliteTokenType::TK_RP) {
        return true;
      }
      if (tok.token_type == SqliteTokenType::TK_COMMA) {
        expected = kId;
        continue;
      }
      return ErrorAtToken(tok, "')' or ',' expected");
    }
    if (expected == kType) {
      if (tok.token_type != SqliteTokenType::TK_ID) {
        // TODO(lalitm): add a link to documentation.
        base::StackString<1024> err("%.*s is not a valid argument type",
                                    static_cast<int>(tok.str.size()),
                                    tok.str.data());
        return ErrorAtToken(tok, err.c_str());
      }
      PERFETTO_CHECK(id);
      res.push_back(std::make_pair(tokenizer_.SubstrToken(*id),
                                   tokenizer_.SubstrToken(tok)));
      id = std::nullopt;
      expected = kCommaOrRp;
      continue;
    }

    // kIdOrRp only happens on the very first token.
    if (tok.token_type == SqliteTokenType::TK_RP && expected == kIdOrRp) {
      return true;
    }

    if (tok.token_type != SqliteTokenType::TK_ID) {
      // TODO(lalitm): add a link to documentation.
      base::StackString<1024> err("%.*s is not a valid argument name",
                                  static_cast<int>(tok.str.size()),
                                  tok.str.data());
      return ErrorAtToken(tok, err.c_str());
    }
    id = tok;
    expected = kType;
    continue;
  }
}

bool PerfettoSqlParser::ErrorAtToken(const SqliteTokenizer::Token& token,
                                     const char* error) {
  std::string traceback = tokenizer_.AsTraceback(token);
  status_ = base::ErrStatus("%s%s", traceback.c_str(), error);
  return false;
}

}  // namespace trace_processor
}  // namespace perfetto
