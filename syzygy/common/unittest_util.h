// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Declares a handful of common unittest helper functions.

#ifndef SYZYGY_COMMON_UNITTEST_UTIL_H_
#define SYZYGY_COMMON_UNITTEST_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/environment.h"
#include "base/logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "gtest/gtest.h"

namespace testing {

const char kNtSymbolPathEnvVar[] = "_NT_SYMBOL_PATH";

// Helper class to make sure that a test that plays with the log level doesn't
// change it for other tests.
class ScopedLogLevelSaver {
 public:
  ScopedLogLevelSaver() : level_(logging::GetMinLogLevel()) {
  }

  ~ScopedLogLevelSaver() {
    logging::SetMinLogLevel(level_);
  }

  int level() const { return level_; }

 private:
  int level_;
};

// An intermediate class to add helper streams to a unit-test fixture.
class ApplicationTestBase : public testing::Test {
 public:
  ApplicationTestBase()
      : log_level_(0), log_handler_(NULL), log_to_console_(false) { }

  // @name IO Stream Accessors.
  // Call InitStreams() to route the IO streams to/from specific files;
  // otherwise, they will be routed to/from the NUL device on first use.
  // @{
  FILE* in() const { return GetOrInitFile(&in_, "r"); }
  FILE* out() const { return GetOrInitFile(&out_, "w"); }
  FILE* err() const { return GetOrInitFile(&err_, "w"); }
  // @}

  // Initialize the IO Streams to send output to specific files. Also intercepts
  // logging messages.
  void InitStreams(const base::FilePath& in_path,
                   const base::FilePath& out_path,
                   const base::FilePath& err_path);

  // Manually tear down the various streams.
  void TearDownStreams();

  // Creates a temporary directory, which is cleaned up after the test runs.
  void CreateTemporaryDir(base::FilePath* temp_dir) {
    ASSERT_TRUE(base::CreateNewTempDirectory(L"", temp_dir));
    temp_dirs_.push_back(*temp_dir);
  }

  // Sets up before each test invocation.
  void SetUp() override;

  // Cleans up after each test invocation.
  void TearDown() override;

  // Disables logging for the test in which this is called.
  void DisableLogging() {
    logging::SetMinLogLevel(logging::LOG_FATAL);
  }

  // Enables logging to screen for the test in which this is called.
  void EnableLoggingToConsole() {
    log_to_console_ = true;
  }

 protected:
  typedef testing::Test Super;

  // Used for logging interception, redirecting via err().
  static bool HandleLogMessage(int severity, const char* file, int line,
      size_t message_start, const std::string& str);

  // Tears down the given stream.
  static void TearDownStream(base::ScopedFILE* stream);

  // Helper to initialize a given stream to refer to the NUL device on first
  // use if it hasn't already been associated with a file.
  static FILE* GetOrInitFile(base::ScopedFILE* f, const char* mode);

  // List of temporary directories created during this test invocation.
  typedef std::vector<base::FilePath> DirList;
  DirList temp_dirs_;

  // @name Replacements for the standard IO streams.
  //
  // By default they are routed to the NUL device (on first uninitialized use).
  //
  // @{
  mutable base::ScopedFILE in_;
  mutable base::ScopedFILE out_;
  mutable base::ScopedFILE err_;
  // @}

  // The logging level saved during SetUp. We restore this on TearDown.
  int log_level_;

  // The log message handler that was intercepted.
  logging::LogMessageHandlerFunction log_handler_;

  // If this is true then log messages handled by this fixture will be repeated
  // to the console rather than simply going to the wrapped stderr. Defaults to
  // false.
  bool log_to_console_;

  // The instance of this test that is redirecting logging. This only works for
  // a single instance at a time, but only one test is running at a time so
  // this is not really an issue.
  static ApplicationTestBase* self_;
};

// Helper class for setting an environment variable, then restoring it on
// destruction.
class ScopedEnvironmentVariable {
 public:
  ScopedEnvironmentVariable();
  ScopedEnvironmentVariable(base::StringPiece name, base::StringPiece value);
  ~ScopedEnvironmentVariable();

  // Overrides the environment variable.
  // @param name the name of the variable to override.
  // @param value the value of the override.
  // @returns true for the first call, false on subsequent calls.
  bool Set(base::StringPiece name, base::StringPiece value);

 private:
  std::string name_;
  bool should_restore_;
  std::string restore_value_;

  std::unique_ptr<base::Environment> env_;

  DISALLOW_COPY_AND_ASSIGN(ScopedEnvironmentVariable);
};

// Helper class for setting up a symbol path that points to the output
// directory.
class ScopedSymbolPath {
 public:
  ScopedSymbolPath() {}
  ~ScopedSymbolPath() {}

  // Constructs the symbol path and sets the kNtSymbolPathEnvVar variable.
  bool Setup();

 private:
  ScopedEnvironmentVariable nt_symbol_path_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSymbolPath);
};

}  // namespace testing

#endif  // SYZYGY_COMMON_UNITTEST_UTIL_H_
