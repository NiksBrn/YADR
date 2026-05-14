#include "yadr/collectors.hpp"

namespace yadr {

namespace {

// /proc/meminfo пишет "kB", но имеет в виду KiB -- конвертируем в байты
std::uint64_t kib_to_bytes(const std::string& v) {
    std::uint64_t n = 0;
    for (char c : v) {
        if (c >= '0' && c <= '9') {
            n = n * 10 + static_cast<std::uint64_t>(c - '0');
        } else if (c == ' ') {
            break;
        }
    }
    return n * 1024ull;
}

}  // namespace

void MemoryCollector::sample(Snapshot& out, double /*dt_s*/) {
    auto text = proc::read_file("/proc/meminfo");
    if (!text) return;

    auto kv = proc::parse_key_value(*text);
    auto get = [&](const char* key) -> std::uint64_t {
        auto it = kv.find(key);
        return it == kv.end() ? 0ull : kib_to_bytes(it->second);
    };

    auto& m = out.memory;
    m.total = get("MemTotal");
    m.free = get("MemFree");
    m.available = get("MemAvailable");
    m.buffers = get("Buffers");
    m.cached = get("Cached");
    // "used" как в free(1)/htop: total минус available (подсказка ядра о реально свободном)
    m.used = (m.total > m.available) ? (m.total - m.available) : 0;
    m.swap_total = get("SwapTotal");
    m.swap_free = get("SwapFree");
    m.swap_used = (m.swap_total > m.swap_free) ? (m.swap_total - m.swap_free) : 0;
}

}  // namespace yadr
