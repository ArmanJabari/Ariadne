#pragma once
#include <string>
#include <cstdint>
#include "cpu_state.h"

struct DisasmLine;

uint16_t ParseImmediate(const std::string& val_str);
bool Is8BitRegister(const std::string& reg);
uint16_t GetRegisterValue(const CPUState& state, const std::string& reg);
void SetRegisterValue(CPUState& state, const std::string& reg, uint16_t val);
bool CalculateParity(uint8_t val);
uint16_t ResolveEffectiveAddress(const CPUState& state, const std::string& op);
void SimulateInstruction(const DisasmLine& line, CPUState& current_state);
bool IsJumpInstruction(const std::string& mnemonic);
uint64_t ParseJumpTargetAddress(const std::string& op_str);