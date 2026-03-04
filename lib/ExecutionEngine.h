#pragma once

#include "GuestState.h"
#include <cstdint>
#include <vector>

namespace dinorisc {

class ExecutionEngine {
public:
  uint64_t executeBlock(const std::vector<uint8_t> &machineCode,
                        GuestState *guestState);
};

} // namespace dinorisc
