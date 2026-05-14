#include "yadr/snapshot.hpp"

namespace yadr {

namespace {

nlohmann::json host_to_json(const HostInfo& h) {
    return {
        {"hostname", h.hostname}, {"kernel", h.kernel},     {"cpu_model", h.cpu_model},
        {"num_cpus", h.num_cpus}, {"uptime_s", h.uptime_s},
    };
}

nlohmann::json load_to_json(const LoadAvg& l) {
    return {{"avg1", l.avg1}, {"avg5", l.avg5}, {"avg15", l.avg15}};
}

nlohmann::json cpu_to_json(const CpuStats& c) {
    return {{"total", c.total}, {"per_core", c.per_core}};
}

nlohmann::json memory_to_json(const MemoryStats& m) {
    return {
        {"total", m.total},           {"available", m.available},   {"free", m.free},
        {"buffers", m.buffers},       {"cached", m.cached},         {"used", m.used},
        {"swap_total", m.swap_total}, {"swap_free", m.swap_free},   {"swap_used", m.swap_used},
    };
}

nlohmann::json process_to_json(const ProcessInfo& p) {
    return {
        {"pid", p.pid},           {"ppid", p.ppid}, {"user", p.user},
        {"state", std::string(1, p.state)},         {"cpu_pct", p.cpu_pct},
        {"mem_pct", p.mem_pct},   {"vsize", p.vsize}, {"rss", p.rss},
        {"total_time_s", p.total_time_s}, {"cmd", p.cmd},
    };
}

nlohmann::json net_to_json(const NetInterface& n) {
    return {{"name", n.name},
            {"rx_bytes", n.rx_bytes},
            {"tx_bytes", n.tx_bytes},
            {"rx_bps", n.rx_bps},
            {"tx_bps", n.tx_bps}};
}

nlohmann::json disk_to_json(const DiskDevice& d) {
    return {{"name", d.name},
            {"read_bytes", d.read_bytes},
            {"write_bytes", d.write_bytes},
            {"read_bps", d.read_bps},
            {"write_bps", d.write_bps}};
}

}  // namespace

nlohmann::json to_json(const Snapshot& s) {
    nlohmann::json procs = nlohmann::json::array();
    for (const auto& p : s.processes) procs.push_back(process_to_json(p));

    nlohmann::json nets = nlohmann::json::array();
    for (const auto& n : s.network) nets.push_back(net_to_json(n));

    nlohmann::json disks = nlohmann::json::array();
    for (const auto& d : s.disk) disks.push_back(disk_to_json(d));

    return {
        {"schema", s.schema},     {"ts_ms", s.ts_ms},        {"warming_up", s.warming_up},
        {"host", host_to_json(s.host)}, {"load", load_to_json(s.load)},
        {"cpu", cpu_to_json(s.cpu)},    {"memory", memory_to_json(s.memory)},
        {"processes", std::move(procs)}, {"network", std::move(nets)},
        {"disk", std::move(disks)},
    };
}

}  // namespace yadr
