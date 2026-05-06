<template>
  <div>
    <!-- Toolbar -->
    <div class="file-toolbar">
      <div class="file-toolbar-left">
        <el-button type="primary" @click="showUploadDialog = true">
          <el-icon><Upload /></el-icon> 上传
        </el-button>
        <el-button @click="handleCreateFolder">
          <el-icon><FolderAdd /></el-icon> 新建文件夹
        </el-button>
      </div>
      <div class="file-toolbar-right">
        <el-input v-model="searchKeyword" placeholder="搜索文件" prefix-icon="Search" style="width: 200px"
          clearable @keyup.enter="handleSearch" @clear="loadFiles" />
        <el-button-group>
          <el-button :type="viewMode === 'list' ? 'primary' : ''" @click="viewMode = 'list'">
            <el-icon><List /></el-icon>
          </el-button>
          <el-button :type="viewMode === 'grid' ? 'primary' : ''" @click="viewMode = 'grid'">
            <el-icon><Grid /></el-icon>
          </el-button>
        </el-button-group>
      </div>
    </div>

    <!-- Breadcrumb -->
    <div class="breadcrumb-container">
      <el-breadcrumb separator="/">
        <el-breadcrumb-item @click="navigateTo(0)">
          <el-icon><HomeFilled /></el-icon> 根目录
        </el-breadcrumb-item>
        <el-breadcrumb-item v-for="(crumb, idx) in breadcrumbs" :key="idx"
          @click="navigateTo(idx + 1)">
          {{ crumb.name }}
        </el-breadcrumb-item>
      </el-breadcrumb>
    </div>

    <!-- List View -->
    <el-table v-if="viewMode === 'list'" :data="fileList" class="file-table" v-loading="loading"
      @row-click="handleRowClick" @row-contextmenu="handleContextMenu" empty-text="暂无文件"
      :row-class-name="getListRowClass">
      <el-table-column label="文件名" min-width="300">
        <template #default="{ row }">
          <div class="file-name-cell" @click="handleOpen(row)"
            :draggable="true"
            @dragstart="onDragStart($event, row)"
            @dragover.prevent="row.is_dir && (dragOverId = row.id)"
            @dragleave="dragOverId = -1"
            @drop.prevent="onDropOnRow($event, row)">
            <el-icon class="file-icon" :class="getIconClass(row)">
              <component :is="getFileIcon(row.filename, row.is_dir)" />
            </el-icon>
            <span>{{ row.filename }}</span>
            <span v-if="row.is_dir && dragOverId === row.id" class="drop-inline-hint">
              松开移入此文件夹
            </span>
          </div>
        </template>
      </el-table-column>
      <el-table-column label="大小" width="120">
        <template #default="{ row }">
          {{ row.is_dir ? '-' : formatSize(row.file_size) }}
        </template>
      </el-table-column>
      <el-table-column label="修改时间" width="180">
        <template #default="{ row }">
          {{ formatDate(row.updated_at) }}
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" @click.stop="handleShare(row)">
            <el-icon><Share /></el-icon>
          </el-button>
          <el-button link type="primary" @click.stop="handleRename(row)">
            <el-icon><Edit /></el-icon>
          </el-button>
          <el-button link type="primary" @click.stop="handleMove(row)">
            <el-icon><Rank /></el-icon>
          </el-button>
          <el-button link type="danger" @click.stop="handleDelete(row)">
            <el-icon><Delete /></el-icon>
          </el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- Grid View -->
    <div v-else class="file-grid">
      <div v-for="file in fileList" :key="file.id" class="file-grid-item"
        :class="{ 'drop-target': file.is_dir && dragOverId === file.id, 'dragging': dragFileId === file.id }"
        draggable="true"
        @dragstart="onDragStart($event, file)"
        @dragover.prevent="onDragOver($event, file)"
        @dragleave="onDragLeave(file)"
        @drop.prevent="onDrop($event, file)"
        @click="handleOpen(file)" @contextmenu.prevent="handleContextMenu(file, $event)">
        <el-icon class="file-icon" :class="getIconClass(file)" style="font-size: 48px;">
          <component :is="getFileIcon(file.filename, file.is_dir)" />
        </el-icon>
        <div class="file-name">{{ file.filename }}</div>
        <div style="font-size: 12px; color: #909399; margin-top: 4px;">
          {{ file.is_dir ? '文件夹' : formatSize(file.file_size) }}
        </div>
        <div v-if="file.is_dir && dragOverId === file.id" class="drop-hint">
          <el-icon><FolderAdd /></el-icon> 移入此文件夹
        </div>
      </div>
      <el-empty v-if="fileList.length === 0 && !loading" description="暂无文件" />
    </div>

    <!-- Pagination -->
    <div style="margin-top: 16px; display: flex; justify-content: flex-end;">
      <el-pagination v-model:current-page="page" v-model:page-size="pageSize"
        :total="total" layout="total, prev, pager, next" @current-change="loadFiles" />
    </div>

    <!-- Create Folder Dialog -->
    <el-dialog v-model="showFolderDialog" title="新建文件夹" width="400px">
      <el-input v-model="folderName" placeholder="请输入文件夹名称" @keyup.enter="confirmCreateFolder" />
      <template #footer>
        <el-button @click="showFolderDialog = false">取消</el-button>
        <el-button type="primary" :loading="creating" @click="confirmCreateFolder">创建</el-button>
      </template>
    </el-dialog>

    <!-- Rename Dialog -->
    <el-dialog v-model="showRenameDialog" title="重命名" width="400px">
      <el-input v-model="renameName" placeholder="请输入新名称" @keyup.enter="confirmRename" />
      <template #footer>
        <el-button @click="showRenameDialog = false">取消</el-button>
        <el-button type="primary" :loading="renaming" @click="confirmRename">确定</el-button>
      </template>
    </el-dialog>

    <!-- Move Dialog -->
    <el-dialog v-model="showMoveDialog" title="移动到" width="400px">
      <el-select v-model="moveTargetId" placeholder="选择目标文件夹" style="width: 100%">
        <el-option label="根目录" :value="0" />
        <el-option v-for="f in allFolders" :key="f.id" :label="f.filename" :value="f.id" />
      </el-select>
      <template #footer>
        <el-button @click="showMoveDialog = false">取消</el-button>
        <el-button type="primary" :loading="moving" @click="confirmMove">移动</el-button>
      </template>
    </el-dialog>

    <!-- Share Dialog -->
    <el-dialog v-model="showShareDialog" title="创建分享" width="450px">
      <el-form label-width="100px">
        <el-form-item label="文件">
          {{ currentFile?.filename }}
        </el-form-item>
        <el-form-item label="需要提取码">
          <el-switch v-model="shareForm.needCode" />
        </el-form-item>
        <el-form-item label="过期时间">
          <el-select v-model="shareForm.expireDays" style="width: 100%">
            <el-option label="永久" :value="0" />
            <el-option label="1天" :value="1" />
            <el-option label="7天" :value="7" />
            <el-option label="30天" :value="30" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showShareDialog = false">取消</el-button>
        <el-button type="primary" :loading="sharing" @click="confirmShare">创建分享</el-button>
      </template>
    </el-dialog>

    <!-- Share Result Dialog -->
    <el-dialog v-model="showShareResult" title="分享成功" width="450px">
      <div style="text-align: center;">
        <p style="margin-bottom: 12px;">分享链接：</p>
        <el-input :model-value="shareResult.url" readonly>
          <template #append>
            <el-button @click="copyToClipboard(shareResult.url)">复制</el-button>
          </template>
        </el-input>
        <p v-if="shareResult.code" style="margin-top: 16px; margin-bottom: 12px;">提取码：</p>
        <el-input v-if="shareResult.code" :model-value="shareResult.code" readonly style="width: 200px; margin: 0 auto; display: block;">
          <template #append>
            <el-button @click="copyToClipboard(shareResult.code)">复制</el-button>
          </template>
        </el-input>
      </div>
    </el-dialog>

    <!-- Upload Dialog -->
    <el-dialog v-model="showUploadDialog" title="上传文件" width="500px" @close="handleUploadDialogClose">
      <div class="upload-drop-zone" :class="{ active: isDragging }"
        @dragover.prevent="isDragging = true" @dragleave="isDragging = false"
        @drop.prevent="handleDrop" @click="triggerFileInput">
        <el-icon><UploadFilled /></el-icon>
        <p>拖拽文件到此处，或点击选择文件</p>
        <p style="color: #909399; font-size: 12px; margin-top: 8px;">支持大文件分片上传</p>
      </div>
      <input ref="fileInputRef" type="file" multiple style="display: none" @change="handleFileSelect" />

      <!-- Upload progress list -->
      <div v-if="uploadList.length > 0" style="margin-top: 16px; max-height: 300px; overflow-y: auto;">
        <div v-for="item in uploadList" :key="item.id" style="margin-bottom: 8px;">
          <div style="display: flex; justify-content: space-between; margin-bottom: 4px;">
            <span style="font-size: 13px;">{{ item.name }}</span>
            <span style="font-size: 12px; color: #909399;">{{ item.status }}</span>
          </div>
          <el-progress :percentage="item.progress" :status="item.progress === 100 ? 'success' : ''" />
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { listFiles, createFolder, renameFile as apiRename, moveFile as apiMove, deleteFile as apiDelete, searchFiles, uploadFile, initChunkedUpload, uploadChunk, completeChunkedUpload } from '../api/file'
import { createShare } from '../api/share'
import { useUserStore } from '../store/user'
import { formatFileSize, formatDate, getFileIcon } from '../utils/helpers'
import { ElMessage, ElMessageBox } from 'element-plus'

