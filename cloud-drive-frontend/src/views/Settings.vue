<template>
  <div>
    <el-card>
      <template #header>
        <span>个人信息</span>
      </template>
      <el-form :model="userForm" label-width="100px" style="max-width: 500px;">
        <el-form-item label="用户名">
          <el-input :model-value="userStore.userInfo?.username" disabled />
        </el-form-item>
        <el-form-item label="昵称">
          <el-input v-model="userForm.nickname" placeholder="请输入昵称" />
        </el-form-item>
        <el-form-item label="头像">
          <el-avatar :size="64">{{ userStore.userInfo?.nickname?.[0] || 'U' }}</el-avatar>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="saving" @click="handleSaveInfo">保存修改</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <el-card style="margin-top: 20px;">
      <template #header>
        <span>存储空间</span>
      </template>
      <div class="storage-bar">
        <el-progress :percentage="userStore.storagePercent" :color="storageColor" :stroke-width="20"
          :format="() => `${userStore.storagePercent}%`" />
        <div class="storage-info">
          <span>已使用 {{ formatFileSize(userStore.userInfo?.storage_used || 0) }}</span>
          <span>总共 {{ formatFileSize(userStore.userInfo?.storage_limit || 0) }}</span>
        </div>
      </div>
    </el-card>

    <el-card style="margin-top: 20px;">
      <template #header>
        <span>修改密码</span>
      </template>
      <el-form ref="pwdFormRef" :model="pwdForm" :rules="pwdRules" label-width="100px" style="max-width: 500px;">
        <el-form-item label="当前密码" prop="oldPassword">
          <el-input v-model="pwdForm.oldPassword" type="password" show-password />
        </el-form-item>
        <el-form-item label="新密码" prop="newPassword">
          <el-input v-model="pwdForm.newPassword" type="password" show-password />
        </el-form-item>
        <el-form-item label="确认密码" prop="confirmPassword">
          <el-input v-model="pwdForm.confirmPassword" type="password" show-password />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="changingPwd" @click="handleChangePassword">修改密码</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useUserStore } from '../store/user'
import { updateUserInfo, changePassword } from '../api/user'
import { formatFileSize } from '../utils/helpers'
import { ElMessage } from 'element-plus'

const userStore = useUserStore()
const saving = ref(false)
const changingPwd = ref(false)
const pwdFormRef = ref(null)

const userForm = reactive({
  nickname: ''
})

const pwdForm = reactive({
  oldPassword: '',
  newPassword: '',
  confirmPassword: ''
})

const validateConfirm = (rule, value, callback) => {
  if (value !== pwdForm.newPassword) {
    callback(new Error('两次输入密码不一致'))
  } else {
    callback()
  }
}

const pwdRules = {
  oldPassword: [{ required: true, message: '请输入当前密码', trigger: 'blur' }],
  newPassword: [
    { required: true, message: '请输入新密码', trigger: 'blur' },
    { min: 8, message: '密码至少8位', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z])(?=.*\d)/, message: '需包含字母和数字', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请确认新密码', trigger: 'blur' },
    { validator: validateConfirm, trigger: 'blur' }
  ]
}

const storageColor = computed(() => {
  const pct = userStore.storagePercent
  if (pct >= 90) return '#f56c6c'
  if (pct >= 70) return '#e6a23c'
  return '#409eff'
})

onMounted(() => {
  userForm.nickname = userStore.userInfo?.nickname || ''
})

async function handleSaveInfo() {
  saving.value = true
  try {
    const res = await updateUserInfo({ nickname: userForm.nickname })
    if (res.code === 0) {
      ElMessage.success('修改成功')
      userStore.loadUserInfo()
    }
  } catch (e) {} finally {
    saving.value = false
  }
}

async function handleChangePassword() {
  const valid = await pwdFormRef.value.validate().catch(() => false)
  if (!valid) return

  changingPwd.value = true
  try {
    const res = await changePassword({
      old_password: pwdForm.oldPassword,
      new_password: pwdForm.newPassword
    })
    if (res.code === 0) {
      ElMessage.success('密码修改成功，请重新登录')
      userStore.clearAuth()
      window.location.href = '/login'
    }
  } catch (e) {} finally {
    changingPwd.value = false
  }
}
</script>
