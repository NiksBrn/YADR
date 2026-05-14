import type { Snapshot } from '../types';
import { bytes } from '../format';
import { makeChart, type LiveChart } from '../charts';

const bars = document.getElementById('memory-bars') as HTMLElement;
const chartEl = document.getElementById('memory-chart') as HTMLElement;
let chart: LiveChart | null = null;

function bar(name: string, used: number, total: number, color: string, extra: string): string {
    const pct = total > 0 ? (used / total) * 100 : 0;
    return `
        <div class="mem-row">
            <span class="name">${name}</span>
            <span class="bar"><span class="fill" style="width:${pct.toFixed(1)}%;background:${color}"></span></span>
            <span class="nums">${bytes(used)} / ${bytes(total)} <span style="color:var(--fg-dim)">${extra}</span></span>
        </div>`;
}

export function renderMemory(s: Snapshot, history: ReadonlyArray<Snapshot>): void {
    const m = s.memory;
    bars.innerHTML =
        bar('RAM', m.used, m.total, '#29b56b',
            `· buffers ${bytes(m.buffers)} · cache ${bytes(m.cached)}`) +
        bar('Swap', m.swap_used, m.swap_total, '#a779e0', '');

    if (!chart) {
        chart = makeChart(
            chartEl,
            [{ label: 'used', color: '#29b56b', valueFmt: (v) => bytes(v ?? 0) }],
            { yMin: 0, yFmt: (v) => bytes(v) },
        );
    }
    const ts = history.map((h) => h.ts_ms / 1000);
    const ys = history.map((h) => h.memory.used);
    chart.update(ts, [ys]);
}
