#include "yadr/collectors.hpp"

#include <algorithm>

namespace yadr {

namespace {

double busy_pct(const proc::CpuTimes& prev, const proc::CpuTimes& cur) {
    // формула как в top: (1 - dIdle/dTotal) * 100
    // счётчики монотонны, но отключение ядра или сдвиг часов могут дать dtotal==0 -- зажимаем
    const std::uint64_t dtotal = cur.total() >= prev.total() ? cur.total() - prev.total() : 0;
    const std::uint64_t didle =
        cur.idle_all() >= prev.idle_all() ? cur.idle_all() - prev.idle_all() : 0;
    if (dtotal == 0) return 0.0;
    double busy = 1.0 - static_cast<double>(didle) / static_cast<double>(dtotal);
    if (busy < 0.0) busy = 0.0;
    if (busy > 1.0) busy = 1.0;
    return busy * 100.0;
}

}  // namespace

void CpuCollector::sample(Snapshot& out, double /*dt_s*/) {
    auto stat = proc::read_file("/proc/stat");
    if (stat) {
        std::vector<proc::CpuTimes> per_core;
        proc::CpuTimes total{};
        bool have_total = false;

        std::size_t pos = 0;
        while (pos < stat->size()) {
            std::size_t eol = stat->find('\n', pos);
            std::string_view line = std::string_view(*stat).substr(
                pos, eol == std::string::npos ? stat->size() - pos : eol - pos);
            pos = (eol == std::string::npos) ? stat->size() : eol + 1;

            if (line.rfind("cpu", 0) != 0) break;  // строки cpu идут первыми в /proc/stat
            auto parsed = proc::parse_cpu_line(line);
            if (!parsed) continue;
            // агрегатная строка -- ровно "cpu" (без цифры после), ядра -- "cpuN"
            const bool is_total = line.size() >= 3 && (line[3] == ' ' || line[3] == '\t');
            if (is_total) {
                total = *parsed;
                have_total = true;
            } else {
                per_core.push_back(*parsed);
            }
        }

        if (primed_ && have_total) {
            out.cpu.total = busy_pct(prev_total_, total);
            out.cpu.per_core.resize(per_core.size());
            for (std::size_t i = 0; i < per_core.size(); ++i) {
                const proc::CpuTimes& prev =
                    (i < prev_per_core_.size()) ? prev_per_core_[i] : proc::CpuTimes{};
                out.cpu.per_core[i] = busy_pct(prev, per_core[i]);
            }
        } else {
            out.cpu.total = 0.0;
            out.cpu.per_core.assign(per_core.size(), 0.0);
            out.warming_up = true;
        }
        prev_total_ = total;
        prev_per_core_ = std::move(per_core);
        primed_ = have_total;
    }

    if (auto la = proc::read_file("/proc/loadavg"); la) {
        auto fields = proc::split_ws(*la);
        if (fields.size() >= 3) {
            // stod вместо sscanf -- избегаем локальных pitfalls с разделителем
            auto to_d = [](std::string_view sv) {
                try {
                    return std::stod(std::string(sv));
                } catch (...) {
                    return 0.0;
                }
            };
            out.load.avg1 = to_d(fields[0]);
            out.load.avg5 = to_d(fields[1]);
            out.load.avg15 = to_d(fields[2]);
        }
    }
}

}  // namespace yadr
