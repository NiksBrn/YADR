#include "yadr/collectors.hpp"

#include <algorithm>

namespace yadr {

void NetworkCollector::sample(Snapshot& out, double dt_s) {
    auto text = proc::read_file("/proc/net/dev");
    if (!text) return;

    std::vector<NetInterface> ifaces;
    std::size_t pos = 0;
    int line_no = 0;
    while (pos < text->size()) {
        std::size_t eol = text->find('\n', pos);
        std::string_view line = std::string_view(*text).substr(
            pos, eol == std::string::npos ? text->size() - pos : eol - pos);
        pos = (eol == std::string::npos) ? text->size() : eol + 1;
        if (++line_no <= 2) continue;  // пропускаем две строки заголовка

        std::size_t colon = line.find(':');
        if (colon == std::string_view::npos) continue;
        std::string name(line.substr(0, colon));
        std::size_t s = name.find_first_not_of(" \t");
        if (s != std::string::npos) name = name.substr(s);

        auto fields = proc::split_ws(line.substr(colon + 1));
        if (fields.size() < 16) continue;
        NetInterface iface;
        iface.name = std::move(name);
        // fields[0]=rx_bytes, fields[8]=tx_bytes
        auto to_u64 = [](std::string_view sv) -> std::uint64_t {
            std::uint64_t v = 0;
            for (char c : sv) {
                if (c < '0' || c > '9') break;
                v = v * 10 + static_cast<std::uint64_t>(c - '0');
            }
            return v;
        };
        iface.rx_bytes = to_u64(fields[0]);
        iface.tx_bytes = to_u64(fields[8]);

        if (primed_ && dt_s > 0.0) {
            auto it = prev_.find(iface.name);
            if (it != prev_.end()) {
                const auto drx = iface.rx_bytes >= it->second.rx ? iface.rx_bytes - it->second.rx
                                                                 : 0ull;
                const auto dtx = iface.tx_bytes >= it->second.tx ? iface.tx_bytes - it->second.tx
                                                                 : 0ull;
                iface.rx_bps = static_cast<double>(drx) / dt_s;
                iface.tx_bps = static_cast<double>(dtx) / dt_s;
            }
        }
        prev_[iface.name] = {iface.rx_bytes, iface.tx_bytes};
        ifaces.push_back(std::move(iface));
    }

    std::sort(ifaces.begin(), ifaces.end(),
              [](const NetInterface& a, const NetInterface& b) { return a.name < b.name; });
    out.network = std::move(ifaces);
    primed_ = true;
}

}  // namespace yadr
