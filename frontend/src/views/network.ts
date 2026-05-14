import type { Snapshot } from '../types';
import { bps } from '../format';
import { makeChart, type LiveChart } from '../charts';

const ifaces = document.getElementById('network-ifaces') as HTMLElement;
const chartEl = document.getElementById('network-chart') as HTMLElement;
let chart: LiveChart | null = null;

function sumActive(s: Snapshot): { rx: number; tx: number } {
    let rx = 0, tx = 0;
    for (const i of s.network) {
        if (i.name === 'lo') continue;  // loopback искажает суммарный трафик -- исключаем
        rx += i.rx_bps;
        tx += i.tx_bps;
    }
    return { rx, tx };
}

export function renderNetwork(s: Snapshot, history: ReadonlyArray<Snapshot>): void {
    ifaces.innerHTML = s.network
        .map(
            (i) => `
        <div class="iface-row">
            <span class="name">${i.name}</span>
            <span class="rx">↓ ${bps(i.rx_bps)}</span>
            <span class="tx">↑ ${bps(i.tx_bps)}</span>
        </div>`,
        )
        .join('');

    if (!chart) {
        chart = makeChart(
            chartEl,
            [
                { label: 'rx', color: '#5aa9e0', valueFmt: (v) => bps(v ?? 0) },
                { label: 'tx', color: '#a779e0', valueFmt: (v) => bps(v ?? 0) },
            ],
            { yMin: 0, yFmt: (v) => bps(v) },
        );
    }
    const ts = history.map((h) => h.ts_ms / 1000);
    const rxs = history.map((h) => sumActive(h).rx);
    const txs = history.map((h) => sumActive(h).tx);
    chart.update(ts, [rxs, txs]);
}