const route = useRoute()
const userStore = useUserStore()

const fileList = ref([])
const loading = ref(false)
const page = ref(1)
const pageSize = ref(50)
const total = ref(0)
const viewMode = ref('list')
const searchKeyword = ref('')

// Breadcrumb navigation
const breadcrumbs = ref([])
const currentParentId = ref(0)

// Folder dialog
const showFolderDialog = ref(false)
const folderName = ref('')
const creating = ref(false)

// Rename dialog
const showRenameDialog = ref(false)
const renameName = ref('')
const renaming = ref(false)
const currentFile = ref(null)

// Move dialog
const showMoveDialog = ref(false)
const moveTargetId = ref(0)
const moving = ref(false)
const allFolders = ref([])

// Share dialog
const showShareDialog = ref(false)
const sharing = ref(false)
const shareForm = ref({ needCode: true, expireDays: 0 })
const showShareResult = ref(false)
const shareResult = ref({ url: '', code: '' })

// Upload dialog
const showUploadDialog = ref(false)
const isDragging = ref(false)
const fileInputRef = ref(null)
const uploadList = ref([])

// Drag & drop state for moving files into folders
const dragFileId = ref(-1)
const dragOverId = ref(-1)

const CHUNK_SIZE = 5 * 1024 * 1024 // 5MB

