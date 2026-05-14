import type { Snapshot } from '../types';
import { duration } from '../format';

const root = document.getElementById('host-info') as HTMLElement;

export function renderHeader(s: Snapshot): void {
    const { host, load } = s;
    root.innerHTML = `
        <b>${host.hostname}</b> · ${host.kernel} ·
        ${host.cpu_model} <span style="color:var(--fg-dim)">(${host.num_cpus} cores)</span> ·
        up ${duration(host.uptime_s)} ·
        load <b>${load.avg1.toFixed(2)}</b> ${load.avg5.toFixed(2)} ${load.avg15.toFixed(2)}
    `;
}
