import type { ProcessInfo, Snapshot } from '../types';
import { bytes, compactTime, pct } from '../format';

const tableHead = document.querySelector('#proc-table thead tr') as HTMLElement;
const tableBody = document.querySelector('#proc-table tbody') as HTMLElement;
const filterEl = document.getElementById('proc-filter') as HTMLInputElement;
const countEl = document.getElementById('proc-count') as HTMLElement;

interface Column {
    key: keyof ProcessInfo;
    label: string;
    numeric?: boolean;
    render: (p: ProcessInfo) => string;
}

const columns: Column[] = [
    { key: 'pid',          label: 'PID',     numeric: true, render: (p) => String(p.pid) },
    { key: 'user',         label: 'USER',                  render: (p) => p.user },
    { key: 'state',        label: 'S',                     render: (p) => p.state },
    { key: 'cpu_pct',      label: 'CPU%',    numeric: true, render: (p) => pct(p.cpu_pct, 1) },
    { key: 'mem_pct',      label: 'MEM%',    numeric: true, render: (p) => pct(p.mem_pct, 1) },
    { key: 'rss',          label: 'RES',     numeric: true, render: (p) => bytes(p.rss) },
    { key: 'vsize',        label: 'VIRT',    numeric: true, render: (p) => bytes(p.vsize) },
    { key: 'total_time_s', label: 'TIME+',   numeric: true, render: (p) => compactTime(p.total_time_s) },
    { key: 'cmd',          label: 'COMMAND',               render: (p) => p.cmd },
];

let sortKey: keyof ProcessInfo = 'cpu_pct';
let sortDir: 'asc' | 'desc' = 'desc';
let filter = '';

function renderHead(): void {
    tableHead.innerHTML = columns
        .map((c) => {
            const cls = c.key === sortKey ? (sortDir === 'asc' ? 'sort-asc' : 'sort-desc') : '';
            return `<th data-key="${c.key}" class="${cls}${c.numeric ? ' num' : ''}">${c.label}</th>`;
        })
        .join('');
}

tableHead.addEventListener('click', (ev) => {
    const th = (ev.target as HTMLElement).closest('th') as HTMLElement | null;
    if (!th) return;
    const key = th.dataset.key as keyof ProcessInfo | undefined;
    if (!key) return;
    if (key === sortKey) {
        sortDir = sortDir === 'asc' ? 'desc' : 'asc';
    } else {
        sortKey = key;
        sortDir = columns.find((c) => c.key === key)?.numeric ? 'desc' : 'asc';
    }
    renderHead();
    if (lastSnap) renderProcesses(lastSnap);
});

filterEl.addEventListener('input', () => {
    filter = filterEl.value.trim().toLowerCase();
    if (lastSnap) renderProcesses(lastSnap);
});

renderHead();

let lastSnap: Snapshot | null = null;

function matches(p: ProcessInfo): boolean {
    if (!filter) return true;
    return (
        p.cmd.toLowerCase().includes(filter) ||
        p.user.toLowerCase().includes(filter) ||
        String(p.pid).includes(filter)
    );
}

function compare(a: ProcessInfo, b: ProcessInfo): number {
    const av = a[sortKey] as unknown;
    const bv = b[sortKey] as unknown;
    let cmp: number;
    if (typeof av === 'number' && typeof bv === 'number') {
        cmp = av - bv;
    } else {
        cmp = String(av).localeCompare(String(bv));
    }
    return sortDir === 'asc' ? cmp : -cmp;
}

export function renderProcesses(s: Snapshot): void {
    lastSnap = s;
    const filtered = s.processes.filter(matches).sort(compare);
    // ограничиваем число строк для отзывчивости DOM
    const MAX_ROWS = 500;
    const rows = filtered.slice(0, MAX_ROWS);

    countEl.textContent =
        filtered.length === s.processes.length
            ? `${s.processes.length} processes`
            : `${filtered.length} / ${s.processes.length} shown`;

    // переиспользуем существующие строки чтобы уменьшить нагрузку на DOM
    const tb = tableBody;
    while (tb.childElementCount > rows.length) tb.lastElementChild?.remove();
    while (tb.childElementCount < rows.length) {
        const tr = document.createElement('tr');
        for (const c of columns) {
            const td = document.createElement('td');
            if (c.numeric) td.className = 'num';
            tr.appendChild(td);
        }
        tb.appendChild(tr);
    }
    for (let r = 0; r < rows.length; r++) {
        const tr = tb.children[r] as HTMLElement;
        const p = rows[r];
        for (let c = 0; c < columns.length; c++) {
            const cell = tr.children[c] as HTMLElement;
            const text = columns[c].render(p);
            if (cell.textContent !== text) cell.textContent = text;
        }
    }
}
