#include "emulator.h"
#include "disassembler.h"
#include "assembler.h"
#include <sstream>
#include <algorithm>
#include <cctype>

uint16_t ParseImmediate(const std::string& val_str) {
    std::string str = val_str;
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    if (str.find("0x") == 0 || str.find("0X") == 0) {
        return static_cast<uint16_t>(std::stoul(str, nullptr, 16));
    } else if (!str.empty() && (std::isdigit(str[0]) || str[0] == '-')) {
        return static_cast<uint16_t>(std::stol(str, nullptr, 10));
    }
    return 0;
}

bool Is8BitRegister(const std::string& reg) {
    return (reg == "al" || reg == "ah" || reg == "bl" || reg == "bh" ||
            reg == "cl" || reg == "ch" || reg == "dl" || reg == "dh");
}

uint16_t GetRegisterValue(const CPUState& state, const std::string& reg) {
    if (reg == "ax") return state.ax;
    if (reg == "ah") return (state.ax >> 8) & 0xFF;
    if (reg == "al") return state.ax & 0xFF;
    if (reg == "bx") return state.bx;
    if (reg == "bh") return (state.bx >> 8) & 0xFF;
    if (reg == "bl") return state.bx & 0xFF;
    if (reg == "cx") return state.cx;
    if (reg == "ch") return (state.cx >> 8) & 0xFF;
    if (reg == "cl") return state.cx & 0xFF;
    if (reg == "dx") return state.dx;
    if (reg == "dh") return (state.dx >> 8) & 0xFF;
    if (reg == "dl") return state.dx & 0xFF;
    if (reg == "si") return state.si;
    if (reg == "di") return state.di;
    if (reg == "bp") return state.bp;
    if (reg == "sp") return state.sp;
    return ParseImmediate(reg);
}

void SetRegisterValue(CPUState& state, const std::string& reg, uint16_t val) {
    if (reg == "ax") state.ax = val;
    else if (reg == "ah") state.ax = (state.ax & 0x00FF) | ((val & 0xFF) << 8);
    else if (reg == "al") state.ax = (state.ax & 0xFF00) | (val & 0xFF);
    else if (reg == "bx") state.bx = val;
    else if (reg == "bh") state.bx = (state.bx & 0x00FF) | ((val & 0xFF) << 8);
    else if (reg == "bl") state.bx = (state.bx & 0xFF00) | (val & 0xFF);
    else if (reg == "cx") state.cx = val;
    else if (reg == "ch") state.cx = (state.cx & 0x00FF) | ((val & 0xFF) << 8);
    else if (reg == "cl") state.cx = (state.cx & 0xFF00) | (val & 0xFF);
    else if (reg == "dx") state.dx = val;
    else if (reg == "dh") state.dx = (state.dx & 0x00FF) | ((val & 0xFF) << 8);
    else if (reg == "dl") state.dx = (state.dx & 0xFF00) | (val & 0xFF);
    else if (reg == "si") state.si = val;
    else if (reg == "di") state.di = val;
    else if (reg == "bp") state.bp = val;
    else if (reg == "sp") state.sp = val;
}

bool CalculateParity(uint8_t val) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (val & (1 << i)) count++;
    }
    return (count % 2 == 0);
}

uint16_t ResolveEffectiveAddress(const CPUState& state, const std::string& op) {
    size_t ob = op.find('[');
    size_t cb = op.find(']');
    if (ob == std::string::npos || cb == std::string::npos || cb <= ob) return 0;
    std::string inner = op.substr(ob + 1, cb - ob - 1);
    std::string clean = "";
    for (char c : inner) {
        if (c == '+') { clean += " + "; }
        else if (c == '-') { clean += " - "; }
        else { clean += c; }
    }
    std::stringstream ss(clean);
    std::string tok;
    int sign = 1;
    uint16_t total = 0;
    while (ss >> tok) {
        if (tok == "+") { sign = 1; }
        else if (tok == "-") { sign = -1; }
        else {
            int r16 = GetReg16Index(tok);
            uint16_t val = 0;
            if (r16 >= 0) {
                val = GetRegisterValue(state, tok);
            } else {
                uint32_t imm = 0;
                if (ParseImmediateValue(tok, imm)) {
                    val = static_cast<uint16_t>(imm & 0xFFFF);
                }
            }
            total += static_cast<uint16_t>(sign * static_cast<int>(val));
        }
    }
    return total;
}

