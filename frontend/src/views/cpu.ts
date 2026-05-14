import type { Snapshot } from '../types';
import { pct } from '../format';
import { makeChart, type LiveChart } from '../charts';

const summary = document.getElementById('cpu-summary') as HTMLElement;
const cores = document.getElementById('cpu-cores') as HTMLElement;
const chartEl = document.getElementById('cpu-chart') as HTMLElement;

let chart: LiveChart | null = null;

function classFor(p: number): string {
    if (p >= 85) return 'hot';
    if (p >= 50) return 'warn';
    return '';
}

function ensureCoreCells(n: number): void {
    if (cores.childElementCount === n) return;
    cores.innerHTML = '';
    for (let i = 0; i < n; i++) {
        const row = document.createElement('div');
        row.className = 'cpu-core';
        row.innerHTML = `<span class="name">cpu${i}</span>
            <span class="bar"><span class="fill"></span></span>
            <span class="pct">0.0%</span>`;
        cores.appendChild(row);
    }
}

export function renderCpu(s: Snapshot, history: ReadonlyArray<Snapshot>): void {
    summary.innerHTML = `
        <div><div class="lbl">total</div><div class="num">${pct(s.cpu.total)}</div></div>
        <div><div class="lbl">cores</div><div class="num">${s.host.num_cpus}</div></div>
        <div><div class="lbl">load 1m</div><div class="num">${s.load.avg1.toFixed(2)}</div></div>
    `;

    ensureCoreCells(s.cpu.per_core.length);
    for (let i = 0; i < s.cpu.per_core.length; i++) {
        const p = s.cpu.per_core[i];
        const row = cores.children[i] as HTMLElement;
        const fill = row.querySelector('.fill') as HTMLElement;
        fill.style.width = `${Math.min(100, Math.max(0, p))}%`;
        fill.className = `fill ${classFor(p)}`;
        (row.querySelector('.pct') as HTMLElement).textContent = pct(p, 0);
    }

    if (!chart) {
        chart = makeChart(chartEl, [{ label: 'CPU %', color: '#29b56b', valueFmt: (v) => pct(v ?? 0) }], {
            yMin: 0,
            yMax: 100,
            yFmt: (v) => `${v.toFixed(0)}%`,
        });
    }
    const ts = history.map((h) => h.ts_ms / 1000);
    const ys = history.map((h) => h.cpu.total);
    chart.update(ts, [ys]);
}
