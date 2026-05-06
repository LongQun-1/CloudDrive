import api from '../utils/request'

// Create folder
export function createFolder(data) {
  return api.post('/file/folder', data)
}

// Upload file (simple upload)
export function uploadFile(file, filename, parentId, onProgress) {
  const formData = new FormData()
  formData.append('file', file)
  return api.post(`/file/upload?filename=${encodeURIComponent(filename)}&parent_id=${parentId}`, file, {
    headers: { 'Content-Type': 'application/octet-stream' },
    onUploadProgress: onProgress
  })
}

// Init chunked upload
export function initChunkedUpload(data) {
  return api.post('/file/chunk/init', data)
}

// Upload chunk
export function uploadChunk(uploadId, chunkIndex, chunkData) {
  return api.post(`/file/chunk/upload?upload_id=${uploadId}&chunk_index=${chunkIndex}`, chunkData, {
    headers: { 'Content-Type': 'application/octet-stream' }
  })
}

// Complete chunked upload
export function completeChunkedUpload(uploadId) {
  return api.post('/file/chunk/complete', { upload_id: uploadId })
}

// Get upload progress
export function getUploadProgress(uploadId) {
  return api.get(`/file/chunk/progress/${uploadId}`)
}

// Download file
export function downloadFile(fileId) {
  return api.get(`/file/download/${fileId}`, {
    responseType: 'blob',
    headers: { 'Content-Type': undefined }
  })
}

// List files
export function listFiles(params) {
  return api.get('/file/list', { params })
}

// Get file info
export function getFileInfo(fileId) {
  return api.get(`/file/info/${fileId}`)
}

// Rename file
export function renameFile(data) {
  return api.put('/file/rename', data)
}

// Move file
export function moveFile(data) {
  return api.put('/file/move', data)
}

// Delete file(s)
export function deleteFile(data) {
  return api.delete('/file/delete', { data })
}

// Search files
export function searchFiles(params) {
  return api.get('/file/search', { params })
}
