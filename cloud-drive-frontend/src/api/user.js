import api from '../utils/request'

// Register
export function register(data) {
  return api.post('/user/register', data)
}

// Login
export function login(data) {
  return api.post('/user/login', data)
}

// Refresh token
export function refreshToken(refreshToken) {
  return api.post('/user/refresh', { refresh_token: refreshToken })
}

// Logout
export function logout() {
  return api.post('/user/logout')
}

// Get user info
export function getUserInfo() {
  return api.get('/user/info')
}

// Update user info
export function updateUserInfo(data) {
  return api.put('/user/info', data)
}

// Change password
export function changePassword(data) {
  return api.put('/user/password', data)
}