onMounted(() => {
  loadFiles()
})

// Watch route query for parent_id
watch(() => route.query.parent_id, (newVal) => {
  if (newVal !== undefined) {
    currentParentId.value = parseInt(newVal) || 0
    loadFiles()
  }
})

async function loadFiles() {
  loading.value = true
  try {
    const res = await listFiles({
      parent_id: currentParentId.value,
      page: page.value,
      page_size: pageSize.value
    })
    if (res.code === 0) {
      fileList.value = res.data || []
      total.value = res.total || 0
    }
  } catch (e) {} finally {
    loading.value = false
  }
}

function handleRowClick(row) {
  // Default click, no action
}

function handleOpen(file) {
  if (file.is_dir) {
    breadcrumbs.value.push({ id: file.id, name: file.filename })
    currentParentId.value = file.id
    page.value = 1
    loadFiles()
  } else {
    // Download file
    downloadFile(file.id, file.filename)
  }
}

async function downloadFile(fileId, filename) {
  try {
    const token = localStorage.getItem('access_token')
    const resp = await fetch(`/api/file/download/${fileId}`, {
      headers: { Authorization: `Bearer ${token}` }
    })
    if (!resp.ok) throw new Error('Download failed')
    const blob = await resp.blob()
    const url = window.URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = filename
    a.click()
    window.URL.revokeObjectURL(url)
  } catch (e) {
    ElMessage.error('下载失败')
  }
}

function navigateTo(level) {
  if (level === 0) {
    breadcrumbs.value = []
    currentParentId.value = 0
  } else {
    breadcrumbs.value = breadcrumbs.value.slice(0, level)
    currentParentId.value = breadcrumbs.value[level - 1].id
  }
  page.value = 1
  loadFiles()
}

function handleCreateFolder() {
  folderName.value = ''
  showFolderDialog.value = true
}

async function confirmCreateFolder() {
  if (!folderName.value.trim()) {
    ElMessage.warning('请输入文件夹名称')
    return
  }
  creating.value = true
  try {
    const res = await createFolder({
      folder_name: folderName.value,
      parent_id: currentParentId.value
    })
    if (res.code === 0) {
      ElMessage.success('创建成功')
      showFolderDialog.value = false
      loadFiles()
    }
  } catch (e) {} finally {
    creating.value = false
  }
}

function handleRename(file) {
  currentFile.value = file
  renameName.value = file.filename
  showRenameDialog.value = true
}

