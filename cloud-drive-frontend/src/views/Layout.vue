<template>
  <div class="layout-container">
    <!-- Sidebar -->
    <div class="layout-sidebar">
      <div class="sidebar-logo">
        <el-icon><Cloudy /></el-icon>
        CloudDrive
      </div>
      <el-menu
        :default-active="currentRoute"
        class="sidebar-menu"
        background-color="#304156"
        text-color="#bfcbd9"
        active-text-color="#409eff"
        router
      >
        <el-menu-item index="/">
          <el-icon><Folder /></el-icon>
          <span>我的文件</span>
        </el-menu-item>
        <el-menu-item index="/shares">
          <el-icon><Share /></el-icon>
          <span>我的分享</span>
        </el-menu-item>
        <el-menu-item index="/recycle">
          <el-icon><Delete /></el-icon>
          <span>回收站</span>
        </el-menu-item>
        <el-menu-item index="/settings">
          <el-icon><Setting /></el-icon>
          <span>个人设置</span>
        </el-menu-item>
      </el-menu>
    </div>

    <!-- Main content -->
    <div class="layout-main">
      <div class="layout-header">
        <div class="header-left">
          <span style="font-size: 18px; font-weight: 500;">{{ pageTitle }}</span>
        </div>
        <div class="header-right">
          <span style="color: #909399; font-size: 14px;">
            {{ userStore.userInfo?.username }}
          </span>
          <el-dropdown @command="handleCommand">
            <el-avatar :size="32" style="cursor: pointer;">
              {{ userStore.userInfo?.nickname?.[0] || 'U' }}
            </el-avatar>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="settings">个人设置</el-dropdown-item>
                <el-dropdown-item command="logout" divided>退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </div>
      <div class="layout-content">
        <router-view />
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useUserStore } from '../store/user'
import { logout as apiLogout } from '../api/user'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()

const currentRoute = computed(() => route.path)
const pageTitle = computed(() => route.meta.title || 'CloudDrive')

onMounted(() => {
  if (!userStore.userInfo) {
    userStore.loadUserInfo()
  }
})

async function handleCommand(cmd) {
  if (cmd === 'logout') {
    try {
      await apiLogout()
    } catch (e) {}
    userStore.clearAuth()
    router.push('/login')
    ElMessage.success('已退出登录')
  } else if (cmd === 'settings') {
    router.push('/settings')
  }
}
</script>
