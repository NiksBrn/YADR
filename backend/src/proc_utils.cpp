#include "yadr/proc_utils.hpp"

#include <pwd.h>
#include <unistd.h>

#include <charconv>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

namespace yadr::proc {

namespace {

std::string_view trim(std::string_view s) {
    auto is_ws = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && is_ws(s.front())) s.remove_prefix(1);
    while (!s.empty() && is_ws(s.back())) s.remove_suffix(1);
    return s;
}

template <class T>
bool to_number(std::string_view sv, T& out) {
    sv = trim(sv);
    if (sv.empty()) return false;
    auto [p, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    return ec == std::errc{};
}

}  // namespace

std::optional<std::string> read_file(const std::string& path) {
    // ifstream нормально работает с /proc -- одно чтение даёт консистентный снимок
    std::ifstream in(path);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::string_view> split_ws(std::string_view s) {
    std::vector<std::string_view> out;
    std::size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t')) ++i;
        std::size_t start = i;
        while (i < n && s[i] != ' ' && s[i] != '\t') ++i;
        if (start < i) out.emplace_back(s.substr(start, i - start));
    }
    return out;
}

std::unordered_map<std::string, std::string> parse_key_value(std::string_view text) {
    std::unordered_map<std::string, std::string> out;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t eol = text.find('\n', pos);
        std::string_view line =
            text.substr(pos, eol == std::string_view::npos ? text.size() - pos : eol - pos);
        pos = (eol == std::string_view::npos) ? text.size() : eol + 1;

        if (line.empty()) continue;
        // принимаем оба формата: "key:" и "key<пробел>"
        std::size_t sep = line.find(':');
        if (sep == std::string_view::npos) {
            sep = line.find_first_of(" \t");
            if (sep == std::string_view::npos) continue;
        }
        std::string key{trim(line.substr(0, sep))};
        std::string val{trim(line.substr(sep + 1))};
        if (!key.empty()) out.emplace(std::move(key), std::move(val));
    }
    return out;
}

std::optional<PidStat> parse_pid_stat(std::string_view line) {
    // формат: "<pid> (<comm>) <state> <ppid> ..." -- comm может содержать пробелы и ')'
    // ищем последнюю ')' чтобы правильно закончить поле comm
    std::size_t lp = line.find('(');
    std::size_t rp = line.rfind(')');
    if (lp == std::string_view::npos || rp == std::string_view::npos || rp <= lp) {
        return std::nullopt;
    }

    PidStat ps;
    if (!to_number(line.substr(0, lp), ps.pid)) return std::nullopt;
    ps.comm = std::string(line.substr(lp + 1, rp - lp - 1));

    auto rest = trim(line.substr(rp + 1));
    auto fields = split_ws(rest);
    if (fields.size() < 22) return std::nullopt;

    ps.state = fields[0].empty() ? '?' : fields[0][0];
    if (!to_number(fields[1], ps.ppid)) return std::nullopt;
    if (!to_number(fields[11], ps.utime)) return std::nullopt;
    if (!to_number(fields[12], ps.stime)) return std::nullopt;
    if (!to_number(fields[19], ps.starttime)) return std::nullopt;
    if (!to_number(fields[20], ps.vsize)) return std::nullopt;
    if (!to_number(fields[21], ps.rss_pages)) return std::nullopt;
    return ps;
}

std::uint64_t CpuTimes::total() const noexcept {
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

std::uint64_t CpuTimes::idle_all() const noexcept { return idle + iowait; }

std::optional<CpuTimes> parse_cpu_line(std::string_view line) {
    auto fields = split_ws(line);
    if (fields.size() < 5) return std::nullopt;
    if (fields[0].substr(0, 3) != "cpu") return std::nullopt;

    CpuTimes t;
    auto take = [&](std::size_t i, std::uint64_t& dst) {
        if (i < fields.size()) to_number(fields[i], dst);
    };
    take(1, t.user);
    take(2, t.nice);
    take(3, t.system);
    take(4, t.idle);
    take(5, t.iowait);
    take(6, t.irq);
    take(7, t.softirq);
    take(8, t.steal);
    return t;
}

std::string resolve_username(std::uint32_t uid) {
    static std::mutex m;
    static std::unordered_map<std::uint32_t, std::string> cache;
    {
        std::lock_guard<std::mutex> lk(m);
        if (auto it = cache.find(uid); it != cache.end()) return it->second;
    }
    std::string name;
    // getpwuid не потокобезопасен -- используем getpwuid_r с буфером под максимальный размер
    long buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buflen <= 0) buflen = 16384;
    std::string buf(static_cast<std::size_t>(buflen), '\0');
    passwd pwd{};
    passwd* result = nullptr;
    if (getpwuid_r(uid, &pwd, buf.data(), buf.size(), &result) == 0 && result) {
        name = result->pw_name;
    } else {
        name = std::to_string(uid);
    }
    std::lock_guard<std::mutex> lk(m);
    cache.emplace(uid, name);
    return name;
}

long clock_ticks_per_sec() {
    static const long v = sysconf(_SC_CLK_TCK);
    return v > 0 ? v : 100;
}

long page_size_bytes() {
    static const long v = sysconf(_SC_PAGESIZE);
    return v > 0 ? v : 4096;
}

}  // namespace yadr::proc
