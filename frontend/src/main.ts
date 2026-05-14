import { wsClient } from './ws-client';
import { store } from './state';
import { renderHeader } from './views/header';
import { renderCpu } from './views/cpu';
import { renderMemory } from './views/memory';
import { renderNetwork } from './views/network';
import { renderDisk } from './views/disk';
import { renderProcesses } from './views/processes';

const connEl = document.getElementById('conn-status') as HTMLElement;
wsClient.onState((s) => {
    connEl.dataset.state = s;
    connEl.textContent =
        s === 'open' ? 'live' : s === 'connecting' ? 'connecting…' : 'disconnected';
});

store.subscribe((snap, history) => {
    renderHeader(snap);
    renderCpu(snap, history);
    renderMemory(snap, history);
    renderNetwork(snap, history);
    renderDisk(snap, history);
    renderProcesses(snap);
});

wsClient.start();
