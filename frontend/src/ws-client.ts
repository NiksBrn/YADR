import type { Snapshot } from './types';
import { store } from './state';

type ConnState = 'connecting' | 'open' | 'closed';
type StateListener = (s: ConnState) => void;

class WsClient {
    private ws: WebSocket | null = null;
    private backoffMs = 250;
    private stopped = false;
    private listeners = new Set<StateListener>();

    onState(fn: StateListener): () => void {
        this.listeners.add(fn);
        return () => this.listeners.delete(fn);
    }

    start(): void {
        this.stopped = false;
        this.connect();
    }

    stop(): void {
        this.stopped = true;
        this.ws?.close();
    }

    private emit(state: ConnState): void {
        for (const fn of this.listeners) fn(state);
    }

    private url(): string {
        const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
        return `${proto}//${location.host}/ws`;
    }

    private connect(): void {
        this.emit('connecting');
        const ws = new WebSocket(this.url());
        this.ws = ws;

        ws.onopen = () => {
            this.backoffMs = 250;
            this.emit('open');
        };
        ws.onmessage = (ev) => {
            try {
                const snap: Snapshot = JSON.parse(ev.data);
                store.push(snap);
            } catch (e) {
                console.error('failed to parse snapshot', e);
            }
        };
        ws.onclose = () => this.scheduleReconnect();
        ws.onerror = () => ws.close();
    }

    private scheduleReconnect(): void {
        this.emit('closed');
        if (this.stopped) return;
        const delay = this.backoffMs;
        this.backoffMs = Math.min(this.backoffMs * 2, 10_000);
        setTimeout(() => {
            if (!this.stopped) this.connect();
        }, delay);
    }
}

export const wsClient = new WsClient();
