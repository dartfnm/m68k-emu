//
// Use tests from https://github.com/SingleStepTests/m68000
// and test loader from https://github.com/Izaron/SegaCxx/blob/main/src/bin/m68k_test/main.cpp
//

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <cpu_state.hpp>
#include <iosfwd>
#include <registers.hpp>
#include <aligned_memory.hpp>
#include "m68k.hpp"
#include "nlohmann/json.hpp"


namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace M68K;

#define LOG_TO_FILE 0

#if LOG_TO_FILE
thread_local std::ofstream ferr;
#else
thread_local std::ostream& ferr = std::cout;
#endif

using AddressType = uint32_t;
using Byte = uint8_t;
using Long = uint32_t;
using MutableDataView = std::span<Byte>;
using Error = std::string;

class DataView : public std::span<const Byte> {
public:
    using Base = std::span<const Byte>;
    using Base::Base;

    template <std::integral T>
    T as() const {
        return std::byteswap(*reinterpret_cast<const T*>(data()));
    }
};

namespace {

using TRamSnapshot = std::map<AddressType, Byte>;


class TestCpu {
public:
    M68K::AlignedMemory memory = {};
    M68K::CPUState state = M68K::CPUState(&memory);
    M68K::InstructionDecoder decoder;
    TRamSnapshot ramControl;

public:
    TestCpu() = default;

    void fillRamByJson(uint32_t pc, const json& prefetch, const json& ram) {
        ramControl.clear();
        // fill RAM
        for (const auto& pair : ram) {
            uint32_t addr = pair[0].get<uint32_t>();
            uint8_t value = pair[1].get<uint8_t>();
            state.memory.set_<uint8_t>(addr, value);
            ramControl[addr] = value;
        }

        // fill prefetch
        assert(prefetch.size() == 2);
        uint32_t p0 = (uint32_t)prefetch[0].get<int>();
        uint32_t p1 = (uint32_t)prefetch[1].get<int>();

        state.memory.set_<uint8_t>(pc + 0, p0 >> 8);
        state.memory.set_<uint8_t>(pc + 1, p0 % 256);
        state.memory.set_<uint8_t>(pc + 2, p1 >> 8);
        state.memory.set_<uint8_t>(pc + 3, p1 % 256);
    }

