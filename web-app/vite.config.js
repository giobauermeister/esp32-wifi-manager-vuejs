import { defineConfig } from 'vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

export default defineConfig({
  plugins: [viteSingleFile()],
  build: {
    target: 'es2018',
    assetsInlineLimit: 100000000,
    cssCodeSplit: false,
    sourcemap: false
  }
})
