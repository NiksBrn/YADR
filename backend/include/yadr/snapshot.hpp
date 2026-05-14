#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace yadr {

// поднять при изменении wire-формата json
constexpr int kSchemaVersion = 1;

struct HostInfo {
    std::string hostname;
    std::string kernel;
    std::string cpu_model;
    std::uint32_t num_cpus = 0;
    double uptime_s = 0.0;
};

struct LoadAvg {
    double avg1 = 0.0;
    double avg5 = 0.0;
    double avg15 = 0.0;
};

struct CpuStats {
    double total = 0.0;        // суммарная загрузка 0..100%
    std::vector<double> per_core;  // загрузка по ядрам, 0..100%
};

struct MemoryStats {
    std::uint64_t total = 0;
    std::uint64_t available = 0;  // MemAvailable -- предпочтительная метрика свободной памяти
    std::uint64_t free = 0;
    std::uint64_t buffers = 0;
    std::uint64_t cached = 0;
    std::uint64_t used = 0;       // total - available
    std::uint64_t swap_total = 0;
    std::uint64_t swap_free = 0;
    std::uint64_t swap_used = 0;
};

struct ProcessInfo {
    std::int32_t pid = 0;
    std::int32_t ppid = 0;
    std::string user;
    char state = '?';      // R/S/D/Z/T/I -- см. proc(5)
    double cpu_pct = 0.0;  // 0..100 * num_cpus (top-style)
    double mem_pct = 0.0;  // доля RSS от MemTotal
    std::uint64_t vsize = 0;
    std::uint64_t rss = 0;
    std::uint64_t total_time_s = 0;
    std::string cmd;
};

struct NetInterface {
    std::string name;
    std::uint64_t rx_bytes = 0;
    std::uint64_t tx_bytes = 0;
    double rx_bps = 0.0;
    double tx_bps = 0.0;
};

struct DiskDevice {
    std::string name;
    std::uint64_t read_bytes = 0;
    std::uint64_t write_bytes = 0;
    double read_bps = 0.0;
    double write_bps = 0.0;
};

struct Snapshot {
    int schema = kSchemaVersion;
    std::int64_t ts_ms = 0;     // unix-время в миллисекундах
    bool warming_up = false;    // true на первом тике -- delta-метрики ещё не готовы
    HostInfo host;
    LoadAvg load;
    CpuStats cpu;
    MemoryStats memory;
    std::vector<ProcessInfo> processes;
    std::vector<NetInterface> network;
    std::vector<DiskDevice> disk;
};

nlohmann::json to_json(const Snapshot& s);

}  // namespace yadr