void SimulateInstruction(const DisasmLine& line, CPUState& current_state) {
    if (line.mnemonic == "mov") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = TrimString(line.op_str.substr(0, comma_pos));
            std::string src = TrimString(line.op_str.substr(comma_pos + 1));

            bool dest_is_mem = (dest.find('[') != std::string::npos && dest.find(']') != std::string::npos);
            bool src_is_mem = (src.find('[') != std::string::npos && src.find(']') != std::string::npos);

            if (src_is_mem) {
                uint16_t ea = ResolveEffectiveAddress(current_state, src);
                if (Is8BitRegister(dest) || src.find("byte") != std::string::npos) {
                    uint8_t val = current_state.memory[ea];
                    SetRegisterValue(current_state, dest, val);
                } else {
                    uint16_t val = current_state.memory[ea] | (current_state.memory[static_cast<uint16_t>(ea + 1)] << 8);
                    SetRegisterValue(current_state, dest, val);
                }
            } else if (dest_is_mem) {
                uint16_t ea = ResolveEffectiveAddress(current_state, dest);
                uint16_t val = GetRegisterValue(current_state, src);
                if (Is8BitRegister(src) || dest.find("byte") != std::string::npos) {
                    current_state.memory[ea] = static_cast<uint8_t>(val & 0xFF);
                } else {
                    current_state.memory[ea] = static_cast<uint8_t>(val & 0xFF);
                    current_state.memory[static_cast<uint16_t>(ea + 1)] = static_cast<uint8_t>((val >> 8) & 0xFF);
                }
            } else {
                dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
                src.erase(std::remove(src.begin(), src.end(), ' '), src.end());
                uint16_t val = GetRegisterValue(current_state, src);
                SetRegisterValue(current_state, dest, val);
            }
        }
    } else if (line.mnemonic == "xor") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = (val1 ^ val2) & mask;

            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            current_state.flag_cf = false;
            current_state.flag_of = false;
        }
    } else if (line.mnemonic == "and") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = (val1 & val2) & mask;

            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            current_state.flag_cf = false;
            current_state.flag_of = false;
        }
    } else if (line.mnemonic == "test") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = (val1 & val2) & mask;

            current_state.flag_cf = false;
            current_state.flag_of = false;
            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
        }
    } else if (line.mnemonic == "or") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = (val1 | val2) & mask;

            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            current_state.flag_cf = false;
            current_state.flag_of = false;
        }
    } else if (line.mnemonic == "not") {
        std::string dest = line.op_str;
        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint32_t res = (~val) & mask;
        SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));
    } else if (line.mnemonic == "neg") {
        std::string dest = line.op_str;
        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;
        uint32_t of_check = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint32_t res = (0 - val) & mask;
        SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

        current_state.flag_cf = (val != 0);
        current_state.flag_zf = (res == 0);
        current_state.flag_sf = ((res & sign_mask) != 0);
        current_state.flag_of = (val == of_check);
        current_state.flag_pf = CalculateParity(res & 0xFF);
    } else if (line.mnemonic == "shl" || line.mnemonic == "sal") {
        size_t comma_pos = line.op_str.find(',');
        std::string dest = (comma_pos != std::string::npos) ? line.op_str.substr(0, comma_pos) : line.op_str;
        std::string src = (comma_pos != std::string::npos) ? line.op_str.substr(comma_pos + 1) : "1";

        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint8_t count = static_cast<uint8_t>(GetRegisterValue(current_state, src) & 0x1F);

        if (count > 0) {
            uint32_t res = (val << count);
            current_state.flag_cf = ((res & (is8 ? 0x100 : 0x10000)) != 0);
            res &= mask;
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            if (count == 1) {
                current_state.flag_of = (current_state.flag_cf != ((res & sign_mask) != 0));
            }
        }
    } else if (line.mnemonic == "shr") {
        size_t comma_pos = line.op_str.find(',');
        std::string dest = (comma_pos != std::string::npos) ? line.op_str.substr(0, comma_pos) : line.op_str;
        std::string src = (comma_pos != std::string::npos) ? line.op_str.substr(comma_pos + 1) : "1";

        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint8_t count = static_cast<uint8_t>(GetRegisterValue(current_state, src) & 0x1F);

        if (count > 0) {
            current_state.flag_cf = ((val & (1 << (count - 1))) != 0);
            uint32_t res = (val >> count) & mask;
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            if (count == 1) {
                current_state.flag_of = ((val & sign_mask) != 0);
            }
        }
    } else if (line.mnemonic == "sar") {
        size_t comma_pos = line.op_str.find(',');
        std::string dest = (comma_pos != std::string::npos) ? line.op_str.substr(0, comma_pos) : line.op_str;
        std::string src = (comma_pos != std::string::npos) ? line.op_str.substr(comma_pos + 1) : "1";

        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint8_t count = static_cast<uint8_t>(GetRegisterValue(current_state, src) & 0x1F);

        if (count > 0) {
            current_state.flag_cf = ((val & (1 << (count - 1))) != 0);
            int32_t sval = is8 ? static_cast<int8_t>(val) : static_cast<int16_t>(val);
            sval >>= count;
            uint32_t res = static_cast<uint32_t>(sval) & mask;
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

            current_state.flag_zf = (res == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_pf = CalculateParity(res & 0xFF);
            current_state.flag_of = false;
        }
    } else if (line.mnemonic == "rol") {
        size_t comma_pos = line.op_str.find(',');
        std::string dest = (comma_pos != std::string::npos) ? line.op_str.substr(0, comma_pos) : line.op_str;
        std::string src = (comma_pos != std::string::npos) ? line.op_str.substr(comma_pos + 1) : "1";

        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

        bool is8 = Is8BitRegister(dest);
        uint8_t bit_width = is8 ? 8 : 16;
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint8_t count = static_cast<uint8_t>(GetRegisterValue(current_state, src) % bit_width);

        if (count > 0) {
            uint32_t res = ((val << count) | (val >> (bit_width - count))) & mask;
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));
            current_state.flag_cf = (res & 1) != 0;
            if (count == 1) {
                current_state.flag_of = current_state.flag_cf != ((res & sign_mask) != 0);
            }
        }
    } else if (line.mnemonic == "ror") {
        size_t comma_pos = line.op_str.find(',');
        std::string dest = (comma_pos != std::string::npos) ? line.op_str.substr(0, comma_pos) : line.op_str;
        std::string src = (comma_pos != std::string::npos) ? line.op_str.substr(comma_pos + 1) : "1";

        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

        bool is8 = Is8BitRegister(dest);
        uint8_t bit_width = is8 ? 8 : 16;
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint8_t count = static_cast<uint8_t>(GetRegisterValue(current_state, src) % bit_width);

        if (count > 0) {
            uint32_t res = ((val >> count) | (val << (bit_width - count))) & mask;
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));
            current_state.flag_cf = (res & sign_mask) != 0;
            if (count == 1) {
                bool high_bit = (res & sign_mask) != 0;
                bool next_high_bit = (res & (sign_mask >> 1)) != 0;
                current_state.flag_of = high_bit != next_high_bit;
            }
        }
    } else if (line.mnemonic == "xchg") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            uint16_t v1 = GetRegisterValue(current_state, dest);
            uint16_t v2 = GetRegisterValue(current_state, src);
            SetRegisterValue(current_state, dest, v2);
            SetRegisterValue(current_state, src, v1);
        }
    } else if (line.mnemonic == "movsx") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            uint8_t val8 = static_cast<uint8_t>(GetRegisterValue(current_state, src) & 0xFF);
            int16_t val16 = static_cast<int16_t>(static_cast<int8_t>(val8));
            SetRegisterValue(current_state, dest, static_cast<uint16_t>(val16));
        }
    } else if (line.mnemonic == "cbw") {
        uint8_t al_val = static_cast<uint8_t>(current_state.ax & 0xFF);
        int16_t ax_val = static_cast<int16_t>(static_cast<int8_t>(al_val));
        current_state.ax = static_cast<uint16_t>(ax_val);
    } else if (line.mnemonic == "cwd") {
        if (current_state.ax & 0x8000) {
            current_state.dx = 0xFFFF;
        } else {
            current_state.dx = 0x0000;
        }
    } else if (line.mnemonic == "movsb") {
        int dir = current_state.flag_df ? -1 : 1;
        uint8_t val = current_state.memory[current_state.ds * 16 + current_state.si];
        current_state.memory[current_state.es * 16 + current_state.di] = val;
        current_state.si += dir;
        current_state.di += dir;
    } else if (line.mnemonic == "stosb") {
        int dir = current_state.flag_df ? -1 : 1;
        uint8_t val = static_cast<uint8_t>(current_state.ax & 0xFF);
        current_state.memory[current_state.es * 16 + current_state.di] = val;
        current_state.di += dir;
    } else if (line.mnemonic == "lodsb") {
        int dir = current_state.flag_df ? -1 : 1;
        uint8_t val = current_state.memory[current_state.ds * 16 + current_state.si];
        current_state.ax = (current_state.ax & 0xFF00) | val;
        current_state.si += dir;
    } else if (line.mnemonic == "add") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;
            uint32_t carry_mask = is8 ? 0x100 : 0x10000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = val1 + val2;

            SetRegisterValue(current_state, dest, static_cast<uint16_t>(res & mask));

            current_state.flag_cf = (res >= carry_mask);
            current_state.flag_zf = ((res & mask) == 0);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_of = (!((val1 ^ val2) & sign_mask) && ((val1 ^ res) & sign_mask));
            current_state.flag_pf = CalculateParity(res & 0xFF);
        }
    } else if (line.mnemonic == "sub" || line.mnemonic == "cmp") {
        size_t comma_pos = line.op_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string dest = line.op_str.substr(0, comma_pos);
            std::string src = line.op_str.substr(comma_pos + 1);

            dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
            src.erase(std::remove(src.begin(), src.end(), ' '), src.end());

            bool is8 = Is8BitRegister(dest);
            uint32_t mask = is8 ? 0xFF : 0xFFFF;
            uint32_t sign_mask = is8 ? 0x80 : 0x8000;

            uint32_t val1 = GetRegisterValue(current_state, dest) & mask;
            uint32_t val2 = GetRegisterValue(current_state, src) & mask;
            uint32_t res = (val1 - val2) & mask;

            if (line.mnemonic == "sub") {
                SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));
            }

            current_state.flag_cf = (val1 < val2);
            current_state.flag_zf = (val1 == val2);
            current_state.flag_sf = ((res & sign_mask) != 0);
            current_state.flag_of = (((val1 ^ val2) & sign_mask) && ((val1 ^ res) & sign_mask));
            current_state.flag_pf = CalculateParity(res & 0xFF);
        }
    } else if (line.mnemonic == "inc") {
        std::string dest = line.op_str;
        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;
        uint32_t of_check = is8 ? 0x7F : 0x7FFF;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint32_t res = (val + 1) & mask;
        SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

        current_state.flag_zf = (res == 0);
        current_state.flag_sf = ((res & sign_mask) != 0);
        current_state.flag_of = (val == of_check);
        current_state.flag_pf = CalculateParity(res & 0xFF);
    } else if (line.mnemonic == "dec") {
        std::string dest = line.op_str;
        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());

        bool is8 = Is8BitRegister(dest);
        uint32_t mask = is8 ? 0xFF : 0xFFFF;
        uint32_t sign_mask = is8 ? 0x80 : 0x8000;
        uint32_t of_check = is8 ? 0x80 : 0x8000;

        uint32_t val = GetRegisterValue(current_state, dest) & mask;
        uint32_t res = (val - 1) & mask;
        SetRegisterValue(current_state, dest, static_cast<uint16_t>(res));

        current_state.flag_zf = (res == 0);
        current_state.flag_sf = ((res & sign_mask) != 0);
        current_state.flag_of = (val == of_check);
        current_state.flag_pf = CalculateParity(res & 0xFF);
    } else if (line.mnemonic == "div") {
        std::string src = line.op_str;
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());
        bool is8 = Is8BitRegister(src);
        if (is8) {
            uint8_t divisor = static_cast<uint8_t>(GetRegisterValue(current_state, src) & 0xFF);
            if (divisor != 0) {
                uint16_t dividend = current_state.ax;
                uint16_t quot = dividend / divisor;
                uint8_t rem = static_cast<uint8_t>(dividend % divisor);
                current_state.ax = (static_cast<uint16_t>(rem) << 8) | (quot & 0xFF);
            }
        } else {
            uint16_t divisor = GetRegisterValue(current_state, src);
            if (divisor != 0) {
                uint32_t dividend = (static_cast<uint32_t>(current_state.dx) << 16) | current_state.ax;
                uint32_t quot = dividend / divisor;
                uint16_t rem = static_cast<uint16_t>(dividend % divisor);
                current_state.ax = static_cast<uint16_t>(quot & 0xFFFF);
                current_state.dx = rem;
            }
        }
    } else if (line.mnemonic == "mul") {
        std::string src = line.op_str;
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());
        bool is8 = Is8BitRegister(src);
        if (is8) {
            uint16_t res = static_cast<uint16_t>(current_state.ax & 0xFF) * static_cast<uint16_t>(GetRegisterValue(current_state, src) & 0xFF);
            current_state.ax = res;
            current_state.flag_cf = current_state.flag_of = ((res & 0xFF00) != 0);
        } else {
            uint32_t res = static_cast<uint32_t>(current_state.ax) * static_cast<uint32_t>(GetRegisterValue(current_state, src));
            current_state.ax = static_cast<uint16_t>(res & 0xFFFF);
            current_state.dx = static_cast<uint16_t>((res >> 16) & 0xFFFF);
            current_state.flag_cf = current_state.flag_of = (current_state.dx != 0);
        }
    } else if (line.mnemonic == "push") {
        std::string src = line.op_str;
        src.erase(std::remove(src.begin(), src.end(), ' '), src.end());
        uint16_t val = GetRegisterValue(current_state, src);
        current_state.push(val);
    } else if (line.mnemonic == "pop") {
        std::string dest = line.op_str;
        dest.erase(std::remove(dest.begin(), dest.end(), ' '), dest.end());
        uint16_t val = current_state.pop();
        SetRegisterValue(current_state, dest, val);
    } else if (line.mnemonic == "call") {
        uint16_t return_address = static_cast<uint16_t>(line.address + line.size);
        current_state.push(return_address);
    } else if (line.mnemonic == "ret" || line.mnemonic == "retn") {
        current_state.pop();
    } else if (line.mnemonic == "std") {
        current_state.flag_df = true;
    } else if (line.mnemonic == "cld") {
        current_state.flag_df = false;
    }
}

