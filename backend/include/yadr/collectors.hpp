#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "yadr/collector.hpp"
#include "yadr/proc_utils.hpp"

namespace yadr {

// хранит предыдущие счётчики jiffies для вычисления delta-загрузки
class CpuCollector final : public Collector {
public:
    void sample(Snapshot& out, double dt_s) override;

private:
    proc::CpuTimes prev_total_{};
    std::vector<proc::CpuTimes> prev_per_core_;
    bool primed_ = false;
};

class MemoryCollector final : public Collector {
public:
    void sample(Snapshot& out, double dt_s) override;
};

// статические поля (hostname, kernel, cpu model) кэшируются в конструкторе
class SystemCollector final : public Collector {
public:
    SystemCollector();
    void sample(Snapshot& out, double dt_s) override;

private:
    HostInfo cached_{};
};

// хранит предыдущие байтовые счётчики на интерфейс для вычисления bps
class NetworkCollector final : public Collector {
public:
    void sample(Snapshot& out, double dt_s) override;

private:
    struct Prev {
        std::uint64_t rx = 0;
        std::uint64_t tx = 0;
    };
    std::unordered_map<std::string, Prev> prev_;
    bool primed_ = false;
};

// размер сектора принят 512 б -- соглашение ядра Linux
class DiskCollector final : public Collector {
public:
    void sample(Snapshot& out, double dt_s) override;

private:
    struct Prev {
        std::uint64_t read_sectors = 0;
        std::uint64_t write_sectors = 0;
    };
    std::unordered_map<std::string, Prev> prev_;
    bool primed_ = false;
};

// для delta cpu% нужны предыдущие (utime+stime) и суммарные jiffies системы
class ProcessCollector final : public Collector {
public:
    void sample(Snapshot& out, double dt_s) override;

private:
    struct Prev {
        std::uint64_t total_ticks = 0;
        std::uint64_t starttime = 0;  // защита от переиспользования pid
    };
    std::unordered_map<std::int32_t, Prev> prev_;
    std::uint64_t prev_cpu_total_jiffies_ = 0;
    bool primed_ = false;
};

}  // namespace yadr
