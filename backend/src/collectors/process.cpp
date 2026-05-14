#include "yadr/collectors.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstring>

namespace yadr {

namespace {

bool is_pid_name(const char* name) {
    if (!name || !*name) return false;
    for (const char* p = name; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return true;
}

std::uint64_t read_total_cpu_jiffies() {
    auto stat = proc::read_file("/proc/stat");
    if (!stat) return 0;
    std::size_t eol = stat->find('\n');
    std::string_view line = std::string_view(*stat).substr(0, eol);
    auto t = proc::parse_cpu_line(line);
    return t ? t->total() : 0;
}

std::unordered_map<std::string, std::string> read_status(int pid) {
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/status", pid);
    auto text = proc::read_file(path);
    if (!text) return {};
    return proc::parse_key_value(*text);
}

std::string read_cmdline(int pid, const std::string& comm) {
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    auto text = proc::read_file(path);
    if (!text || text->empty() || text->front() == '\0') {
        return "[" + comm + "]";
    }
    // argv в /proc/.../cmdline разделены NUL-байтами -- заменяем на пробелы
    std::string out;
    out.reserve(text->size());
    for (char c : *text) {
        if (c == '\0') {
            if (!out.empty() && out.back() != ' ') out.push_back(' ');
        } else {
            out.push_back(c);
        }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::uint32_t parse_uid(const std::string& uid_field) {
    // поле "Uid:" имеет 4 значения: real, effective, saved, fs -- берём первое (real uid)
    std::uint32_t uid = 0;
    std::size_t i = 0;
    while (i < uid_field.size() && (uid_field[i] == ' ' || uid_field[i] == '\t')) ++i;
    while (i < uid_field.size() && uid_field[i] >= '0' && uid_field[i] <= '9') {
        uid = uid * 10 + static_cast<std::uint32_t>(uid_field[i] - '0');
        ++i;
    }
    return uid;
}

}  // namespace

void ProcessCollector::sample(Snapshot& out, double /*dt_s*/) {
    DIR* dir = opendir("/proc");
    if (!dir) return;

    const std::uint64_t cur_cpu_total = read_total_cpu_jiffies();
    const std::uint64_t dcpu = cur_cpu_total >= prev_cpu_total_jiffies_
                                   ? cur_cpu_total - prev_cpu_total_jiffies_
                                   : 0;
    const long ticks_per_sec = proc::clock_ticks_per_sec();
    const long page_sz = proc::page_size_bytes();
    const double ncpu = out.host.num_cpus > 0 ? static_cast<double>(out.host.num_cpus) : 1.0;
    const std::uint64_t mem_total = out.memory.total;

    std::vector<ProcessInfo> procs;
    std::unordered_map<std::int32_t, Prev> next_prev;
    procs.reserve(512);

    dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        if (!is_pid_name(ent->d_name)) continue;
        int pid = std::atoi(ent->d_name);
        if (pid <= 0) continue;

        char path[64];
        std::snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        auto stat_text = proc::read_file(path);
        if (!stat_text) continue;  // процесс завершился между readdir и чтением
        auto pidstat = proc::parse_pid_stat(*stat_text);
        if (!pidstat) continue;

        auto status = read_status(pid);

        ProcessInfo pi;
        pi.pid = pidstat->pid;
        pi.ppid = pidstat->ppid;
        pi.state = pidstat->state;
        pi.vsize = pidstat->vsize;
        pi.rss = static_cast<std::uint64_t>(pidstat->rss_pages) *
                 static_cast<std::uint64_t>(page_sz);
        pi.total_time_s =
            (pidstat->utime + pidstat->stime) / static_cast<std::uint64_t>(ticks_per_sec);

        const std::uint64_t total_ticks = pidstat->utime + pidstat->stime;
        if (primed_ && dcpu > 0) {
            auto it = prev_.find(pid);
            // starttime проверяем чтобы не смешивать тики разных процессов с одним pid
            if (it != prev_.end() && it->second.starttime == pidstat->starttime) {
                const std::uint64_t dticks = total_ticks >= it->second.total_ticks
                                                 ? total_ticks - it->second.total_ticks
                                                 : 0;
                // top-style: 100 * (delta ticks процесса) / (delta total ticks) * ncpu
                pi.cpu_pct =
                    100.0 * static_cast<double>(dticks) / static_cast<double>(dcpu) * ncpu;
                if (pi.cpu_pct < 0.0) pi.cpu_pct = 0.0;
            }
        }
        next_prev.emplace(pid, Prev{total_ticks, pidstat->starttime});

        if (mem_total > 0) {
            pi.mem_pct = 100.0 * static_cast<double>(pi.rss) / static_cast<double>(mem_total);
        }

        if (auto it = status.find("Uid"); it != status.end()) {
            pi.user = proc::resolve_username(parse_uid(it->second));
        } else {
            pi.user = "?";
        }
        pi.cmd = read_cmdline(pid, pidstat->comm);
        procs.push_back(std::move(pi));
    }
    closedir(dir);

    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        if (a.cpu_pct != b.cpu_pct) return a.cpu_pct > b.cpu_pct;
        return a.pid < b.pid;
    });

    out.processes = std::move(procs);
    prev_ = std::move(next_prev);
    prev_cpu_total_jiffies_ = cur_cpu_total;
    primed_ = true;
}

}  // namespace yadr
