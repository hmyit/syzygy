// Copyright 2015 Google Inc. All Rights Reserved.
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
// Declares a backdrop class to be used with Heap events.
#ifndef SYZYGY_BARD_BACKDROPS_HEAP_BACKDROP_H_
#define SYZYGY_BARD_BACKDROPS_HEAP_BACKDROP_H_

#include <windows.h>

#include <map>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/synchronization/lock.h"
#include "syzygy/bard/trace_live_map.h"

namespace bard {
namespace backdrops {

using base::Callback;

// Backdrop class to be used with Heap management events. It stores the
// existing heaps and objects, and maps them from and to their trace file
// addresses. It also stores the total time taken to run all the commands
// so far.
// The class is thread safe for simultaneous access across multiple threads.
class HeapBackdrop {
 public:
  // @name Heap API callback signatures.
  // @{
  using GetProcessHeapCallback = Callback<HANDLE(void)>;
  using HeapAllocCallback = Callback<LPVOID(HANDLE, DWORD, SIZE_T)>;
  using HeapCreateCallback = Callback<HANDLE(DWORD, SIZE_T, SIZE_T)>;
  using HeapDestroyCallback = Callback<BOOL(HANDLE)>;
  using HeapFreeCallback = Callback<BOOL(HANDLE, DWORD, LPVOID)>;
  using HeapReAllocCallback = Callback<LPVOID(HANDLE, DWORD, LPVOID, SIZE_T)>;
  using HeapSizeCallback = Callback<SIZE_T(HANDLE, DWORD, LPCVOID)>;
  // @}

  HeapBackdrop();

  // @name TraceLiveMap accessors.
  // @{
  TraceLiveMap<HANDLE>& heap_map() { return heap_map_; }
  TraceLiveMap<LPVOID>& alloc_map() { return alloc_map_; }

  const TraceLiveMap<HANDLE>& heap_map() const { return heap_map_; }
  const TraceLiveMap<LPVOID>& alloc_map() const { return alloc_map_; }
  // @}

  // @name Heap API functions.
  // @{
  HANDLE GetProcessHeap();
  LPVOID HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes);
  HANDLE HeapCreate(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
  BOOL HeapDestroy(HANDLE heap);
  BOOL HeapFree(HANDLE heap, DWORD flags, LPVOID mem);
  LPVOID HeapReAlloc(HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes);
  SIZE_T HeapSize(HANDLE heap, DWORD flags, LPCVOID mem);
  // @}

  // @name Heap API callback mutators.
  // @{
  void set_get_process_heap(const GetProcessHeapCallback& get_process_heap) {
    get_process_heap_ = get_process_heap;
  }
  void set_heap_alloc(const HeapAllocCallback& heap_alloc) {
    heap_alloc_ = heap_alloc;
  }
  void set_heap_create(const HeapCreateCallback& heap_create) {
    heap_create_ = heap_create;
  }
  void set_heap_destroy(const HeapDestroyCallback& heap_destroy) {
    heap_destroy_ = heap_destroy;
  }
  void set_heap_free(const HeapFreeCallback& heap_free) {
    heap_free_ = heap_free;
  }
  void set_heap_realloc(const HeapReAllocCallback& heap_realloc) {
    heap_realloc_ = heap_realloc;
  }
  void set_heap_size(const HeapSizeCallback& heap_size) {
    heap_size_ = heap_size;
  }
  // @}

  // Update the total time taken by a event with name @p name.
  // @param name the name of the heap function call.
  // @param time the time the heap call took to run, in cycles as
  //     measured by rdtsc.
  void UpdateStats(std::string name, uint64_t time);

  // Exposed for unittesting.
 protected:
  // The following struct holds the statistics generated by a specific
  // function call: the sum of the time it takes to run and the number
  // of times it was called.
  struct Stats {
    uint64_t time;
    uint64_t calls;
  };

  // Pointers to heap API implementation that is being evaluated.
  GetProcessHeapCallback get_process_heap_;
  HeapAllocCallback heap_alloc_;
  HeapCreateCallback heap_create_;
  HeapDestroyCallback heap_destroy_;
  HeapFreeCallback heap_free_;
  HeapReAllocCallback heap_realloc_;
  HeapSizeCallback heap_size_;

  TraceLiveMap<HANDLE> heap_map_;
  TraceLiveMap<LPVOID> alloc_map_;

  std::map<std::string, struct Stats> total_stats_;

  base::Lock lock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(HeapBackdrop);
};

}  // namespace backdrops
}  // namespace bard

#endif  // SYZYGY_BARD_BACKDROPS_HEAP_BACKDROP_H_
