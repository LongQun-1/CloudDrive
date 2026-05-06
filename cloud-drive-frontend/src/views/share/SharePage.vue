<template>
  <div class="share-container">
    <div class="share-card" :class="{ 'share-card-wide': verified && shareInfo?.is_dir }">
      <el-icon style="font-size: 48px; color: #409eff; margin-bottom: 16px;"><Share /></el-icon>
      
      <!-- Step 1: Show share info + verify code -->
      <div v-if="!verified && shareInfo">
        <h2>{{ shareInfo.file_name }}</h2>
        <p style="color: #909399; margin: 8px 0;">
          {{ shareInfo.is_dir ? '文件夹' : formatFileSize(shareInfo.file_size) }}
        </p>
        <p v-if="shareInfo.need_code" style="color: #e6a23c; margin: 16px 0;">需要提取码</p>
        
        <div v-if="shareInfo.need_code" style="margin-top: 20px;">
          <el-input v-model="shareCode" placeholder="请输入提取码" style="width: 200px;" @keyup.enter="handleVerify" />
          <el-button type="primary" style="margin-left: 12px;" :loading="verifying" @click="handleVerify">
            验证
          </el-button>
        </div>
        <div v-else style="margin-top: 20px;">
          <el-button type="primary" size="large" @click="handleVerify" :loading="verifying">
            查看文件
          </el-button>
        </div>
      </div>

      <!-- Step 2: Verified - single file -->
      <div v-if="verified && shareInfo && !shareInfo.is_dir">
        <h2>{{ shareInfo.file_name }}</h2>
        <p style="color: #909399; margin: 8px 0;">
          {{ formatFileSize(shareInfo.file_size) }}
        </p>
        <el-button type="primary" size="large" style="margin-top: 20px;" @click="handleDownloadSingle">
          <el-icon><Download /></el-icon> 下载文件
        </el-button>
      </div>

      <!-- Step 2: Verified - folder -->
      <div v-if="verified && shareInfo && shareInfo.is_dir" class="share-folder-view">
        <h2 style="margin-bottom: 16px;">📁 {{ shareInfo.file_name }}</h2>
        
        <!-- Folder breadcrumb -->
        <div class="share-breadcrumb" v-if="folderBreadcrumbs.length > 0">
          <el-breadcrumb separator="/">
            <el-breadcrumb-item @click="navigateShareFolder(0)">
              <el-icon><HomeFilled /></el-icon> 根目录
            </el-breadcrumb-item>
            <el-breadcrumb-item v-for="(crumb, idx) in folderBreadcrumbs" :key="idx"
              @click="navigateShareFolder(idx + 1)">
              {{ crumb.name }}
            </el-breadcrumb-item>
          </el-breadcrumb>
        </div>

        <!-- File list -->
        <el-table :data="folderFiles" v-loading="loadingFiles" empty-text="文件夹为空" style="width: 100%; margin-top: 12px;">
          <el-table-column label="文件名" min-width="250">
            <template #default="{ row }">
              <div class="share-file-name" @click="handleOpenSharedFile(row)">
                <el-icon style="margin-right: 8px; font-size: 18px;">
                  <component :is="getFileIcon(row.filename, row.is_dir)" />
                </el-icon>
                <span>{{ row.filename }}</span>
              </div>
            </template>
          </el-table-column>
          <el-table-column label="大小" width="120">
            <template #default="{ row }">
              {{ row.is_dir ? '-' : formatFileSize(row.file_size) }}
            </template>
          </el-table-column>
          <el-table-column label="修改时间" width="180">
            <template #default="{ row }">
              {{ formatDate(row.updated_at) }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="100">
            <template #default="{ row }">
              <el-button v-if="!row.is_dir" link type="primary" @click="handleDownloadFromFolder(row)">
                <el-icon><Download /></el-icon> 下载
              </el-button>
            </template>
          </el-table-column>
        </el-table>
      </div>

      <!-- Loading -->
      <div v-if="!shareInfo && loadingInfo">
        <el-icon class="is-loading" style="font-size: 32px;"><Loading /></el-icon>
        <p style="margin-top: 12px; color: #909399;">加载中...</p>
      </div>

      <!-- Error -->
      <div v-if="!shareInfo && !loadingInfo && error">
        <el-icon style="font-size: 48px; color: #f56c6c; margin-bottom: 16px;"><CircleCloseFilled /></el-icon>
        <h2 style="color: #f56c6c;">{{ error }}</h2>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { getShareInfo, verifyShare } from '../../api/share'
import { formatFileSize, formatDate, getFileIcon } from '../../utils/helpers'
import { ElMessage } from 'element-plus'
import api from '../../utils/request'

const route = useRoute()
const shareUrl = route.params.shareUrl

const shareInfo = ref(null)
const loadingInfo = ref(true)
const error = ref('')
const shareCode = ref('')
const verifying = ref(false)
const verified = ref(false)

// Folder browsing state
const folderFiles = ref([])
const loadingFiles = ref(false)
const folderBreadcrumbs = ref([])
const currentFolderParentId = ref(0)

onMounted(async () => {
  try {
    const res = await getShareInfo(shareUrl)
    if (res.code === 0) {
      shareInfo.value = res.data
    } else {
      error.value = res.message || '分享不存在或已失效'
    }
  } catch (e) {
    error.value = '分享不存在或已失效'
  } finally {
    loadingInfo.value = false
  }
})

async function handleVerify() {
  if (shareInfo.value?.need_code && !shareCode.value.trim()) {
    ElMessage.warning('请输入提取码')
    return
  }
  verifying.value = true
  try {
    const res = await verifyShare({
      share_url: shareUrl,
      share_code: shareCode.value
    })
    if (res.code === 0) {
      verified.value = true
      // If it's a folder, load its files
      if (shareInfo.value?.is_dir) {
        await loadFolderFiles(0)
      }
    }
  } catch (e) {} finally {
    verifying.value = false
  }
}

async function loadFolderFiles(parentId) {
  loadingFiles.value = true
  currentFolderParentId.value = parentId
  try {
    const params = { parent_id: parentId }
    if (shareCode.value) params.code = shareCode.value
    const res = await api.get(`/share/files/${shareUrl}`, { params })
    if (res.code === 0) {
      folderFiles.value = res.data?.files || []
    }
  } catch (e) {
    ElMessage.error('加载文件列表失败')
  } finally {
    loadingFiles.value = false
  }
}

function handleOpenSharedFile(file) {
  if (file.is_dir) {
    folderBreadcrumbs.value.push({ id: file.id, name: file.filename })
    loadFolderFiles(file.id)
  }
}

function navigateShareFolder(level) {
  if (level === 0) {
    folderBreadcrumbs.value = []
    loadFolderFiles(0)
  } else {
    folderBreadcrumbs.value = folderBreadcrumbs.value.slice(0, level)
    const crumb = folderBreadcrumbs.value[level - 1]
    loadFolderFiles(crumb.id)
  }
}

function handleDownloadSingle() {
  const code = shareCode.value || ''
  const url = `/api/share/download/${shareUrl}${code ? '?code=' + code : ''}`
  triggerDownload(url, shareInfo.value?.file_name || 'file')
}

function handleDownloadFromFolder(file) {
  const code = shareCode.value || ''
  const url = `/api/share/download/${shareUrl}?file_id=${file.id}${code ? '&code=' + code : ''}`
  triggerDownload(url, file.filename)
}

function triggerDownload(url, filename) {
  const a = document.createElement('a')
  a.href = url
  a.download = filename
  a.style.display = 'none'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
}
</script>

<style scoped>
.share-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.share-card {
  width: 500px;
  padding: 40px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
  text-align: center;
}

.share-card-wide {
  width: 750px;
  max-width: 90vw;
  text-align: left;
}

.share-card h2 {
  margin-bottom: 20px;
  color: #303133;
}

.share-folder-view {
  width: 100%;
}

.share-breadcrumb {
  margin-bottom: 8px;
}

.share-file-name {
  display: flex;
  align-items: center;
  cursor: pointer;
  color: #409eff;
}

.share-file-name:hover {
  text-decoration: underline;
}
</style>
