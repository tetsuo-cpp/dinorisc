#pragma once

#include "Instruction.h"
#include <cstdint>

namespace dinorisc {
namespace riscv {

class Decoder {
public:
  Decoder() = default;
  ~Decoder() = default;

  // Decode instruction from memory at given offset and PC address
  Instruction decode(const uint8_t *data, size_t offset, uint64_t pc) const;

private:
  // Internal structures for decoding
  struct DecodedFields {
    uint32_t opcode;
    uint32_t rd, rs1, rs2;
    uint32_t funct3, funct7;
    int32_t immediate;
  };

  // Low-level decoding helpers
  DecodedFields extractFields(uint32_t raw) const;
  int32_t signExtend(uint32_t value, int bits) const;

  // High-level decoding logic
  Instruction::Opcode determineOpcode(const DecodedFields &fields,
                                      uint32_t raw) const;
  std::vector<Instruction::Operand> extractOperands(const DecodedFields &fields,
                                                    Instruction::Opcode opcode,
                                                    uint32_t raw) const;

  // Operand extraction helpers
  std::vector<Instruction::Operand>
  extractRTypeOperands(const DecodedFields &fields) const;
  std::vector<Instruction::Operand>
  extractITypeOperands(const DecodedFields &fields, uint32_t raw) const;

  // Format-specific immediate extraction
  int32_t extractITypeImmediate(uint32_t raw) const;
  int32_t extractSTypeImmediate(uint32_t raw) const;
  int32_t extractBTypeImmediate(uint32_t raw) const;
  int32_t extractUTypeImmediate(uint32_t raw) const;
  int32_t extractJTypeImmediate(uint32_t raw) const;

  // Internal decode implementation
  Instruction decode(uint32_t rawInstruction, uint64_t pc) const;

  // Helper to read a 32-bit instruction from memory (little-endian)
  static uint32_t readInstructionFromMemory(const uint8_t *data, size_t offset);
};

} // namespace riscv
} // namespace dinorisc
