#include "yadr/server.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>

#include <crow.h>

#include "yadr/snapshot.hpp"

namespace yadr {

namespace {

std::string ext_to_mime(std::string_view ext) {
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".js" || ext == ".mjs") return "application/javascript; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".map") return "application/json; charset=utf-8";
    if (ext == ".woff2") return "font/woff2";
    return "application/octet-stream";
}

// защита от path traversal: отклоняем пути вне web_root; при промахе отдаём index.html (SPA)
std::optional<std::filesystem::path> resolve_static(const std::filesystem::path& root,
                                                    std::string url_path) {
    if (url_path.empty() || url_path == "/") url_path = "/index.html";
    // filesystem::path / "/x" заменяет root -- срезаем ведущий слэш вручную
    while (!url_path.empty() && url_path.front() == '/') url_path.erase(0, 1);

    std::error_code ec;
    auto root_abs = std::filesystem::weakly_canonical(root, ec);
    if (ec) return std::nullopt;
    auto requested = std::filesystem::weakly_canonical(root_abs / url_path, ec);
    if (ec) return std::nullopt;

    auto root_str = root_abs.string();
    auto req_str = requested.string();
    auto inside = [&] {
        if (req_str.size() < root_str.size()) return false;
        if (req_str.compare(0, root_str.size(), root_str) != 0) return false;
        return req_str.size() == root_str.size() || req_str[root_str.size()] == '/';
    };
    if (!inside()) return std::nullopt;

    if (std::filesystem::is_regular_file(requested, ec)) return requested;
    // SPA fallback
    auto idx = root_abs / "index.html";
    if (std::filesystem::is_regular_file(idx, ec)) return idx;
    return std::nullopt;
}

crow::response serve_file(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return crow::response(404);
    std::ostringstream ss;
    ss << in.rdbuf();
    crow::response r{ss.str()};
    r.set_header("Content-Type", ext_to_mime(file.extension().string()));
    r.set_header("Cache-Control", "no-cache");
    return r;
}

}  // namespace

struct Server::Impl {
    ServerConfig cfg;
    crow::SimpleApp app;
    std::mutex clients_mu;
    std::unordered_set<crow::websocket::connection*> clients;
    std::atomic<bool> stopping{false};

    explicit Impl(ServerConfig c) : cfg(std::move(c)) {
        app.loglevel(crow::LogLevel::Warning);
    }
};

Server::Server(ServerConfig cfg, Sampler& sampler) : impl_(std::make_unique<Impl>(std::move(cfg))) {
    auto* impl = impl_.get();

    CROW_ROUTE(impl->app, "/api/snapshot")([impl, &sampler]() {
        auto snap = sampler.latest();
        if (!snap) {
            crow::response r(503, "snapshot not ready");
            return r;
        }
        crow::response r{to_json(*snap).dump()};
        r.set_header("Content-Type", "application/json; charset=utf-8");
        r.set_header("Cache-Control", "no-cache");
        return r;
    });

    CROW_ROUTE(impl->app, "/api/health")([]() {
        crow::response r{R"({"ok":true})"};
        r.set_header("Content-Type", "application/json; charset=utf-8");
        return r;
    });

    CROW_WEBSOCKET_ROUTE(impl->app, "/ws")
        .onopen([impl, &sampler](crow::websocket::connection& c) {
            {
                std::lock_guard<std::mutex> lk(impl->clients_mu);
                impl->clients.insert(&c);
            }
            if (auto s = sampler.latest()) c.send_text(to_json(*s).dump());
        })
        .onclose([impl](crow::websocket::connection& c, const std::string&) {
            std::lock_guard<std::mutex> lk(impl->clients_mu);
            impl->clients.erase(&c);
        })
        .onmessage([](crow::websocket::connection&, const std::string&, bool) {});

    CROW_ROUTE(impl->app, "/")([impl]() {
        if (auto f = resolve_static(impl->cfg.web_root, "/")) return serve_file(*f);
        return crow::response(404, "web root not found");
    });
    CROW_ROUTE(impl->app, "/<path>")([impl](const std::string& path) {
        if (auto f = resolve_static(impl->cfg.web_root, "/" + path)) return serve_file(*f);
        return crow::response(404);
    });

    (void)sampler;
}

Server::~Server() { stop(); }

void Server::run() {
    impl_->app.bindaddr(impl_->cfg.bind).port(impl_->cfg.port).multithreaded().run();
}

void Server::stop() {
    if (impl_->stopping.exchange(true)) return;
    impl_->app.stop();
}

void Server::broadcast(std::shared_ptr<const Snapshot> snap) {
    if (!snap) return;
    const std::string payload = to_json(*snap).dump();
    std::lock_guard<std::mutex> lk(impl_->clients_mu);
    for (auto* c : impl_->clients) {
        try {
            c->send_text(payload);
        } catch (...) {
            // ошибка будет обработана в onclose -- не кидаем из потока семплера
        }
    }
}

}  // namespace yadr
