#pragma once

#include "RV64IInstruction.h"
#include <cstdint>

namespace dinorisc {

class RV64IDecoder {
public:
  RV64IDecoder() = default;
  ~RV64IDecoder() = default;

  // Primary interface: decode a single 32-bit instruction at given PC address
  RV64IInstruction decode(uint32_t rawInstruction, uint64_t pc) const;

  // Helper to read a 32-bit instruction from memory (little-endian)
  static uint32_t readInstruction(const uint8_t *data, size_t offset);

private:
  // Internal structures for decoding
  struct DecodedFields {
    uint32_t opcode;
    uint32_t rd, rs1, rs2;
    uint32_t funct3, funct7;
    int32_t immediate;
  };

  // Low-level decoding helpers
  uint32_t readLittleEndian32(const uint8_t *data, size_t offset) const;
  DecodedFields extractFields(uint32_t raw) const;
  int32_t signExtend(uint32_t value, int bits) const;

  // High-level decoding logic
  RV64IInstruction::Opcode determineOpcode(const DecodedFields &fields,
                                           uint32_t raw) const;
  std::vector<RV64IInstruction::Operand>
  extractOperands(const DecodedFields &fields, RV64IInstruction::Opcode opcode,
                  uint32_t raw) const;

  // Format-specific immediate extraction
  int32_t extractITypeImmediate(uint32_t raw) const;
  int32_t extractSTypeImmediate(uint32_t raw) const;
  int32_t extractBTypeImmediate(uint32_t raw) const;
  int32_t extractUTypeImmediate(uint32_t raw) const;
  int32_t extractJTypeImmediate(uint32_t raw) const;
};

} // namespace dinorisc