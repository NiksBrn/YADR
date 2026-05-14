#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "yadr/collector.hpp"
#include "yadr/snapshot.hpp"

namespace yadr {

class Sampler {
public:
    using SnapshotPtr = std::shared_ptr<const Snapshot>;
    using BroadcastFn = std::function<void(SnapshotPtr)>;

    Sampler(std::chrono::milliseconds interval, BroadcastFn on_sample);
    ~Sampler();

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    void add_collector(std::unique_ptr<Collector> c);
    void start();
    void stop();  // идемпотентен
    SnapshotPtr latest() const;

private:
    void run_();

    std::chrono::milliseconds interval_;
    BroadcastFn on_sample_;
    std::vector<std::unique_ptr<Collector>> collectors_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    // std::atomic<shared_ptr> требует C++20 -- используем для lock-free публикации снимка
    mutable std::atomic<std::shared_ptr<const Snapshot>> latest_;
};

}  // namespace yadr
