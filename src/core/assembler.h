#pragma once
#include <string>
#include <vector>
#include <cstdint>

std::string TrimString(const std::string& str);
bool ParseImmediateValue(const std::string& val_str, uint32_t& out_val);
bool ParseDirectMemory(const std::string& op, uint32_t& out_addr, bool& is_byte, bool& is_word);
int GetReg16Index(const std::string& reg);
int GetReg8Index(const std::string& reg);
bool TryParseHexBytes(const std::string& input, std::vector<uint8_t>& out_bytes);
bool AssembleInstruction(const std::string& input, uint64_t current_addr, std::vector<uint8_t>& out_bytes);