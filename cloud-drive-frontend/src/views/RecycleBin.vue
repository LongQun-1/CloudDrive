<template>
  <div>
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
      <h3 style="margin: 0;">回收站</h3>
      <el-button type="danger" :disabled="fileList.length === 0" @click="handleEmpty">
        清空回收站
      </el-button>
    </div>

    <el-table :data="fileList" v-loading="loading" empty-text="回收站为空">
      <el-table-column label="文件名" min-width="300">
        <template #default="{ row }">
          <div class="file-name-cell">
            <el-icon class="file-icon" :class="row.is_dir ? 'folder' : 'document'">
              <component :is="row.is_dir ? 'Folder' : 'Document'" />
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
      <el-table-column label="删除时间" width="180">
        <template #default="{ row }">
          {{ formatDate(row.deleted_at) }}
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200">
        <template #default="{ row }">
          <el-button link type="primary" @click="handleRestore(row)">恢复</el-button>
          <el-button link type="danger" @click="handlePermanentDelete(row)">永久删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div style="margin-top: 16px; display: flex; justify-content: flex-end;">
      <el-pagination v-model:current-page="page" v-model:page-size="pageSize"
        :total="total" layout="total, prev, pager, next" @current-change="loadFiles" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { listRecycleBin, restoreFile, permanentDelete, emptyRecycleBin } from '../api/recycle'
import { useUserStore } from '../store/user'
import { formatFileSize, formatDate } from '../utils/helpers'
import { ElMessage, ElMessageBox } from 'element-plus'

const userStore = useUserStore()
const fileList = ref([])
const loading = ref(false)
const page = ref(1)
const pageSize = ref(50)
const total = ref(0)

onMounted(() => {
  loadFiles()
})

async function loadFiles() {
  loading.value = true
  try {
    const res = await listRecycleBin({ page: page.value, page_size: pageSize.value })
    if (res.code === 0) {
      fileList.value = res.data || []
      total.value = res.total || 0
    }
  } catch (e) {} finally {
    loading.value = false
  }
}

async function handleRestore(file) {
  try {
    const res = await restoreFile({ file_id: file.id })
    if (res.code === 0) {
      ElMessage.success('已恢复')
      loadFiles()
      userStore.loadUserInfo()
    }
  } catch (e) {}
}

async function handlePermanentDelete(file) {
  try {
    await ElMessageBox.confirm('永久删除后将无法恢复，确定删除？', '永久删除', {
      confirmButtonText: '删除',
      cancelButtonText: '取消',
      type: 'warning'
    })
    const res = await permanentDelete({ file_id: file.id })
    if (res.code === 0) {
      ElMessage.success('已永久删除')
      loadFiles()
    }
  } catch (e) {}
}

async function handleEmpty() {
  try {
    await ElMessageBox.confirm('清空回收站后所有文件将无法恢复，确定？', '清空回收站', {
      confirmButtonText: '清空',
      cancelButtonText: '取消',
      type: 'warning'
    })
    const res = await emptyRecycleBin()
    if (res.code === 0) {
      ElMessage.success('回收站已清空')
      loadFiles()
    }
  } catch (e) {}
}
</script>
