/// @file test_to_json.cpp
/// @brief Snapshot ↔ JSON contract tests. Frontend depends on these exact field names.

#include <gtest/gtest.h>

#include "yadr/snapshot.hpp"

TEST(ToJson, SnapshotShape) {
    yadr::Snapshot s;
    s.ts_ms = 1715515200000;
    s.host.hostname = "testbox";
    s.host.kernel = "Linux 6.5.0";
    s.host.cpu_model = "TestCPU";
    s.host.num_cpus = 4;
    s.host.uptime_s = 3600.5;
    s.load = {0.5, 1.0, 1.5};
    s.cpu.total = 42.0;
    s.cpu.per_core = {10.0, 20.0, 30.0, 40.0};
    s.memory.total = 16ull * 1024 * 1024 * 1024;
    s.memory.used = 4ull * 1024 * 1024 * 1024;
    s.processes.push_back({1, 0, "root", 'S', 0.5, 0.1, 1000, 500, 12, "/sbin/init"});
    s.network.push_back({"lo", 1234, 5678, 10.0, 20.0});
    s.disk.push_back({"sda", 4096, 8192, 100.0, 200.0});

    auto j = yadr::to_json(s);

    EXPECT_EQ(j["schema"].get<int>(), yadr::kSchemaVersion);
    EXPECT_EQ(j["ts_ms"].get<std::int64_t>(), 1715515200000);
    EXPECT_EQ(j["host"]["hostname"], "testbox");
    EXPECT_EQ(j["host"]["num_cpus"], 4);
    EXPECT_EQ(j["load"]["avg5"].get<double>(), 1.0);
    EXPECT_EQ(j["cpu"]["per_core"].size(), 4u);
    EXPECT_EQ(j["memory"]["total"].get<std::uint64_t>(), s.memory.total);
    ASSERT_EQ(j["processes"].size(), 1u);
    EXPECT_EQ(j["processes"][0]["pid"], 1);
    EXPECT_EQ(j["processes"][0]["user"], "root");
    EXPECT_EQ(j["processes"][0]["state"], "S");
    ASSERT_EQ(j["network"].size(), 1u);
    EXPECT_EQ(j["network"][0]["name"], "lo");
    ASSERT_EQ(j["disk"].size(), 1u);
    EXPECT_EQ(j["disk"][0]["name"], "sda");
}

TEST(ToJson, EmptySnapshotIsValid) {
    yadr::Snapshot s;
    auto j = yadr::to_json(s);
    EXPECT_TRUE(j.contains("schema"));
    EXPECT_TRUE(j["processes"].is_array());
    EXPECT_TRUE(j["network"].is_array());
    EXPECT_TRUE(j["disk"].is_array());
    EXPECT_EQ(j["processes"].size(), 0u);
}
