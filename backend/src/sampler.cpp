#include "yadr/sampler.hpp"

#include <chrono>

namespace yadr {

Sampler::Sampler(std::chrono::milliseconds interval, BroadcastFn on_sample)
    : interval_(interval), on_sample_(std::move(on_sample)) {}

Sampler::~Sampler() { stop(); }

void Sampler::add_collector(std::unique_ptr<Collector> c) {
    collectors_.push_back(std::move(c));
}

void Sampler::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread(&Sampler::run_, this);
}

void Sampler::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

Sampler::SnapshotPtr Sampler::latest() const { return latest_.load(); }

void Sampler::run_() {
    using clock = std::chrono::steady_clock;
    auto next = clock::now();
    auto last = clock::now();

    while (running_.load()) {
        const auto now = clock::now();
        const double dt_s = std::chrono::duration<double>(now - last).count();
        last = now;

        auto snap = std::make_shared<Snapshot>();
        snap->ts_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

        for (auto& c : collectors_) {
            c->sample(*snap, dt_s);
        }

        SnapshotPtr published(std::move(snap));
        latest_.store(published);
        if (on_sample_) on_sample_(published);

        // планируем следующий тик относительно предыдущего дедлайна -- без накопления дрейфа
        next += interval_;
        const auto t = clock::now();
        if (next <= t) {
            // отстали (медленный коллектор) -- сбрасываем дедлайн вместо busy-loop
            next = t + interval_;
        } else {
            std::this_thread::sleep_for(next - t);
        }
    }
}

}  // namespace yadr
