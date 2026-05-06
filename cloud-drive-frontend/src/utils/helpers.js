import SparkMD5 from 'spark-md5'

/**
 * Calculate file hash using SparkMD5 (SHA256 not available in browser easily, use MD5 for quick check)
 * For production, should use Web Crypto API for SHA256
 */
export function calculateFileHash(file) {
  return new Promise((resolve, reject) => {
    const chunkSize = 2 * 1024 * 1024 // 2MB chunks for hash calc
    const chunks = Math.ceil(file.size / chunkSize)
    const spark = new SparkMD5.ArrayBuffer()
    const reader = new FileReader()
    let currentChunk = 0

    reader.onload = (e) => {
      spark.append(e.target.result)
      currentChunk++
      if (currentChunk < chunks) {
        loadNext()
      } else {
        resolve(spark.end())
      }
    }

    reader.onerror = reject

    function loadNext() {
      const start = currentChunk * chunkSize
      const end = Math.min(start + chunkSize, file.size)
      reader.readAsArrayBuffer(file.slice(start, end))
    }

    loadNext()
  })
}

/**
 * Calculate SHA256 hash using Web Crypto API
 */
export async function calculateFileSHA256(file) {
  const chunkSize = 2 * 1024 * 1024
  const chunks = Math.ceil(file.size / chunkSize)
  const hashBuffer = await crypto.subtle.digest('SHA-256', await file.arrayBuffer())
  const hashArray = Array.from(new Uint8Array(hashBuffer))
  return hashArray.map(b => b.toString(16).padStart(2, '0')).join('')
}

/**
 * Format file size
 */
export function formatFileSize(bytes) {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

/**
 * Format date
 */
export function formatDate(dateStr) {
  if (!dateStr) return ''
  const d = new Date(dateStr)
  return d.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

/**
 * Get file icon by filename
 */
export function getFileIcon(filename, isDir) {
  if (isDir) return 'Folder'
  const ext = filename.split('.').pop().toLowerCase()
  const iconMap = {
    jpg: 'Picture', jpeg: 'Picture', png: 'Picture', gif: 'Picture', webp: 'Picture', bmp: 'Picture',
    mp4: 'VideoPlay', avi: 'VideoPlay', mkv: 'VideoPlay', mov: 'VideoPlay', webm: 'VideoPlay',
    pdf: 'Document',
    doc: 'Document', docx: 'Document', xls: 'Document', xlsx: 'Document', ppt: 'Document', pptx: 'Document',
    txt: 'Document', md: 'Document',
    zip: 'Files', rar: 'Files', '7z': 'Files', tar: 'Files', gz: 'Files',
    mp3: 'Headset', wav: 'Headset', flac: 'Headset',
  }
  return iconMap[ext] || 'Document'
}

/**
 * Get file type label
 */
export function getFileType(filename, isDir) {
  if (isDir) return '文件夹'
  const ext = filename.split('.').pop().toLowerCase()
  const typeMap = {
    jpg: '图片', jpeg: '图片', png: '图片', gif: '图片', webp: '图片',
    mp4: '视频', avi: '视频', mkv: '视频', mov: '视频',
    pdf: 'PDF',
    doc: 'Word', docx: 'Word', xls: 'Excel', xlsx: 'Excel', ppt: 'PPT', pptx: 'PPT',
    txt: '文本', md: '文本',
    zip: '压缩包', rar: '压缩包', '7z': '压缩包',
    mp3: '音频', wav: '音频',
  }
  return typeMap[ext] || '文件'
}
