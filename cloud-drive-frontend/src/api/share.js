import api from '../utils/request'

// Create share
export function createShare(data) {
  return api.post('/share/create', data)
}

// Verify share
export function verifyShare(data) {
  return api.post('/share/verify', data)
}

// Get share info
export function getShareInfo(shareUrl) {
  return api.get(`/share/info/${shareUrl}`)
}

// List shared folder files
export function listSharedFiles(shareUrl, params) {
  return api.get(`/share/files/${shareUrl}`, { params })
}

// Download shared file
export function downloadSharedFile(shareUrl, code) {
  return api.get(`/share/download/${shareUrl}${code ? '?code=' + code : ''}`, {
    responseType: 'blob',
    headers: { 'Content-Type': undefined }
  })
}

// List my shares
export function listMyShares(params) {
  return api.get('/share/list', { params })
}

// Cancel share
export function cancelShare(shareId) {
  return api.put('/share/cancel', { share_id: shareId })
}
