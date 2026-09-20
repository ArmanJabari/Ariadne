#include "assembler.h"
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <cctype>

std::string TrimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool ParseImmediateValue(const std::string& val_str, uint32_t& out_val) {
    std::string str = TrimString(val_str);
    if (str.empty()) return false;
    if (str.length() == 3 && str.front() == '\'' && str.back() == '\'') {
        out_val = static_cast<uint8_t>(str[1]);
        return true;
    }
    if (str.length() > 2 && (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X")) {
        try {
            out_val = std::stoul(str, nullptr, 16);
            return true;
        } catch (...) { return false; }
    }
    if (str.back() == 'h' || str.back() == 'H') {
        try {
            out_val = std::stoul(str.substr(0, str.length() - 1), nullptr, 16);
            return true;
        } catch (...) { return false; }
    }
    try {
        out_val = std::stoul(str, nullptr, 10);
        return true;
    } catch (...) { return false; }
}

bool ParseDirectMemory(const std::string& op, uint32_t& out_addr, bool& is_byte, bool& is_word) {
    size_t ob = op.find('[');
    size_t cb = op.find(']');
    if (ob == std::string::npos || cb == std::string::npos || cb <= ob) return false;
    std::string inner = TrimString(op.substr(ob + 1, cb - ob - 1));
    if (!ParseImmediateValue(inner, out_addr)) return false;
    std::string prefix = op.substr(0, ob);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
    is_byte = (prefix.find("byte") != std::string::npos);
    is_word = (prefix.find("word") != std::string::npos);
    return true;
}

int GetReg16Index(const std::string& reg) {
    std::string r = reg;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    if (r == "ax") return 0;
    if (r == "cx") return 1;
    if (r == "dx") return 2;
    if (r == "bx") return 3;
    if (r == "sp") return 4;
    if (r == "bp") return 5;
    if (r == "si") return 6;
    if (r == "di") return 7;
    return -1;
}

int GetReg8Index(const std::string& reg) {
    std::string r = reg;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    if (r == "al") return 0;
    if (r == "cl") return 1;
    if (r == "dl") return 2;
    if (r == "bl") return 3;
    if (r == "ah") return 4;
    if (r == "ch") return 5;
    if (r == "dh") return 6;
    if (r == "bh") return 7;
    return -1;
}

bool TryParseHexBytes(const std::string& input, std::vector<uint8_t>& out_bytes) {
    std::string clean = input;
    std::replace(clean.begin(), clean.end(), ',', ' ');
    std::stringstream ss(clean);
    std::string token;
    std::vector<uint8_t> temp;
    while (ss >> token) {
        if (token.length() > 2 && (token.substr(0, 2) == "0x" || token.substr(0, 2) == "0X")) {
            token = token.substr(2);
        }
        if (token.empty() || token.length() > 2) return false;
        for (char c : token) {
            if (!isxdigit(static_cast<unsigned char>(c))) return false;
        }
        uint32_t v = std::stoul(token, nullptr, 16);
        temp.push_back(static_cast<uint8_t>(v & 0xFF));
    }
    if (temp.empty()) return false;
    out_bytes = temp;
    return true;
}

bool AssembleInstruction(const std::string& input, uint64_t current_addr, std::vector<uint8_t>& out_bytes) {
    std::string s = TrimString(input);
    if (s.empty()) return false;

    size_t sp = s.find_first_of(" \t");
    std::string m = (sp == std::string::npos) ? s : s.substr(0, sp);
    std::string rest = (sp == std::string::npos) ? "" : TrimString(s.substr(sp + 1));
    std::transform(m.begin(), m.end(), m.begin(), ::tolower);

    std::string op1 = "", op2 = "";
    size_t comma = rest.find(',');
    if (comma != std::string::npos) {
        op1 = TrimString(rest.substr(0, comma));
        op2 = TrimString(rest.substr(comma + 1));
    } else {
        op1 = rest;
    }

    if (m == "nop") { out_bytes = { 0x90 }; return true; }
    if (m == "ret" || m == "retn") {
        uint32_t imm = 0;
        if (!op1.empty() && ParseImmediateValue(op1, imm)) {
            out_bytes = { 0xC2, static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
        } else {
            out_bytes = { 0xC3 };
        }
        return true;
    }
    if (m == "retf") { out_bytes = { 0xCB }; return true; }
    if (m == "cbw") { out_bytes = { 0x98 }; return true; }
    if (m == "cwd") { out_bytes = { 0x99 }; return true; }
    if (m == "cld") { out_bytes = { 0xFC }; return true; }
    if (m == "std") { out_bytes = { 0xFD }; return true; }
    if (m == "clc") { out_bytes = { 0xF8 }; return true; }
    if (m == "stc") { out_bytes = { 0xF9 }; return true; }
    if (m == "cli") { out_bytes = { 0xFA }; return true; }
    if (m == "sti") { out_bytes = { 0xFB }; return true; }
    if (m == "into") { out_bytes = { 0xCE }; return true; }
    if (m == "iret") { out_bytes = { 0xCF }; return true; }
    if (m == "movsb") { out_bytes = { 0xA4 }; return true; }
    if (m == "movsw") { out_bytes = { 0xA5 }; return true; }
    if (m == "stosb") { out_bytes = { 0xAA }; return true; }
    if (m == "stosw") { out_bytes = { 0xAB }; return true; }
    if (m == "lodsb") { out_bytes = { 0xAC }; return true; }
    if (m == "lodsw") { out_bytes = { 0xAD }; return true; }

    if (m == "int") {
        uint32_t imm = 0;
        if (ParseImmediateValue(op1, imm)) {
            if (imm == 3) out_bytes = { 0xCC };
            else out_bytes = { 0xCD, static_cast<uint8_t>(imm & 0xFF) };
            return true;
        }
    }

    if (m == "push") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { static_cast<uint8_t>(0x50 + r16) }; return true; }
        std::string r = op1; std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        if (r == "cs") { out_bytes = { 0x0E }; return true; }
        if (r == "ss") { out_bytes = { 0x16 }; return true; }
        if (r == "ds") { out_bytes = { 0x1E }; return true; }
        if (r == "es") { out_bytes = { 0x06 }; return true; }
        uint32_t imm = 0;
        if (ParseImmediateValue(op1, imm)) {
            out_bytes = { 0x68, static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
            return true;
        }
    }

    if (m == "pop") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { static_cast<uint8_t>(0x58 + r16) }; return true; }
        std::string r = op1; std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        if (r == "ss") { out_bytes = { 0x17 }; return true; }
        if (r == "ds") { out_bytes = { 0x1F }; return true; }
        if (r == "es") { out_bytes = { 0x07 }; return true; }
    }

    if (m == "inc") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { static_cast<uint8_t>(0x40 + r16) }; return true; }
        int r8 = GetReg8Index(op1);
        if (r8 >= 0) { out_bytes = { 0xFE, static_cast<uint8_t>(0xC0 + r8) }; return true; }
    }

    if (m == "dec") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { static_cast<uint8_t>(0x48 + r16) }; return true; }
        int r8 = GetReg8Index(op1);
        if (r8 >= 0) { out_bytes = { 0xFE, static_cast<uint8_t>(0xC8 + r8) }; return true; }
    }

    if (m == "not") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { 0xF7, static_cast<uint8_t>(0xD0 + r16) }; return true; }
        int r8 = GetReg8Index(op1);
        if (r8 >= 0) { out_bytes = { 0xF6, static_cast<uint8_t>(0xD0 + r8) }; return true; }
    }

    if (m == "neg") {
        int r16 = GetReg16Index(op1);
        if (r16 >= 0) { out_bytes = { 0xF7, static_cast<uint8_t>(0xD8 + r16) }; return true; }
        int r8 = GetReg8Index(op1);
        if (r8 >= 0) { out_bytes = { 0xF6, static_cast<uint8_t>(0xD8 + r8) }; return true; }
    }

    if (m == "lea") {
        int r16_d = GetReg16Index(op1);
        uint32_t mem_addr = 0;
        bool is_b = false, is_w = false;
        if (r16_d >= 0 && ParseDirectMemory(op2, mem_addr, is_b, is_w)) {
            out_bytes = { 0x8D, static_cast<uint8_t>(0x06 | (r16_d << 3)), static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            return true;
        }
    }

    if (m == "mov") {
        int r16_d = GetReg16Index(op1);
        int r16_s = GetReg16Index(op2);
        int r8_d = GetReg8Index(op1);
        int r8_s = GetReg8Index(op2);
        uint32_t imm = 0;
        uint32_t mem_addr = 0;
        bool is_byte = false, is_word = false;

        if (r16_d >= 0 && r16_s >= 0) {
            out_bytes = { 0x89, static_cast<uint8_t>(0xC0 + (r16_s << 3) + r16_d) };
            return true;
        }
        if (r8_d >= 0 && r8_s >= 0) {
            out_bytes = { 0x88, static_cast<uint8_t>(0xC0 + (r8_s << 3) + r8_d) };
            return true;
        }
        if (r16_d >= 0 && ParseDirectMemory(op2, mem_addr, is_byte, is_word)) {
            if (r16_d == 0) {
                out_bytes = { 0xA1, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            } else {
                out_bytes = { 0x8B, static_cast<uint8_t>(0x06 | (r16_d << 3)), static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            }
            return true;
        }
        if (r8_d >= 0 && ParseDirectMemory(op2, mem_addr, is_byte, is_word)) {
            if (r8_d == 0) {
                out_bytes = { 0xA0, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            } else {
                out_bytes = { 0x8A, static_cast<uint8_t>(0x06 | (r8_d << 3)), static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            }
            return true;
        }
        if (ParseDirectMemory(op1, mem_addr, is_byte, is_word) && r16_s >= 0) {
            if (r16_s == 0) {
                out_bytes = { 0xA3, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            } else {
                out_bytes = { 0x89, static_cast<uint8_t>(0x06 | (r16_s << 3)), static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            }
            return true;
        }
        if (ParseDirectMemory(op1, mem_addr, is_byte, is_word) && r8_s >= 0) {
            if (r8_s == 0) {
                out_bytes = { 0xA2, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            } else {
                out_bytes = { 0x88, static_cast<uint8_t>(0x06 | (r8_s << 3)), static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF) };
            }
            return true;
        }
        if (ParseDirectMemory(op1, mem_addr, is_byte, is_word) && ParseImmediateValue(op2, imm)) {
            if (is_byte) {
                out_bytes = { 0xC6, 0x06, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF), static_cast<uint8_t>(imm & 0xFF) };
            } else {
                out_bytes = { 0xC7, 0x06, static_cast<uint8_t>(mem_addr & 0xFF), static_cast<uint8_t>((mem_addr >> 8) & 0xFF), static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
            }
            return true;
        }
        if (r16_d >= 0 && ParseImmediateValue(op2, imm)) {
            out_bytes = { static_cast<uint8_t>(0xB8 + r16_d), static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
            return true;
        }
        if (r8_d >= 0 && ParseImmediateValue(op2, imm)) {
            out_bytes = { static_cast<uint8_t>(0xB0 + r8_d), static_cast<uint8_t>(imm & 0xFF) };
            return true;
        }
    }

    static const std::unordered_map<std::string, uint8_t> shift_map = {
        {"rol", 0}, {"ror", 1}, {"rcl", 2}, {"rcr", 3},
        {"shl", 4}, {"sal", 4}, {"shr", 5}, {"sar", 7}
    };
    auto it_shift = shift_map.find(m);
    if (it_shift != shift_map.end()) {
        uint8_t digit = it_shift->second;
        int r16 = GetReg16Index(op1);
        int r8 = GetReg8Index(op1);
        if (r16 >= 0 || r8 >= 0) {
            std::string count_str = op2;
            std::transform(count_str.begin(), count_str.end(), count_str.begin(), ::tolower);
            if (count_str.empty() || count_str == "1") {
                if (r16 >= 0) {
                    out_bytes = { 0xD1, static_cast<uint8_t>(0xC0 + (digit << 3) + r16) };
                    return true;
                } else {
                    out_bytes = { 0xD0, static_cast<uint8_t>(0xC0 + (digit << 3) + r8) };
                    return true;
                }
            } else if (count_str == "cl") {
                if (r16 >= 0) {
                    out_bytes = { 0xD3, static_cast<uint8_t>(0xC0 + (digit << 3) + r16) };
                    return true;
                } else {
                    out_bytes = { 0xD2, static_cast<uint8_t>(0xC0 + (digit << 3) + r8) };
                    return true;
                }
            } else {
                uint32_t imm = 0;
                if (ParseImmediateValue(op2, imm)) {
                    if (r16 >= 0) {
                        out_bytes = { 0xC1, static_cast<uint8_t>(0xC0 + (digit << 3) + r16), static_cast<uint8_t>(imm & 0xFF) };
                        return true;
                    } else {
                        out_bytes = { 0xC0, static_cast<uint8_t>(0xC0 + (digit << 3) + r8), static_cast<uint8_t>(imm & 0xFF) };
                        return true;
                    }
                }
            }
        }
    }

    if (m == "test") {
        int r16_d = GetReg16Index(op1);
        int r16_s = GetReg16Index(op2);
        int r8_d = GetReg8Index(op1);
        int r8_s = GetReg8Index(op2);
        uint32_t imm = 0;
        if (r16_d >= 0 && r16_s >= 0) {
            out_bytes = { 0x85, static_cast<uint8_t>(0xC0 + (r16_s << 3) + r16_d) };
            return true;
        }
        if (r8_d >= 0 && r8_s >= 0) {
            out_bytes = { 0x84, static_cast<uint8_t>(0xC0 + (r8_s << 3) + r8_d) };
            return true;
        }
        if (r16_d >= 0 && ParseImmediateValue(op2, imm)) {
            if (r16_d == 0) {
                out_bytes = { 0xA9, static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
            } else {
                out_bytes = { 0xF7, static_cast<uint8_t>(0xC0 + r16_d), static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
            }
            return true;
        }
        if (r8_d >= 0 && ParseImmediateValue(op2, imm)) {
            if (r8_d == 0) {
                out_bytes = { 0xA8, static_cast<uint8_t>(imm & 0xFF) };
            } else {
                out_bytes = { 0xF6, static_cast<uint8_t>(0xC0 + r8_d), static_cast<uint8_t>(imm & 0xFF) };
            }
            return true;
        }
    }

    if (m == "xchg") {
        int r1 = GetReg16Index(op1);
        int r2 = GetReg16Index(op2);
        if (r1 >= 0 && r2 >= 0) {
            if (r1 == 0) { out_bytes = { static_cast<uint8_t>(0x90 + r2) }; return true; }
            if (r2 == 0) { out_bytes = { static_cast<uint8_t>(0x90 + r1) }; return true; }
            out_bytes = { 0x87, static_cast<uint8_t>(0xC0 + (r1 << 3) + r2) };
            return true;
        }
    }

    static const char* alu_names[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
    for (int a = 0; a < 8; a++) {
        if (m == alu_names[a]) {
            int r16_d = GetReg16Index(op1);
            int r16_s = GetReg16Index(op2);
            int r8_d = GetReg8Index(op1);
            int r8_s = GetReg8Index(op2);
            uint32_t imm = 0;
            if (r16_d >= 0 && r16_s >= 0) {
                out_bytes = { static_cast<uint8_t>((a << 3) | 0x01), static_cast<uint8_t>(0xC0 + (r16_s << 3) + r16_d) };
                return true;
            }
            if (r8_d >= 0 && r8_s >= 0) {
                out_bytes = { static_cast<uint8_t>((a << 3) | 0x00), static_cast<uint8_t>(0xC0 + (r8_s << 3) + r8_d) };
                return true;
            }
            if (r16_d >= 0 && ParseImmediateValue(op2, imm)) {
                if (r16_d == 0) {
                    out_bytes = { static_cast<uint8_t>((a << 3) | 0x05), static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
                } else {
                    out_bytes = { 0x81, static_cast<uint8_t>(0xC0 + (a << 3) + r16_d), static_cast<uint8_t>(imm & 0xFF), static_cast<uint8_t>((imm >> 8) & 0xFF) };
                }
                return true;
            }
            if (r8_d >= 0 && ParseImmediateValue(op2, imm)) {
                if (r8_d == 0) {
                    out_bytes = { static_cast<uint8_t>((a << 3) | 0x04), static_cast<uint8_t>(imm & 0xFF) };
                } else {
                    out_bytes = { 0x80, static_cast<uint8_t>(0xC0 + (a << 3) + r8_d), static_cast<uint8_t>(imm & 0xFF) };
                }
                return true;
            }
        }
    }

    if (m == "jmp") {
        uint32_t target = 0;
        if (ParseImmediateValue(op1, target)) {
            int64_t rel8 = static_cast<int64_t>(target) - (static_cast<int64_t>(current_addr) + 2);
            if (rel8 >= -128 && rel8 <= 127) {
                out_bytes = { 0xEB, static_cast<uint8_t>(rel8 & 0xFF) };
                return true;
            }
            int64_t rel16 = static_cast<int64_t>(target) - (static_cast<int64_t>(current_addr) + 3);
            out_bytes = { 0xE9, static_cast<uint8_t>(rel16 & 0xFF), static_cast<uint8_t>((rel16 >> 8) & 0xFF) };
            return true;
        }
    }

    if (m == "call") {
        uint32_t target = 0;
        if (ParseImmediateValue(op1, target)) {
            int64_t rel16 = static_cast<int64_t>(target) - (static_cast<int64_t>(current_addr) + 3);
            out_bytes = { 0xE8, static_cast<uint8_t>(rel16 & 0xFF), static_cast<uint8_t>((rel16 >> 8) & 0xFF) };
            return true;
        }
    }

    static const std::unordered_map<std::string, uint8_t> jcc_map = {
        {"jo", 0x70}, {"jno", 0x71}, {"jb", 0x72}, {"jc", 0x72}, {"jnae", 0x72},
        {"jae", 0x73}, {"jnb", 0x73}, {"jnc", 0x73}, {"je", 0x74}, {"jz", 0x74},
        {"jne", 0x75}, {"jnz", 0x75}, {"jbe", 0x76}, {"jna", 0x76}, {"ja", 0x77},
        {"jnbe", 0x77}, {"js", 0x78}, {"jns", 0x79}, {"jp", 0x7A}, {"jpe", 0x7A},
        {"jnp", 0x7B}, {"jpo", 0x7B}, {"jl", 0x7C}, {"jnge", 0x7C}, {"jge", 0x7D},
        {"jnl", 0x7D}, {"jle", 0x7E}, {"jng", 0x7E}, {"jg", 0x7F}, {"jnle", 0x7F},
        {"loopne", 0xE0}, {"loopnz", 0xE0}, {"loope", 0xE1}, {"loopz", 0xE1},
        {"loop", 0xE2}, {"jcxz", 0xE3}
    };
    auto it_jcc = jcc_map.find(m);
    if (it_jcc != jcc_map.end()) {
        uint32_t target = 0;
        if (ParseImmediateValue(op1, target)) {
            int64_t rel8 = static_cast<int64_t>(target) - (static_cast<int64_t>(current_addr) + 2);
            out_bytes = { it_jcc->second, static_cast<uint8_t>(rel8 & 0xFF) };
            return true;
        }
    }

    if (m == "dw") {
        uint32_t imm = 0;
        if (ParseImmediateValue(rest, imm)) {
            out_bytes.push_back(static_cast<uint8_t>(imm & 0xFF));
            out_bytes.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
            return true;
        }
    }

    if (m == "db") {
        if (!rest.empty() && rest.front() == '\'' && rest.back() == '\'') {
            std::string inner = rest.substr(1, rest.length() - 2);
            for (char c : inner) out_bytes.push_back(static_cast<uint8_t>(c));
            return true;
        }
        if (!rest.empty() && rest.front() == '"' && rest.back() == '"') {
            std::string inner = rest.substr(1, rest.length() - 2);
            for (char c : inner) out_bytes.push_back(static_cast<uint8_t>(c));
            return true;
        }
        uint32_t imm = 0;
        if (ParseImmediateValue(rest, imm)) {
            out_bytes.push_back(static_cast<uint8_t>(imm & 0xFF));
            return true;
        }
    }

    if (TryParseHexBytes(s, out_bytes)) {
        return true;
    }

    return false;
}