#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace yadr::proc {

// файлы /proc виртуальные -- одно read() возвращает консистентный снимок
std::optional<std::string> read_file(const std::string& path);

std::vector<std::string_view> split_ws(std::string_view s);

std::unordered_map<std::string, std::string> parse_key_value(std::string_view text);

// comm в /proc/[pid]/stat может содержать пробелы и скобки -- ищем последнюю ')'
struct PidStat {
    std::int32_t pid = 0;
    std::string comm;
    char state = '?';
    std::int32_t ppid = 0;
    std::uint64_t utime = 0;
    std::uint64_t stime = 0;
    std::uint64_t starttime = 0;  // тики с момента загрузки -- для защиты от reuse pid
    std::uint64_t vsize = 0;
    std::int64_t rss_pages = 0;   // страниц (умножить на page_size_bytes)
};
std::optional<PidStat> parse_pid_stat(std::string_view line);

struct CpuTimes {
    std::uint64_t user = 0;
    std::uint64_t nice = 0;
    std::uint64_t system = 0;
    std::uint64_t idle = 0;
    std::uint64_t iowait = 0;
    std::uint64_t irq = 0;
    std::uint64_t softirq = 0;
    std::uint64_t steal = 0;

    std::uint64_t total() const noexcept;
    std::uint64_t idle_all() const noexcept;  // idle + iowait -- то, что top считает простоем
};
std::optional<CpuTimes> parse_cpu_line(std::string_view line);

std::string resolve_username(std::uint32_t uid);

long clock_ticks_per_sec();
long page_size_bytes();

}  // namespace yadr::proc
