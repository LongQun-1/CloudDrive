import axios from 'axios'
import { ElMessage } from 'element-plus'
import router from '../router'

const api = axios.create({
  baseURL: '/api',
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// Request interceptor - add JWT token
api.interceptors.request.use(
  config => {
    const token = localStorage.getItem('access_token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  error => Promise.reject(error)
)

// Response interceptor - handle errors and token refresh
api.interceptors.response.use(
  response => {
    const data = response.data
    if (data.code !== undefined && data.code !== 0) {
      // Business error
      const errorMsg = data.message || '请求失败'
      
      if (data.code === 1002) {
        // Token expired, try refresh
        return refreshTokenAndRetry(response.config)
      }
      
      ElMessage.error(errorMsg)
      return Promise.reject(new Error(errorMsg))
    }
    return data
  },
  async error => {
    if (error.response?.status === 401) {
      return refreshTokenAndRetry(error.config)
    }
    const msg = error.response?.data?.message || error.message || '网络错误'
    ElMessage.error(msg)
    return Promise.reject(error)
  }
)

let isRefreshing = false
let refreshSubscribers = []

function subscribeTokenRefresh(cb) {
  refreshSubscribers.push(cb)
}

function onTokenRefreshed(newToken) {
  refreshSubscribers.forEach(cb => cb(newToken))
  refreshSubscribers = []
}

async function refreshTokenAndRetry(config) {
  const refreshToken = localStorage.getItem('refresh_token')
  if (!refreshToken) {
    logout()
    return Promise.reject(new Error('未登录'))
  }

  if (!isRefreshing) {
    isRefreshing = true
    try {
      const res = await axios.post('/api/user/refresh', {
        refresh_token: refreshToken
      })
      
      if (res.data.code === 0) {
        const newAccessToken = res.data.data.access_token
        const newRefreshToken = res.data.data.refresh_token
        localStorage.setItem('access_token', newAccessToken)
        localStorage.setItem('refresh_token', newRefreshToken)
        isRefreshing = false
        onTokenRefreshed(newAccessToken)
        
        config.headers.Authorization = `Bearer ${newAccessToken}`
        return api(config)
      } else {
        logout()
        return Promise.reject(new Error('Token刷新失败'))
      }
    } catch (err) {
      isRefreshing = false
      logout()
      return Promise.reject(err)
    }
  }

  return new Promise(resolve => {
    subscribeTokenRefresh(newToken => {
      config.headers.Authorization = `Bearer ${newToken}`
      resolve(api(config))
    })
  })
}

function logout() {
  localStorage.removeItem('access_token')
  localStorage.removeItem('refresh_token')
  localStorage.removeItem('user_info')
  router.push('/login')
}

export default api
export { logout }
