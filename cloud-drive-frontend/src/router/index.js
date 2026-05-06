import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/Login.vue'),
    meta: { title: '登录', guest: true }
  },
  {
    path: '/register',
    name: 'Register',
    component: () => import('../views/Register.vue'),
    meta: { title: '注册', guest: true }
  },
  {
    path: '/',
    component: () => import('../views/Layout.vue'),
    meta: { requiresAuth: true },
    children: [
      {
        path: '',
        name: 'Home',
        component: () => import('../views/FileList.vue'),
        meta: { title: '我的文件' }
      },
      {
        path: 'settings',
        name: 'Settings',
        component: () => import('../views/Settings.vue'),
        meta: { title: '个人设置' }
      },
      {
        path: 'shares',
        name: 'MyShares',
        component: () => import('../views/MyShares.vue'),
        meta: { title: '我的分享' }
      },
      {
        path: 'recycle',
        name: 'RecycleBin',
        component: () => import('../views/RecycleBin.vue'),
        meta: { title: '回收站' }
      }
    ]
  },
  {
    path: '/share/:shareUrl',
    name: 'SharePage',
    component: () => import('../views/share/SharePage.vue'),
    meta: { title: '分享', public: true }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// Navigation guard
router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('access_token')
  
  if (to.meta.requiresAuth && !token) {
    next('/login')
  } else if (to.meta.guest && token && !to.meta.public) {
    next('/')
  } else {
    next()
  }
})

export default router