async function confirmRename() {
  if (!renameName.value.trim()) {
    ElMessage.warning('请输入新名称')
    return
  }
  renaming.value = true
  try {
    const res = await apiRename({
      file_id: currentFile.value.id,
      new_name: renameName.value
    })
    if (res.code === 0) {
      ElMessage.success('重命名成功')
      showRenameDialog.value = false
      loadFiles()
    }
  } catch (e) {} finally {
    renaming.value = false
  }
}

function handleMove(file) {
  currentFile.value = file
  moveTargetId.value = 0
  // Load all folders for move target selection
  allFolders.value = fileList.value.filter(f => f.is_dir && f.id !== file.id)
  showMoveDialog.value = true
}

async function confirmMove() {
  moving.value = true
  try {
    const res = await apiMove({
      file_id: currentFile.value.id,
      new_parent_id: moveTargetId.value
    })
    if (res.code === 0) {
      ElMessage.success('移动成功')
      showMoveDialog.value = false
      loadFiles()
    }
  } catch (e) {} finally {
    moving.value = false
  }
}

async function handleDelete(file) {
  try {
    await ElMessageBox.confirm(`确定删除 "${file.filename}" 吗？文件将移入回收站。`, '删除确认', {
      confirmButtonText: '删除',
      cancelButtonText: '取消',
      type: 'warning'
    })
    const res = await apiDelete({ file_id: file.id })
    if (res.code === 0) {
      ElMessage.success('已移入回收站')
      loadFiles()
      userStore.loadUserInfo()
    }
  } catch (e) {}
}

function handleShare(file) {
  currentFile.value = file
  shareForm.value = { needCode: true, expireDays: 0 }
  showShareDialog.value = true
}

async function confirmShare() {
  sharing.value = true
  try {
    let expireAt = ''
    if (shareForm.value.expireDays > 0) {
      const d = new Date()
      d.setDate(d.getDate() + shareForm.value.expireDays)
      expireAt = d.toISOString().slice(0, 19).replace('T', ' ')
    }
    const res = await createShare({
      file_id: currentFile.value.id,
      need_code: shareForm.value.needCode,
      expire_at: expireAt,
      max_download_count: 0
    })
    if (res.code === 0) {
      const baseUrl = window.location.origin + '/share/'
      shareResult.value = {
        url: baseUrl + res.data.share_url,
        code: res.data.share_code || ''
      }
      showShareDialog.value = false
      showShareResult.value = true
    }
  } catch (e) {} finally {
    sharing.value = false
  }
}

function copyToClipboard(text) {
  navigator.clipboard.writeText(text).then(() => {
    ElMessage.success('已复制到剪贴板')
  }).catch(() => {
    ElMessage.error('复制失败')
  })
}

async function handleSearch() {
  if (!searchKeyword.value.trim()) {
    loadFiles()
    return
  }
  loading.value = true
  try {
    const res = await searchFiles({
      keyword: searchKeyword.value,
      page: page.value,
      page_size: pageSize.value
    })
    if (res.code === 0) {
      fileList.value = res.data || []
      total.value = res.total || 0
    }
  } catch (e) {} finally {
    loading.value = false
  }
}

// Upload functions
function triggerFileInput() {
  fileInputRef.value?.click()
}

function handleFileSelect(e) {
  const files = Array.from(e.target.files)
  files.forEach(f => startUpload(f))
  e.target.value = ''
}

function handleDrop(e) {
  isDragging.value = false
  const files = Array.from(e.dataTransfer.files)
  files.forEach(f => startUpload(f))
}

