import uPlot, { type AlignedData, type Options as UplotOptions } from 'uplot';
import 'uplot/dist/uPlot.min.css';

export interface SeriesSpec {
    label: string;
    color: string;
    valueFmt?: (v: number | null) => string;
}

export interface LiveChart {
    update(timestamps: number[], series: number[][]): void;
    destroy(): void;
}

export function makeChart(
    container: HTMLElement,
    series: SeriesSpec[],
    opts: { yMin?: number; yMax?: number; yFmt?: (v: number) => string } = {},
): LiveChart {
    const baseSeries: UplotOptions['series'] = [
        { label: 'time' },
        ...series.map((s) => ({
            label: s.label,
            stroke: s.color,
            fill: s.color + '22',
            width: 1.5,
            points: { show: false },
            value: (_u: uPlot, v: number | null) =>
                v == null ? '—' : (s.valueFmt ? s.valueFmt(v) : v.toFixed(1)),
        })),
    ];

    const yFmt = opts.yFmt;

    const options: UplotOptions = {
        width: container.clientWidth || 400,
        height: container.clientHeight || 110,
        padding: [6, 12, 0, 12],
        cursor: { drag: { x: false, y: false } },
        legend: { show: true, live: true },
        scales: {
            x: { time: true },
            y: { range: (_u, dMin, dMax) => [opts.yMin ?? dMin, opts.yMax ?? dMax] },
        },
        axes: [
            { stroke: '#8c96a3', grid: { stroke: '#23304033' }, ticks: { stroke: '#23304033' } },
            {
                stroke: '#8c96a3',
                grid: { stroke: '#23304033' },
                ticks: { stroke: '#23304033' },
                size: 50,
                values: yFmt ? (_u, splits) => splits.map((v) => yFmt(v)) : undefined,
            },
        ],
        series: baseSeries,
    };

    // uPlot требует строку временных меток + по одной строке на каждую серию
    const empty: AlignedData = [[], ...series.map(() => [] as number[])] as AlignedData;
    const plot = new uPlot(options, empty, container);

    const ro = new ResizeObserver(() => {
        plot.setSize({ width: container.clientWidth, height: container.clientHeight });
    });
    ro.observe(container);

    return {
        update(timestamps, seriesData) {
            const data: AlignedData = [timestamps, ...seriesData] as AlignedData;
            plot.setData(data);
        },
        destroy() {
            ro.disconnect();
            plot.destroy();
        },
    };
}
