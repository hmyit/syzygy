// Copyright 2013 Google Inc. All Rights Reserved.
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

#include "syzygy/agent/asan/nested_heap.h"

#include <algorithm>

#include "gtest/gtest.h"
#include "syzygy/agent/asan/rtl_impl.h"
#include "syzygy/agent/asan/runtime.h"
#include "syzygy/agent/asan/shadow.h"
#include "syzygy/agent/asan/unittest_util.h"
#include "syzygy/common/align.h"

namespace agent {
namespace asan {

namespace {

class NestedHeapTest : public testing::Test {
 public:
  void SetUp() OVERRIDE {
    testing::Test::SetUp();
    runtime_.SetUp(std::wstring());
    agent::asan::SetUpRtl(&runtime_);
  }

  void TearDown() OVERRIDE {
    agent::asan::TearDownRtl();
    runtime_.TearDown();
    testing::Test::TearDown();
  }

  static const size_t kMaxAlignment = 2048;
  // If we want to test the alignments up to 2048 we need a buffer of at least
  // 3 * 2048 bytes:
  // +--- 0 <= size < 2048 bytes---+---2048 bytes---+--2048 bytes--+
  // ^buffer                       ^aligned_buffer  ^user_pointer
  static const size_t kBufferSize = kMaxAlignment * 3;

  bool MemoryRangeIsPoisoned(const uint8_t* address, size_t size) {
    EXPECT_TRUE(address != NULL);
    for (size_t i = 0; i < size; ++i) {
      if (Shadow::IsAccessible(address + i))
        return false;
    }
    return true;
  }

  bool MemoryRangeIsAccessible(const uint8_t* address, size_t size) {
    EXPECT_TRUE(address != NULL);
    for (size_t i = 0; i < size; ++i) {
      if (!Shadow::IsAccessible(address + i))
        return false;
    }
    return true;
  }

  AsanRuntime runtime_;

  // The buffers we use internally.
  uint8_t buffer[kBufferSize];
  uint8_t buffer_copy[kBufferSize];
};

}  // namespace

TEST_F(NestedHeapTest, IntegrationTest) {
  const size_t kAllocSize = 100;
  const uint8_t kMagicValue = 0x9C;

  for (size_t alignment = kShadowRatio; alignment <= kMaxAlignment;
       alignment *= 2) {
    uint8_t* aligned_buffer = reinterpret_cast<uint8_t*>(
        common::AlignUp(reinterpret_cast<size_t>(buffer), alignment));
    uint8_t* aligned_buffer_copy = reinterpret_cast<uint8_t*>(
        common::AlignUp(reinterpret_cast<size_t>(buffer_copy), alignment));

    // The simulated 'allocations' that we use must be a multiple of 8 bytes
    // in length.
    size_t real_buffer_size = common::AlignDown(
        kBufferSize - (aligned_buffer - buffer), 8);
    size_t real_buffer_copy_size = common::AlignDown(
        kBufferSize - (aligned_buffer_copy - buffer_copy), 8);

    EXPECT_TRUE(MemoryRangeIsAccessible(aligned_buffer, real_buffer_size));
    asan_PoisonMemoryRange(aligned_buffer, real_buffer_size);
    EXPECT_TRUE(MemoryRangeIsPoisoned(aligned_buffer, real_buffer_size));
    asan_UnpoisonMemoryRange(aligned_buffer, real_buffer_size);
    EXPECT_TRUE(MemoryRangeIsAccessible(aligned_buffer, real_buffer_size));

    size_t asan_size = asan_GetAsanObjectSize(kAllocSize, alignment);
    ASSERT_GE(real_buffer_size, asan_size);
    ASSERT_GE(real_buffer_copy_size, asan_size);

    asan_InitializeObject(aligned_buffer, kAllocSize, alignment);

    void* user_pointer = NULL;
    size_t tmp_size = 0;
    asan_GetUserExtent(aligned_buffer, &user_pointer, &tmp_size);
    EXPECT_NE(reinterpret_cast<void*>(NULL), user_pointer);
    EXPECT_EQ(kAllocSize, tmp_size);
    ::memset(user_pointer, kMagicValue, kAllocSize);

    void* asan_pointer = NULL;
    asan_GetAsanExtent(user_pointer, &asan_pointer, &tmp_size);
    EXPECT_EQ(asan_size, tmp_size);
    EXPECT_EQ(reinterpret_cast<void*>(aligned_buffer), asan_pointer);

    asan_CloneObject(aligned_buffer, aligned_buffer_copy);
    void* user_pointer_copy = NULL;
    asan_GetUserExtent(aligned_buffer_copy, &user_pointer_copy, &tmp_size);
    EXPECT_NE(reinterpret_cast<void*>(NULL), user_pointer_copy);

    for (size_t i = 0; i < kAllocSize; ++i)
      EXPECT_EQ(kMagicValue, reinterpret_cast<uint8_t*>(user_pointer_copy)[i]);

    size_t header_size =
        reinterpret_cast<uint8_t*>(user_pointer_copy) - aligned_buffer_copy;
    EXPECT_TRUE(MemoryRangeIsPoisoned(aligned_buffer_copy, header_size));
    EXPECT_TRUE(MemoryRangeIsAccessible(
        reinterpret_cast<uint8_t*>(user_pointer), kAllocSize));
    EXPECT_TRUE(MemoryRangeIsPoisoned(
        reinterpret_cast<uint8_t*>(user_pointer) + kAllocSize,
        asan_size - kAllocSize - header_size));

    asan_QuarantineObject(aligned_buffer);
    EXPECT_TRUE(MemoryRangeIsPoisoned(aligned_buffer, asan_size));

    asan_DestroyObject(aligned_buffer);
    // Destroying the object shouldn't affect the shadow memory.
    EXPECT_TRUE(MemoryRangeIsPoisoned(aligned_buffer, asan_size));

    asan_UnpoisonMemoryRange(aligned_buffer, real_buffer_size);
    asan_UnpoisonMemoryRange(aligned_buffer_copy, real_buffer_copy_size);
  }
}

}  // namespace asan
}  // namespace agent
