import type { Snapshot } from './types';

const HISTORY_LIMIT = 120;  // ~2 минуты при частоте 1 Гц

type Listener = (snap: Snapshot, history: ReadonlyArray<Snapshot>) => void;

class Store {
    private history: Snapshot[] = [];
    private listeners = new Set<Listener>();

    push(s: Snapshot): void {
        this.history.push(s);
        if (this.history.length > HISTORY_LIMIT) this.history.shift();
        for (const fn of this.listeners) fn(s, this.history);
    }

    subscribe(fn: Listener): () => void {
        this.listeners.add(fn);
        return () => this.listeners.delete(fn);
    }

    snapshots(): ReadonlyArray<Snapshot> {
        return this.history;
    }

    latest(): Snapshot | null {
        return this.history.length ? this.history[this.history.length - 1] : null;
    }
}

export const store = new Store();
