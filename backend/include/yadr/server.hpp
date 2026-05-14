#pragma once

#include <memory>
#include <string>

#include "yadr/sampler.hpp"

namespace yadr {

struct ServerConfig {
    std::string bind = "127.0.0.1";
    std::uint16_t port = 8080;
    std::string web_root = "./web";  // SPA с fallback на index.html
};

class Server {
public:
    Server(ServerConfig cfg, Sampler& sampler);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void stop();  // безопасно вызывать из обработчика сигнала
    void broadcast(std::shared_ptr<const Snapshot> snap);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace yadr
