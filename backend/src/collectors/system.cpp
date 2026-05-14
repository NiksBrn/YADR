#include "yadr/collectors.hpp"

#include <sys/utsname.h>
#include <unistd.h>

namespace yadr {

namespace {

std::string read_cpu_model() {
    auto text = proc::read_file("/proc/cpuinfo");
    if (!text) return "unknown";
    // в /proc/cpuinfo по блоку на каждое логическое ядро -- берём первое "model name"
    std::size_t pos = 0;
    while (pos < text->size()) {
        std::size_t eol = text->find('\n', pos);
        std::string_view line = std::string_view(*text).substr(
            pos, eol == std::string::npos ? text->size() - pos : eol - pos);
        pos = (eol == std::string::npos) ? text->size() : eol + 1;
        if (line.rfind("model name", 0) == 0) {
            std::size_t c = line.find(':');
            if (c != std::string_view::npos) {
                auto v = line.substr(c + 1);
                while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.remove_prefix(1);
                return std::string(v);
            }
        }
    }
    return "unknown";
}

double read_uptime() {
    auto text = proc::read_file("/proc/uptime");
    if (!text) return 0.0;
    try {
        return std::stod(*text);
    } catch (...) {
        return 0.0;
    }
}

}  // namespace

SystemCollector::SystemCollector() {
    utsname u{};
    if (uname(&u) == 0) {
        cached_.hostname = u.nodename;
        cached_.kernel = std::string(u.sysname) + " " + u.release;
    } else {
        cached_.hostname = "localhost";
        cached_.kernel = "Linux";
    }
    cached_.cpu_model = read_cpu_model();
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    cached_.num_cpus = n > 0 ? static_cast<std::uint32_t>(n) : 1u;
}

void SystemCollector::sample(Snapshot& out, double /*dt_s*/) {
    out.host = cached_;
    out.host.uptime_s = read_uptime();
}

}  // namespace yadr
