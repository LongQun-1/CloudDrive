import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://101.42.30.47:8080',
        changeOrigin: true
      },
      '/health': {
        target: 'http://101.42.30.47:8080',
        changeOrigin: true
      }
    }
  }
})