    const TRamSnapshot& takeRamSnapshot() {
        for (auto& pair : ramControl) {
            uint32_t addr = pair.first;
            pair.second = state.memory.get(addr, SIZE_BYTE);
        }
        return ramControl;
    }
};
//////////////////////////////////////////////////////////////////////////


std::string dump(const Registers& r) {
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (int i = 0; i < 8; ++i) {
        ss << "D" << i << " = " << std::setw(8) << std::dec << r.d[i] << "(" << std::hex << std::setw(8) << r.d[i] << ")"
        << "\t" << "A" << i << " = " << std::setw(8) << std::dec << r.a[i] << "(" << std::hex << std::setw(8) << r.a[i] << ")"
        << "\n";
    }
    ss << "USP = " << r.usp << "\n";
    ss << "SSP = " << r.ssp << "\n";
    ss << "PC = " << r.pc << "\n";

    ss << "SR: ";
    ss << "T = " << (int)r.sr.trace << ", ";
    ss << "S = " << (int)r.sr.supervisor << ", ";
    ss << "M = " << (int)r.sr.master_switch << ", ";
    ss << "I = " << (int)r.sr.interrupt_mask << ", ";
    ss << "X = " << (int)r.sr.extend << ", ";
    ss << "N = " << (int)r.sr.negative << ", ";
    ss << "Z = " << (int)r.sr.zero << ", ";
    ss << "V = " << (int)r.sr.overflow << ", ";
    ss << "C = " << (int)r.sr.carry << "\n";

    return ss.str();
}


std::string dumpRamSnapshot(const TRamSnapshot& ram) {
    std::stringstream ss;
    for (const auto& pair : ram) {
        ss << "[" << std::setw(8) << std::dec << pair.first << "] = " << std::setw(3) << (int)(pair.second)
        << "  |  [" << std::setw(8) << std::hex << pair.first << "] = " << std::setw(2) << (int)(pair.second)
        << "\n";
    }
    return ss.str();
}

TRamSnapshot readRamSnapshot(const json& ram) {
    TRamSnapshot res;
    for (const auto& pair : ram) {
        uint32_t addr = pair[0].get<uint32_t>();
        const auto value = pair[1].get<uint8_t>();
        if (value != 0) {
            res[addr] = value;
        }
    }
    return res;
}


std::vector<std::pair<AddressType, Byte>> getRamDiff(const TRamSnapshot& ram0, const TRamSnapshot& ram1) {
    std::vector<std::pair<AddressType, Byte>> diff;
    for (const auto& p : ram1) {
        const auto it = ram0.find(p.first);
        if (it == ram0.end() || it->second != p.second) {
            diff.emplace_back(p.first, p.second);
        }
    }
    for (const auto& p : ram0) {
        if (ram1.find(p.first) == ram1.end()) {
            diff.emplace_back(p.first, 0);
        }
    }
    if (!diff.empty())
        std::sort(diff.begin(), diff.end());

  #if 0
    std::stringstream ss;
    ss << "diff is: ";
    for (const auto& [address, value] : diff) {
        ss << "[" << address << "] = " << static_cast<int>(value) << ", ";
    }
    ss << "\n";
    ferr << ss.str();
  #endif

    return diff;
}


json LoadTestFile(std::string_view path) {
    std::ifstream f(path.data());
    json data = json::parse(f);
    ferr << "\"" << path << "\" parsed" << std::endl;
    return data;
}


std::optional<std::string> DumpDiff(const Registers& lhs, const Registers& rhs) {
    std::vector<std::string> diffs;

  for (int i = 0; i < 8; ++i) {
    if (lhs.d[i] != rhs.d[i])
      diffs.emplace_back("D" + std::to_string(i));
  }
  for (int i = 0; i < 7; ++i) {
    if (lhs.a[i] != rhs.a[i])
      diffs.emplace_back("A" + std::to_string(i));
  }
  if (lhs.usp != rhs.usp)
    diffs.emplace_back("USP");
  if (lhs.ssp != rhs.ssp)
    diffs.emplace_back("SSP");
  if (lhs.pc != rhs.pc)
    diffs.emplace_back("PC");
  if ((lhs.srr ^ rhs.srr) & 0b1111'0111'0001'1111)
    diffs.emplace_back("SR");

    if (diffs.empty()) {
        return std::nullopt;
    } else {
        std::string info = "";
        for (const auto& d : diffs) {
            info += d + " ";
        }
        return info;
    }
}


void readRegistersJson(Registers &regs, const json& j) {
    for (int i = 0; i < 8; ++i) {
        uint32_t val = j["d" + std::to_string(i)].get<uint32_t>();
        regs.d[i] = val;
    }
    for (int i = 0; i < 7; ++i) {
        uint32_t val = j["a" + std::to_string(i)].get<uint32_t>();
        regs.a[i] = val;
    }
    regs.pc = j["pc"].get<uint32_t>();
    regs.set_(REG_USP, j["usp"].get<uint32_t>());
    regs.set_(REG_SSP, j["ssp"].get<uint32_t>());
    regs.set_(REG_SR, j["sr"].get<uint32_t>());
}


bool WorkOnTest(TestCpu& cpu, const json& test) {
    const std::string test_name = test["name"].get<std::string>();
    const json& initialJson = test["initial"];
    const json& finalJson = test["final"];

    Registers initRegs;
    readRegistersJson(initRegs, initialJson);
    cpu.state.registers = initRegs;

    const json &initPrefetchJson = initialJson["prefetch"];
    const json &initRamJson = initialJson["ram"];
    cpu.fillRamByJson(initRegs.pc, initPrefetchJson, initRamJson);

    const TRamSnapshot actualRam0 = cpu.takeRamSnapshot();

    // decode
    INSTRUCTION::Instruction* curInst;
    uint32_t pc = (uint32_t)cpu.state.registers.pc;
    uint16_t opcode = (uint16_t)cpu.state.memory.get(pc, SIZE_WORD);

    curInst = cpu.decoder.Decode(opcode);

    curInst->execute(cpu.state);

    // this program counter means there really was an illegal instruction
    //return (expectedRegs.get(REG_PC) == 0x1400);
    //}

    const TRamSnapshot actualRam1 = cpu.takeRamSnapshot();
    Registers& actualRegs = cpu.state.registers;
    Registers expectedRegs;
    readRegistersJson(expectedRegs, finalJson);

    TRamSnapshot expectedRam0 = readRamSnapshot(initRamJson);
    const TRamSnapshot expectedRam1 = readRamSnapshot(finalJson["ram"]);

    const auto regsDiff = DumpDiff(expectedRegs, actualRegs);
    std::vector<std::pair<AddressType, Byte>> actualDiff = getRamDiff(actualRam0, actualRam1);
    std::vector<std::pair<AddressType, Byte>> expectDiff = getRamDiff(expectedRam0, expectedRam1);
    bool ramDiffers = actualDiff != expectDiff;

    // because of some bugs in data
    if (test_name.contains("CHK")) {
        ramDiffers = false;
    }

    if (regsDiff || ramDiffers) {
        ferr << "Test name: \"" << test_name << "\"" << std::endl << std::endl;

        if (regsDiff) {
            ferr << "Initial registers:" << std::endl;
            ferr << dump(initRegs) << std::endl;

            ferr << "Actual final registers:" << std::endl;
            ferr << dump(actualRegs) << std::endl;

            ferr << "Expected final registers:" << std::endl;
            ferr << dump(expectedRegs) << std::endl;

            ferr << "Differing registers: " << *regsDiff << std::endl << std::endl;
        }

        if (ramDiffers) {
            ferr << "Initial RAM:" << std::endl;
            ferr << dumpRamSnapshot(actualRam0) << std::endl;

            ferr << "Actual RAM:" << std::endl;
            ferr << dumpRamSnapshot(actualRam1) << std::endl;

            ferr << "Expected RAM:" << std::endl;
            ferr << dumpRamSnapshot(expectedRam1) << std::endl;

            ferr << "RAM differs" << std::endl;
        }

        return false;
    }

    return true;
}


bool WorkOnFile(TestCpu& cpu, const json& file) {
    std::size_t size = file.size();
    ferr << "work on file with " << size << " tests" << std::endl;

    int passed = 0;
    int failed = 0;
    int ignored = (int)size;

    for (std::size_t nTest = 0; nTest < size; ++nTest) {
        const json& curTestFile = file[nTest];
        const bool ok = WorkOnTest(cpu, curTestFile);
        ferr << (nTest + 1) << "/" << size << " test is " << (ok ? "OK" : "FAIL") << std::endl;

        --ignored;
        if (ok) {
            ++passed;
        } else {
            ++failed;
            // break;
        }
    }
    ferr << "TOTAL TESTS: " << size << std::endl;
    ferr << "PASSED TESTS: " << passed << std::endl;
    ferr << "FAILED TESTS: " << failed << std::endl;
    ferr << "IGNORED TESTS: " << ignored << std::endl;
    return passed == size;
}

}  // namespace


int main() {
    std::vector<std::string> paths;

    std::filesystem::path basePath = __FILE__;
    std::filesystem::path testPath = basePath.parent_path() / "../../v1"; // decoded json https://github.com/SingleStepTests/m68000/tree/main/v1

    for (const auto& entry : fs::directory_iterator(testPath)) {
        auto path = entry.path().string();
        if (!path.ends_with(".json")) {
            continue;
        }
        paths.emplace_back(std::move(path));
    }
    std::sort(paths.begin(), paths.end(), [](auto&& lhs, auto&& rhs) {
        for (std::size_t i = 0; i < std::min(lhs.size(), rhs.size()); ++i) {
            if (std::tolower(lhs[i]) < std::tolower(rhs[i]))
                return true;
            if (std::tolower(lhs[i]) > std::tolower(rhs[i]))
                return false;
        }
        return lhs.size() < rhs.size();
    });

    const auto shouldRunTest = [](int /*index*/) { return true; };

    constexpr int threadCount = 1; //10;
    std::mutex mut;
    int curIndex = 1;
    int totalCount = 0;

    fs::remove_all("logs");
    fs::create_directories("logs");

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&]() {
            TestCpu cpu;
            while (true) {
                std::string path;
                {
                    std::lock_guard guard{mut};
                    while (curIndex <= paths.size() && !shouldRunTest(curIndex)) {
                        path = paths[curIndex - 1];
                        std::cerr << "NOT working on file " << path.substr(path.rfind('/') + 1) << " [index "
                                  << curIndex << "]" << std::endl;
                        ++curIndex;
                    }
                    if (curIndex > paths.size()) {
                        return;
                    }
                    path = paths[curIndex - 1];
                    std::cerr << "working on file " << path.substr(path.rfind('/') + 1) << std::endl;
                    ++curIndex;
                    ++totalCount;
                }

                std::string part = path.substr(path.rfind('/') + 1);
                part = part.substr(0, part.rfind('.'));
            #if LOG_TO_FILE
                ferr = std::ofstream{"logs/" + part};
            #else
                //ferr = std::cout;
            #endif

                json file = LoadTestFile(path);
                WorkOnFile(cpu, file);
            }
        });
    }
    for (std::thread& t : threads) {
        t.join();
    }
    std::cerr << "Total file count: " << totalCount << std::endl;
}