bool IsJumpInstruction(const std::string& mnemonic) {
    return (mnemonic == "call" || mnemonic == "jmp" ||
            mnemonic == "je"   || mnemonic == "jz"   ||
            mnemonic == "jne"  || mnemonic == "jnz"  ||
            mnemonic == "ja"   || mnemonic == "jnbe" ||
            mnemonic == "jae"  || mnemonic == "jnb"  || mnemonic == "jnc" ||
            mnemonic == "jb"   || mnemonic == "jnae" || mnemonic == "jc"  ||
            mnemonic == "jbe"  || mnemonic == "jna"  ||
            mnemonic == "jg"   || mnemonic == "jnle" ||
            mnemonic == "jge"  || mnemonic == "jnl"  ||
            mnemonic == "jl"   || mnemonic == "jnge" ||
            mnemonic == "jle"  || mnemonic == "jng"  ||
            mnemonic == "js"   || mnemonic == "jns"  ||
            mnemonic == "jo"   || mnemonic == "jno"  ||
            mnemonic == "jp"   || mnemonic == "jpe"  ||
            mnemonic == "jnp"  || mnemonic == "jpo"  ||
            mnemonic == "jcxz" ||
            mnemonic == "loop" || mnemonic == "loope" || mnemonic == "loopz" || mnemonic == "loopne" || mnemonic == "loopnz");
}

uint64_t ParseJumpTargetAddress(const std::string& op_str) {
    size_t hpos = op_str.find("0x");
    if (hpos == std::string::npos) {
        hpos = op_str.find("0X");
    }
    if (hpos != std::string::npos) {
        try {
            return std::stoull(op_str.substr(hpos), nullptr, 16);
        } catch (...) {}
    }
    return 0;
}