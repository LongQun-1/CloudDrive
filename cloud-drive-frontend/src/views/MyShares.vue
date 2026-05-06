<template>
  <div>
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
      <h3 style="margin: 0;">我的分享</h3>
    </div>

    <el-table :data="shareList" v-loading="loading" empty-text="暂无分享">
      <el-table-column label="文件名" min-width="200">
        <template #default="{ row }">
          {{ row.file_name || '-' }}
        </template>
      </el-table-column>
      <el-table-column label="分享链接" min-width="150">
        <template #default="{ row }">
          <el-link type="primary" @click="copyShareLink(row)">{{ row.share_url }}</el-link>
        </template>
      </el-table-column>
      <el-table-column label="提取码" width="100">
        <template #default="{ row }">
          {{ row.need_code ? row.share_code : '无' }}
        </template>
      </el-table-column>
      <el-table-column label="浏览/下载" width="120">
        <template #default="{ row }">
          {{ row.view_count }} / {{ row.download_count }}
        </template>
      </el-table-column>
      <el-table-column label="创建时间" width="180">
        <template #default="{ row }">
          {{ formatDate(row.created_at) }}
        </template>
      </el-table-column>
      <el-table-column label="状态" width="100">
        <template #default="{ row }">
          <el-tag :type="row.is_active ? 'success' : 'info'">
            {{ row.is_active ? '有效' : '已取消' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="120">
        <template #default="{ row }">
          <el-button v-if="row.is_active" link type="danger" @click="handleCancel(row)">取消分享</el-button>
        </template>
      </el-table-column>
    </el-table>

    <div style="margin-top: 16px; display: flex; justify-content: flex-end;">
      <el-pagination v-model:current-page="page" v-model:page-size="pageSize"
        :total="total" layout="total, prev, pager, next" @current-change="loadShares" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { listMyShares, cancelShare } from '../api/share'
import { formatDate } from '../utils/helpers'
import { ElMessage, ElMessageBox } from 'element-plus'

const shareList = ref([])
const loading = ref(false)
const page = ref(1)
const pageSize = ref(50)
const total = ref(0)

onMounted(() => {
  loadShares()
})

async function loadShares() {
  loading.value = true
  try {
    const res = await listMyShares({ page: page.value, page_size: pageSize.value })
    if (res.code === 0) {
      shareList.value = res.data || []
      total.value = res.total || 0
    }
  } catch (e) {} finally {
    loading.value = false
  }
}

async function handleCancel(share) {
  try {
    await ElMessageBox.confirm('确定取消此分享？取消后他人将无法访问。', '取消分享', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })
    const res = await cancelShare(share.id)
    if (res.code === 0) {
      ElMessage.success('已取消分享')
      loadShares()
    }
  } catch (e) {}
}

function copyShareLink(share) {
  const url = window.location.origin + '/share/' + share.share_url
  navigator.clipboard.writeText(url + (share.need_code ? `\n提取码: ${share.share_code}` : ''))
    .then(() => ElMessage.success('已复制到剪贴板'))
    .catch(() => ElMessage.error('复制失败'))
}
</script>
