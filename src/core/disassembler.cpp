#include "disassembler.h"
#include <capstone/capstone.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <set>
#include <string>
#include <vector>

static std::string UppercaseHex(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == '0' && i + 1 < result.size() && (result[i + 1] == 'x' || result[i + 1] == 'X')) {
            i += 2;
            while (i < result.size() && std::isxdigit(static_cast<unsigned char>(result[i]))) {
                result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
                i++;
            }
            i--;
        }
    }
    return result;
}

static void FindReferences(csh handle, const uint8_t* code, size_t code_size, uint64_t base_address, std::set<uint64_t>& word_addrs, std::set<uint64_t>& code_targets) {
    cs_insn* insn = cs_malloc(handle);
    const uint8_t* ptr = code;
    size_t size = code_size;
    uint64_t address = base_address;

    while (size > 0) {
        if (cs_disasm_iter(handle, &ptr, &size, &address, insn)) {
            std::string mnem = insn->mnemonic;
            std::string op_str = insn->op_str;

            size_t pos = op_str.find("word ptr [");
            if (pos != std::string::npos) {
                size_t start = pos + 10;
                size_t end = op_str.find(']', start);
                if (end != std::string::npos) {
                    std::string addr_str = op_str.substr(start, end - start);
                    try {
                        uint64_t target_addr = std::stoull(addr_str, nullptr, 0);
                        word_addrs.insert(target_addr);
                    } catch (...) {
                    }
                }
            }

            if (mnem == "call" || (!mnem.empty() && mnem[0] == 'j') || mnem.find("loop") == 0) {
                if (op_str.find('[') == std::string::npos) {
                    size_t hpos = op_str.find("0x");
                    if (hpos == std::string::npos) hpos = op_str.find("0X");
                    if (hpos != std::string::npos) {
                        try {
                            uint64_t target_addr = std::stoull(op_str.substr(hpos), nullptr, 16);
                            code_targets.insert(target_addr);
                        } catch (...) {
                        }
                    } else {
                        try {
                            uint64_t target_addr = std::stoull(op_str, nullptr, 0);
                            code_targets.insert(target_addr);
                        } catch (...) {
                        }
                    }
                }
            }
        } else {
            ptr += 1;
            address += 1;
            size -= 1;
        }
    }
    cs_free(insn, 1);
}

std::vector<DisasmLine> DisassembleBuffer(const std::vector<uint8_t>& buffer) {
    std::vector<DisasmLine> lines;
    if (buffer.empty()) {
        return lines;
    }

    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_16, &handle) != CS_ERR_OK) {
        return lines;
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    const uint8_t* code = buffer.data();
    size_t code_size = buffer.size();
    uint64_t base_address = 0x0100;

    std::set<uint64_t> word_references;
    std::set<uint64_t> code_targets;
    FindReferences(handle, code, code_size, base_address, word_references, code_targets);

    const uint8_t* ptr = code;
    size_t size = code_size;
    uint64_t address = base_address;

    cs_insn* insn = cs_malloc(handle);
    bool stop_code = false;

    while (size > 0) {
        const uint8_t* prev_ptr = ptr;
        size_t prev_size = size;
        uint64_t prev_address = address;

        if (word_references.find(address) != word_references.end() && size >= 2) {
            DisasmLine line;
            line.address = address;
            line.size = 2;
            line.mnemonic = "dw";
            line.is_data = true;

            uint16_t word_val = static_cast<uint16_t>(ptr[0]) | (static_cast<uint16_t>(ptr[1]) << 8);
            char op_buf[32];
            snprintf(op_buf, sizeof(op_buf), "0x%04X", word_val);
            line.op_str = op_buf;

            char byte_buf[32];
            snprintf(byte_buf, sizeof(byte_buf), "%02X %02X", ptr[0], ptr[1]);
            line.bytes = byte_buf;

            lines.push_back(line);

            ptr += 2;
            size -= 2;
            address += 2;
            continue;
        }

        if (code_targets.find(address) != code_targets.end()) {
            stop_code = false;
        }

        if (!stop_code && cs_disasm_iter(handle, &ptr, &size, &address, insn)) {
            DisasmLine line;
            line.address = insn->address;
            line.size = insn->size;
            line.mnemonic = insn->mnemonic;
            line.op_str = UppercaseHex(insn->op_str);
            line.is_data = false;

            char byte_buf[128] = "";
            for (size_t i = 0; i < insn->size; i++) {
                char tmp[8];
                snprintf(tmp, sizeof(tmp), "%02X ", insn->bytes[i]);
                strcat(byte_buf, tmp);
            }
            if (strlen(byte_buf) > 0) {
                byte_buf[strlen(byte_buf) - 1] = '\0';
            }
            line.bytes = byte_buf;

            lines.push_back(line);

            if ((line.mnemonic == "int" && line.op_str == "0x20") ||
                (line.mnemonic == "int" && line.op_str == "0x21" && lines.size() >= 2 &&
                 lines[lines.size() - 2].mnemonic == "mov" &&
                 (lines[lines.size() - 2].op_str.find("0x4C") != std::string::npos ||
                  lines[lines.size() - 2].op_str.find("0x4c") != std::string::npos))) {
                stop_code = true;
            } else if (line.mnemonic == "ret" || line.mnemonic == "retn" || line.mnemonic == "retf" || line.mnemonic == "iret") {
                stop_code = true;
            } else if (line.mnemonic == "jmp") {
                stop_code = true;
            }
        } else {
            DisasmLine line;
            line.address = prev_address;
            line.size = 1;
            line.mnemonic = "db";
            line.is_data = true;

            uint8_t byte_val = prev_ptr[0];
            char op_buf[32];
            if (byte_val >= 0x20 && byte_val <= 0x7E && byte_val != '\'') {
                snprintf(op_buf, sizeof(op_buf), "'%c'", byte_val);
            } else {
                snprintf(op_buf, sizeof(op_buf), "0x%02X", byte_val);
            }
            line.op_str = op_buf;

            char byte_buf[16];
            snprintf(byte_buf, sizeof(byte_buf), "%02X", byte_val);
            line.bytes = byte_buf;

            lines.push_back(line);

            ptr = prev_ptr + 1;
            size = prev_size - 1;
            address = prev_address + 1;
        }
    }

    cs_free(insn, 1);
    cs_close(&handle);

    return lines;
}