#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "yadr/collectors.hpp"
#include "yadr/sampler.hpp"
#include "yadr/server.hpp"

namespace {

struct CliOpts {
    std::string bind = "127.0.0.1";
    std::uint16_t port = 8080;
    int interval_ms = 1000;
    std::string web_root = "./web";
};

void print_usage(const char* argv0) {
    std::printf(
        "Usage: %s [options]\n"
        "  --bind <addr>      bind address (default 127.0.0.1; use 0.0.0.0 for LAN)\n"
        "  --port <n>         TCP port (default 8080)\n"
        "  --interval <ms>    sampling interval in ms (default 1000, min 250)\n"
        "  --web-root <dir>   directory with the built frontend (default ./web)\n"
        "  -h, --help         show this help\n",
        argv0);
}

bool parse_args(int argc, char** argv, CliOpts& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string_view a = argv[i];
        auto next = [&](std::string_view name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "yadr: %s requires an argument\n", std::string(name).c_str());
                return nullptr;
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (a == "--bind") {
            if (auto v = next(a)) opts.bind = v; else return false;
        } else if (a == "--port") {
            if (auto v = next(a)) opts.port = static_cast<std::uint16_t>(std::atoi(v)); else return false;
        } else if (a == "--interval") {
            if (auto v = next(a)) opts.interval_ms = std::atoi(v); else return false;
        } else if (a == "--web-root") {
            if (auto v = next(a)) opts.web_root = v; else return false;
        } else {
            std::fprintf(stderr, "yadr: unknown argument: %s\n", std::string(a).c_str());
            print_usage(argv[0]);
            return false;
        }
    }
    if (opts.interval_ms < 250) opts.interval_ms = 250;
    return true;
}

yadr::Server* g_server = nullptr;

void signal_handler(int) {
    if (g_server) g_server->stop();
}

}  // namespace

int main(int argc, char** argv) {
    CliOpts opts;
    if (!parse_args(argc, argv, opts)) return 2;

    yadr::Sampler::BroadcastFn broadcast;
    auto sampler = std::make_unique<yadr::Sampler>(std::chrono::milliseconds(opts.interval_ms),
                                                   [&broadcast](auto snap) {
                                                       if (broadcast) broadcast(std::move(snap));
                                                   });

    // system-коллектор первым -- остальные читают host.num_cpus и memory.total из этого тика
    sampler->add_collector(std::make_unique<yadr::SystemCollector>());
    sampler->add_collector(std::make_unique<yadr::MemoryCollector>());
    sampler->add_collector(std::make_unique<yadr::CpuCollector>());
    sampler->add_collector(std::make_unique<yadr::NetworkCollector>());
    sampler->add_collector(std::make_unique<yadr::DiskCollector>());
    sampler->add_collector(std::make_unique<yadr::ProcessCollector>());

    yadr::ServerConfig scfg;
    scfg.bind = opts.bind;
    scfg.port = opts.port;
    scfg.web_root = opts.web_root;
    yadr::Server server(std::move(scfg), *sampler);
    g_server = &server;

    broadcast = [&server](auto snap) { server.broadcast(std::move(snap)); };

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);

    sampler->start();
    std::printf("yadr-server listening on http://%s:%u  (interval=%d ms, web_root=%s)\n",
                opts.bind.c_str(), opts.port, opts.interval_ms, opts.web_root.c_str());
    server.run();
    sampler->stop();
    return 0;
}
