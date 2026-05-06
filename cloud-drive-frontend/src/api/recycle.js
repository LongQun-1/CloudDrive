import api from '../utils/request'

// List recycle bin
export function listRecycleBin(params) {
  return api.get('/recycle/list', { params })
}

// Restore file(s)
export function restoreFile(data) {
  return api.put('/recycle/restore', data)
}

// Permanent delete
export function permanentDelete(data) {
  return api.delete('/recycle/delete', { data })
}

// Empty recycle bin
export function emptyRecycleBin() {
  return api.delete('/recycle/empty')
}
