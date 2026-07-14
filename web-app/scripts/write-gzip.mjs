import { gzipSync } from 'node:zlib'
import { mkdir, readFile, writeFile } from 'node:fs/promises'
import { resolve } from 'node:path'

const webRoot = resolve(import.meta.dirname, '..')
const html = await readFile(resolve(webRoot, 'dist/index.html'))
const gzip = gzipSync(html, { level: 9, mtime: 0 })
const firmwareData = resolve(webRoot, '../data')

await mkdir(firmwareData, { recursive: true })
await writeFile(resolve(webRoot, 'dist/index.html.gz'), gzip)
await writeFile(resolve(firmwareData, 'index.html.gz'), gzip)

console.log(`Created data/index.html.gz (${gzip.length} bytes, source ${html.length} bytes)`)
