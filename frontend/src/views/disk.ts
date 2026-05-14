import type { Snapshot } from '../types';
import { bps } from '../format';
import { makeChart, type LiveChart } from '../charts';

const devs = document.getElementById('disk-devices') as HTMLElement;
const chartEl = document.getElementById('disk-chart') as HTMLElement;
let chart: LiveChart | null = null;

function sumDisks(s: Snapshot): { rd: number; wr: number } {
    let rd = 0, wr = 0;
    for (const d of s.disk) {
        rd += d.read_bps;
        wr += d.write_bps;
    }
    return { rd, wr };
}

export function renderDisk(s: Snapshot, history: ReadonlyArray<Snapshot>): void {
    devs.innerHTML = s.disk
        .map(
            (d) => `
        <div class="iface-row">
            <span class="name">${d.name}</span>
            <span class="rd">read ${bps(d.read_bps)}</span>
            <span class="wr">write ${bps(d.write_bps)}</span>
        </div>`,
        )
        .join('') || '<div class="hint">no block devices detected</div>';

    if (!chart) {
        chart = makeChart(
            chartEl,
            [
                { label: 'read', color: '#5aa9e0', valueFmt: (v) => bps(v ?? 0) },
                { label: 'write', color: '#e5a13a', valueFmt: (v) => bps(v ?? 0) },
            ],
            { yMin: 0, yFmt: (v) => bps(v) },
        );
    }
    const ts = history.map((h) => h.ts_ms / 1000);
    const rd = history.map((h) => sumDisks(h).rd);
    const wr = history.map((h) => sumDisks(h).wr);
    chart.update(ts, [rd, wr]);
}
