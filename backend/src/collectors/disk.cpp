#include "yadr/collectors.hpp"

#include <algorithm>
#include <string_view>

namespace yadr {

namespace {

// отфильтровываем псевдоустройства и разделы -- оставляем только целые диски
bool include_device(std::string_view name) {
    if (name.rfind("loop", 0) == 0) return false;
    if (name.rfind("ram", 0) == 0) return false;
    if (name.rfind("zram", 0) == 0) return false;
    // nvme-разделы (nvme0n1p1) пропускаем, целый диск (nvme0n1) оставляем
    if (name.rfind("nvme", 0) == 0) {
        auto p = name.find('p');
        if (p != std::string_view::npos && p + 1 < name.size() &&
            name[p + 1] >= '0' && name[p + 1] <= '9') {
            return false;
        }
        return true;
    }
    // sd*/hd*/vd*/xvd* -- целые диски оканчиваются на букву, разделы -- на цифру
    if (!name.empty() && (name.rfind("sd", 0) == 0 || name.rfind("hd", 0) == 0 ||
                          name.rfind("vd", 0) == 0 || name.rfind("xvd", 0) == 0)) {
        char last = name.back();
        return !(last >= '0' && last <= '9');
    }
    // mmcblk -- без суффикса 'p' это целое устройство
    if (name.rfind("mmcblk", 0) == 0) {
        return name.find('p') == std::string_view::npos;
    }
    return false;
}

}  // namespace

void DiskCollector::sample(Snapshot& out, double dt_s) {
    auto text = proc::read_file("/proc/diskstats");
    if (!text) return;

    constexpr std::uint64_t kSectorSize = 512;
    std::vector<DiskDevice> devs;
    std::size_t pos = 0;
    while (pos < text->size()) {
        std::size_t eol = text->find('\n', pos);
        std::string_view line = std::string_view(*text).substr(
            pos, eol == std::string::npos ? text->size() - pos : eol - pos);
        pos = (eol == std::string::npos) ? text->size() : eol + 1;
        auto fields = proc::split_ws(line);
        if (fields.size() < 10) continue;
        std::string name(fields[2]);
        if (!include_device(name)) continue;

        auto to_u64 = [](std::string_view sv) -> std::uint64_t {
            std::uint64_t v = 0;
            for (char c : sv) {
                if (c < '0' || c > '9') break;
                v = v * 10 + static_cast<std::uint64_t>(c - '0');
            }
            return v;
        };
        const auto rsect = to_u64(fields[5]);
        const auto wsect = to_u64(fields[9]);

        DiskDevice d;
        d.name = name;
        d.read_bytes = rsect * kSectorSize;
        d.write_bytes = wsect * kSectorSize;

        if (primed_ && dt_s > 0.0) {
            auto it = prev_.find(name);
            if (it != prev_.end()) {
                const auto drs = rsect >= it->second.read_sectors
                                     ? rsect - it->second.read_sectors
                                     : 0ull;
                const auto dws = wsect >= it->second.write_sectors
                                     ? wsect - it->second.write_sectors
                                     : 0ull;
                d.read_bps = static_cast<double>(drs * kSectorSize) / dt_s;
                d.write_bps = static_cast<double>(dws * kSectorSize) / dt_s;
            }
        }
        prev_[name] = {rsect, wsect};
        devs.push_back(std::move(d));
    }

    std::sort(devs.begin(), devs.end(),
              [](const DiskDevice& a, const DiskDevice& b) { return a.name < b.name; });
    out.disk = std::move(devs);
    primed_ = true;
}

}  // namespace yadr
