//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// UNSUPPORTED: availability-pmr-missing

// Exercise the default resource without test_memory_resource's RTTI dependency,
// including configurations that disable both RTTI and threads.

#include <cassert>
#include <memory_resource>

int main(int, char**) {
  auto* initial = std::pmr::get_default_resource();
  assert(initial == std::pmr::new_delete_resource());
  char buffer[1024];
  std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer), std::pmr::null_memory_resource());
  assert(std::pmr::set_default_resource(&resource) == initial);
  assert(std::pmr::get_default_resource() == &resource);
  std::pmr::polymorphic_allocator<int> allocator;
  int* values = allocator.allocate(4);
  values[0]   = 7;
  assert(values[0] == 7);
  allocator.deallocate(values, 4);
  assert(std::pmr::set_default_resource(nullptr) == &resource);
  assert(std::pmr::get_default_resource() == initial);
  return 0;
}