async function startUpload(file) {
  const uploadId = Date.now()
  const item = {
    id: uploadId,
    name: file.name,
    size: file.size,
    progress: 0,
    status: '准备上传...'
  }
  uploadList.value.push(item)

  // Get reactive reference from the array (Vue3 reactivity)
  const itemRef = uploadList.value[uploadList.value.length - 1]

  try {
    if (file.size <= CHUNK_SIZE) {
      // Simple upload
      itemRef.status = '上传中...'
      await uploadFile(file, file.name, currentParentId.value, (e) => {
        if (e.total > 0) {
          itemRef.progress = Math.round((e.loaded / e.total) * 100)
        }
      })
      itemRef.progress = 100
      itemRef.status = '上传完成'
    } else {
      // Chunked upload
      itemRef.status = '初始化分片上传...'
      const totalChunks = Math.ceil(file.size / CHUNK_SIZE)
      const initRes = await initChunkedUpload({
        filename: file.name,
        file_size: file.size,
        file_hash: '',
        total_chunks: totalChunks,
        parent_id: currentParentId.value
      })

      if (initRes.code !== 0) {
        itemRef.status = '上传失败'
        return
      }

      const uploadUid = initRes.data.upload_id

      // Upload chunks sequentially
      for (let i = 0; i < totalChunks; i++) {
        const start = i * CHUNK_SIZE
        const end = Math.min(start + CHUNK_SIZE, file.size)
        const chunk = file.slice(start, end)

        itemRef.status = `上传分片 ${i + 1}/${totalChunks}`
        await uploadChunk(uploadUid, i, chunk)
        itemRef.progress = Math.round(((i + 1) / totalChunks) * 100)
      }

      // Complete upload
      itemRef.status = '合并中...'
      await completeChunkedUpload(uploadUid)
      itemRef.progress = 100
      itemRef.status = '上传完成'
    }

    ElMessage.success(`${file.name} 上传成功`)
    loadFiles()
    userStore.loadUserInfo()

    // Auto-close upload dialog after all uploads complete
    const allDone = uploadList.value.every(u => u.status === '上传完成' || u.status === '上传失败')
    if (allDone) {
      setTimeout(() => {
        showUploadDialog.value = false
        uploadList.value = []
      }, 800)
    }
  } catch (e) {
    itemRef.status = '上传失败'
    ElMessage.error(`${file.name} 上传失败`)
  }
}

function handleUploadDialogClose() {
  uploadList.value = []
}

// --- Drag & Drop: Move file into folder ---
function onDragStart(event, file) {
  dragFileId.value = file.id
  event.dataTransfer.effectAllowed = 'move'
  event.dataTransfer.setData('text/plain', String(file.id))
  // Add a slight visual cue
  if (event.target) event.target.style.opacity = '0.5'
  // Reset dragover after drag ends
  const onDragEnd = () => {
    dragFileId.value = -1
    dragOverId.value = -1
    if (event.target) event.target.style.opacity = ''
    document.removeEventListener('dragend', onDragEnd)
  }
  document.addEventListener('dragend', onDragEnd)
}

function onDragOver(event, file) {
  if (file.is_dir && file.id !== dragFileId.value) {
    event.dataTransfer.dropEffect = 'move'
    dragOverId.value = file.id
  }
}

function onDragLeave(file) {
  if (dragOverId.value === file.id) {
    dragOverId.value = -1
  }
}

async function onDrop(event, targetFile) {
  dragOverId.value = -1
  if (!targetFile.is_dir) return
  const fileId = Number(event.dataTransfer.getData('text/plain'))
  if (!fileId || fileId === targetFile.id) return
  // Don't move a folder into itself
  const sourceFile = fileList.value.find(f => f.id === fileId)
  if (!sourceFile) return
  await doMoveFile(sourceFile, targetFile.id)
}

async function onDropOnRow(event, targetFile) {
  dragOverId.value = -1
  if (!targetFile.is_dir) return
  const fileId = Number(event.dataTransfer.getData('text/plain'))
  if (!fileId || fileId === targetFile.id) return
  const sourceFile = fileList.value.find(f => f.id === fileId)
  if (!sourceFile) return
  await doMoveFile(sourceFile, targetFile.id)
}

async function doMoveFile(file, targetFolderId) {
  try {
    const res = await apiMove({
      file_id: file.id,
      new_parent_id: targetFolderId
    })
    if (res.code === 0) {
      ElMessage.success(`已将 "${file.filename}" 移入文件夹`)
      loadFiles()
    }
  } catch (e) {}
}

function getListRowClass({ row }) {
  if (row.is_dir && dragOverId.value === row.id) return 'drop-target-row'
  if (dragFileId.value === row.id) return 'dragging-row'
  return ''
}

function handleContextMenu(file, event) {
  // For future right-click menu
}

function formatSize(bytes) {
  return formatFileSize(bytes)
}

function getIconClass(file) {
  if (file.is_dir) return 'folder'
  const ext = file.filename.split('.').pop().toLowerCase()
  if (['jpg', 'jpeg', 'png', 'gif', 'webp', 'bmp'].includes(ext)) return 'image'
  if (['mp4', 'avi', 'mkv', 'mov', 'webm'].includes(ext)) return 'video'
  if (['zip', 'rar', '7z', 'tar', 'gz'].includes(ext)) return 'archive'
  return 'document'
}
</script>
