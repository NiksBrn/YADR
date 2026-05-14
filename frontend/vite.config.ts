import { defineConfig } from 'vite';

// Vite config. The dev server proxies API/WS to the C++ backend so `npm run dev`
// can be used for iterating on the UI while `yadr-server` runs on :8080.
export default defineConfig({
    server: {
        port: 5173,
        proxy: {
            '/api': 'http://127.0.0.1:8080',
            '/ws': {
                target: 'ws://127.0.0.1:8080',
                ws: true,
            },
        },
    },
    build: {
        outDir: 'dist',
        emptyOutDir: true,
        target: 'es2022',
        sourcemap: false,
    },
});
