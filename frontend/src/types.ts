// типы зеркалят C++ Snapshot -- держать в синхронизации с backend/include/yadr/snapshot.hpp

export interface HostInfo {
    hostname: string;
    kernel: string;
    cpu_model: string;
    num_cpus: number;
    uptime_s: number;
}

export interface LoadAvg {
    avg1: number;
    avg5: number;
    avg15: number;
}

export interface CpuStats {
    total: number;
    per_core: number[];
}

export interface MemoryStats {
    total: number;
    available: number;
    free: number;
    buffers: number;
    cached: number;
    used: number;
    swap_total: number;
    swap_free: number;
    swap_used: number;
}

export interface ProcessInfo {
    pid: number;
    ppid: number;
    user: string;
    state: string;
    cpu_pct: number;
    mem_pct: number;
    vsize: number;
    rss: number;
    total_time_s: number;
    cmd: string;
}

export interface NetInterface {
    name: string;
    rx_bytes: number;
    tx_bytes: number;
    rx_bps: number;
    tx_bps: number;
}

export interface DiskDevice {
    name: string;
    read_bytes: number;
    write_bytes: number;
    read_bps: number;
    write_bps: number;
}

export interface Snapshot {
    schema: number;
    ts_ms: number;
    warming_up: boolean;
    host: HostInfo;
    load: LoadAvg;
    cpu: CpuStats;
    memory: MemoryStats;
    processes: ProcessInfo[];
    network: NetInterface[];
    disk: DiskDevice[];
}
