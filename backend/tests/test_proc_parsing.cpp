/// @file test_proc_parsing.cpp
/// @brief Unit tests for the /proc parser helpers. Fixtures are inline strings so the
///        tests are hermetic and don't depend on the host's actual /proc contents.

#include <gtest/gtest.h>

#include "yadr/proc_utils.hpp"

using namespace yadr::proc;

TEST(SplitWs, BasicWhitespace) {
    auto v = split_ws("  one\t two  three\t\t ");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "one");
    EXPECT_EQ(v[1], "two");
    EXPECT_EQ(v[2], "three");
}

TEST(SplitWs, Empty) {
    EXPECT_TRUE(split_ws("").empty());
    EXPECT_TRUE(split_ws("   \t  ").empty());
}

TEST(KeyValue, MeminfoLikeFormat) {
    constexpr std::string_view sample =
        "MemTotal:       16331552 kB\n"
        "MemFree:         1024000 kB\n"
        "MemAvailable:    8000000 kB\n"
        "Buffers:           12345 kB\n"
        "Cached:          2000000 kB\n"
        "SwapTotal:       2097148 kB\n"
        "SwapFree:        2097148 kB\n";
    auto kv = parse_key_value(sample);
    EXPECT_EQ(kv["MemTotal"], "16331552 kB");
    EXPECT_EQ(kv["MemAvailable"], "8000000 kB");
    EXPECT_EQ(kv["SwapFree"], "2097148 kB");
}

TEST(KeyValue, StatusLikeFormat) {
    constexpr std::string_view sample =
        "Name:\tbash\n"
        "Umask:\t0022\n"
        "State:\tS (sleeping)\n"
        "Uid:\t1000\t1000\t1000\t1000\n";
    auto kv = parse_key_value(sample);
    EXPECT_EQ(kv["Name"], "bash");
    EXPECT_EQ(kv["State"], "S (sleeping)");
    EXPECT_EQ(kv["Uid"], "1000\t1000\t1000\t1000");
}

TEST(CpuLine, AggregateAndPerCpu) {
    auto agg = parse_cpu_line("cpu  100 20 300 5000 50 10 20 5 0 0");
    ASSERT_TRUE(agg.has_value());
    EXPECT_EQ(agg->user, 100u);
    EXPECT_EQ(agg->nice, 20u);
    EXPECT_EQ(agg->system, 300u);
    EXPECT_EQ(agg->idle, 5000u);
    EXPECT_EQ(agg->iowait, 50u);
    EXPECT_EQ(agg->irq, 10u);
    EXPECT_EQ(agg->softirq, 20u);
    EXPECT_EQ(agg->steal, 5u);
    EXPECT_EQ(agg->idle_all(), 5050u);
    EXPECT_EQ(agg->total(), 100u + 20u + 300u + 5000u + 50u + 10u + 20u + 5u);

    auto per = parse_cpu_line("cpu3 10 0 5 100 0 0 0 0");
    ASSERT_TRUE(per.has_value());
    EXPECT_EQ(per->user, 10u);
    EXPECT_EQ(per->idle, 100u);
}

TEST(CpuLine, NotACpuLine) {
    EXPECT_FALSE(parse_cpu_line("intr 1234").has_value());
    EXPECT_FALSE(parse_cpu_line("").has_value());
}

TEST(PidStat, SimpleCommNoSpaces) {
    constexpr std::string_view line =
        "1234 (bash) S 1000 1234 1234 34816 1234 4194304 200 0 0 0 "
        "150 75 10 5 20 0 1 0 9876543 12345678 4567 18446744073709551615 0 0 0 0 0 0 65538 3686404 1266761467 0 0 0 17 2 0 0 0 0 0 0 0 0 0 0 0 0 0";
    auto p = parse_pid_stat(line);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->pid, 1234);
    EXPECT_EQ(p->comm, "bash");
    EXPECT_EQ(p->state, 'S');
    EXPECT_EQ(p->ppid, 1000);
    EXPECT_EQ(p->utime, 150u);
    EXPECT_EQ(p->stime, 75u);
    EXPECT_EQ(p->starttime, 9876543u);
    EXPECT_EQ(p->vsize, 12345678u);
    EXPECT_EQ(p->rss_pages, 4567);
}

TEST(PidStat, CommWithSpacesAndParens) {
    // comm = "my (weird) process" — three spaces, embedded parens. Must be parsed by
    // finding the LAST ')', not the first.
    constexpr std::string_view line =
        "42 (my (weird) process) R 1 42 42 0 -1 0 1 0 0 0 "
        "5 2 0 0 20 0 1 0 100 200 50 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
    auto p = parse_pid_stat(line);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->pid, 42);
    EXPECT_EQ(p->comm, "my (weird) process");
    EXPECT_EQ(p->state, 'R');
    EXPECT_EQ(p->ppid, 1);
    EXPECT_EQ(p->utime, 5u);
    EXPECT_EQ(p->stime, 2u);
}

TEST(PidStat, RejectMalformed) {
    EXPECT_FALSE(parse_pid_stat("garbage").has_value());
    EXPECT_FALSE(parse_pid_stat("123 noparens R").has_value());
}
