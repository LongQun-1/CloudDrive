import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { getUserInfo as fetchUserInfo } from '../api/user'

export const useUserStore = defineStore('user', () => {
  const userInfo = ref(JSON.parse(localStorage.getItem('user_info') || 'null'))
  const token = ref(localStorage.getItem('access_token') || '')

  const isLoggedIn = computed(() => !!token.value)
  const storagePercent = computed(() => {
    if (!userInfo.value) return 0
    return Math.round((userInfo.value.storage_used / userInfo.value.storage_limit) * 10000) / 100
  })

  function setTokens(access, refresh) {
    token.value = access
    localStorage.setItem('access_token', access)
    localStorage.setItem('refresh_token', refresh)
  }

  function setUserInfo(info) {
    userInfo.value = info
    localStorage.setItem('user_info', JSON.stringify(info))
  }

  async function loadUserInfo() {
    try {
      const res = await fetchUserInfo()
      if (res.code === 0) {
        setUserInfo(res.data)
      }
    } catch (e) {
      console.error('Failed to load user info', e)
    }
  }

  function clearAuth() {
    token.value = ''
    userInfo.value = null
    localStorage.removeItem('access_token')
    localStorage.removeItem('refresh_token')
    localStorage.removeItem('user_info')
  }

  return {
    userInfo,
    token,
    isLoggedIn,
    storagePercent,
    setTokens,
    setUserInfo,
    loadUserInfo,
    clearAuth
  }
})
