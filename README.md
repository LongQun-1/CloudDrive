# CloudDrive 云网盘系统 - 项目开发文档

## 一、项目概述

CloudDrive 是一个支持文件上传下载、秒传去重、分块上传与断点续传、文件分享、回收站等核心功能的云网盘系统。后端基于 C++17 + drogon 高性能 HTTP 框架，前端基于 Vue3 + Element Plus，采用前后端分离架构。

## 二、技术架构

```
┌─────────────────────────────────────────────────┐
│                   用户浏览器                      │
│          Vue3 + Element Plus + Axios             │
└────────────────────┬────────────────────────────┘
                     │ HTTPS
┌────────────────────▼────────────────────────────┐
│               Nginx (反向代理)                     │
│  ┌──────────────┐  ┌──────────────────────────┐  │
│  │ 静态文件服务  │  │  反向代理 /api/ → :8080   │  │
│  └──────────────┘  └──────────────────────────┘  │
└────────────────────┬────────────────────────────┘
                     │ HTTP
┌────────────────────▼────────────────────────────┐
│          Drogon HTTP Server (C++17) :8080        │
│  ┌─────────────────────────────────────────────┐ │
│  │       AuthMiddleware + RateLimitMiddleware    │ │
│  ├──────────┬──────────┬──────────┬────────────┤ │
│  │UserController│FileController│ShareCtrl│RecycleCtrl│ │
│  ├──────────┴──────────┴──────────┴────────────┤ │
│  │  AuthService │ JwtUtil │ FileUtil │ HashUtil │ │
│  ├─────────────────────────────────────────────┤ │
│  │            RedisPlugin (hiredis)             │ │
│  ├─────────────────────┬───────────────────────┤ │
│  │      MySQL 8.0      │     Redis 6.0         │ │
│  └─────────────────────┴───────────────────────┘ │
└─────────────────────────────────────────────────┘
```

### 技术栈

| 层级 | 技术 | 版本 | 用途 |
|------|------|------|------|
| 后端框架 | Drogon | 1.9.12 | 高性能 C++ HTTP 框架 |
| 语言标准 | C++ | 17 | std::filesystem, std::optional, 结构化绑定 |
| 数据库 | MySQL | 8.0 | 数据持久化 |
| 缓存 | Redis | 6.0 | Refresh Token 存储、限流计数器 |
| 密码哈希 | bcrypt (glibc crypt_r) | $2b$12$ | 安全密码存储 |
| JWT | OpenSSL HMAC-SHA256 | 自实现 | 无状态身份认证 |
| 文件哈希 | OpenSSL EVP (SHA256) | - | 秒传去重、文件完整性校验 |
| 前端框架 | Vue | 3.4 | Composition API + SFC |
| UI 组件库 | Element Plus | 2.7 | 企业级 Vue3 组件库 |
| 构建工具 | Vite | 5.3 | 快速 HMR + 生产构建 |
| 状态管理 | Pinia | 2.1 | Vue3 官方状态管理 |
| 路由 | Vue Router | 4.3 | History 模式 + 导航守卫 |
| HTTP 客户端 | Axios | 1.7 | 请求拦截 + Token 自动刷新 |
| Web 服务器 | Nginx | 1.18 | 静态文件 + 反向代理 + HTTPS |
| SSL | Let's Encrypt (certbot) | 1.21 | HTTPS 证书自动续期 |
| 进程管理 | systemd | - | 后端服务自启 + 崩溃重启 |

## 三、项目结构

### 3.1 后端目录结构

```
cloud-drive/
├── CMakeLists.txt              # 构建配置
├── config/
│   └── config.json             # drogon 运行配置
├── sql/
│   └── file_tables.sql         # 数据库建表脚本
├── main.cpp                    # 入口：配置加载、中间件注册、启动服务
├── controllers/                # 控制器层：路由处理
│   ├── UserController.h/cpp    # 用户模块（7 个接口）
│   ├── FileController.h/cpp    # 文件模块（13 个接口）
│   ├── ShareController.h/cpp   # 分享模块（7 个接口）
│   └── RecycleController.h/cpp # 回收站模块（4 个接口）
├── models/                     # 数据模型层：SQL 操作
│   ├── User.h/cpp              # 用户模型
│   ├── File.h/cpp              # 文件模型
│   └── Share.h/cpp             # 分享模型
├── services/                   # 业务服务层
│   └── AuthService.h/cpp       # 认证服务（密码哈希 + JWT + Token 管理）
├── middlewares/                 # 中间件
│   ├── AuthMiddleware.h/cpp    # JWT 鉴权中间件
│   └── RateLimitMiddleware.h/cpp # 限流中间件
├── plugins/                    # drogon 插件
│   └── RedisPlugin.h/cpp       # Redis 封装（hiredis + mutex 线程安全）
└── utils/                      # 工具类
    ├── JwtUtil.h/cpp           # JWT 生成/验证（OpenSSL HMAC-SHA256）
    ├── HashUtil.h/cpp          # SHA256/MD5 哈希（OpenSSL EVP）
    ├── FileUtil.h/cpp          # 文件操作（std::filesystem + POSIX）
    ├── ValidatorUtil.h/cpp     # 输入校验
    ├── CodeUtil.h/cpp          # 随机码生成
    └── ResponseUtil.h/cpp      # 统一响应格式
```

### 3.2 前端目录结构

```
cloud-drive-frontend/
├── package.json
├── vite.config.js              # Vite 配置 + 开发代理
├── index.html
├── src/
│   ├── main.js                 # 入口
│   ├── App.vue                 # 根组件
│   ├── style.css               # 全局样式
│   ├── router/
│   │   └── index.js            # 路由定义 + 导航守卫
│   ├── store/
│   │   └── user.js             # Pinia 用户状态管理
│   ├── utils/
│   │   ├── request.js          # Axios 封装 + Token 拦截器
│   │   └── helpers.js          # 工具函数
│   ├── api/
│   │   ├── user.js             # 用户 API
│   │   ├── file.js             # 文件 API
│   │   ├── share.js            # 分享 API
│   │   └── recycle.js          # 回收站 API
│   └── views/
│       ├── Login.vue           # 登录页
│       ├── Register.vue        # 注册页
│       ├── Layout.vue          # 主布局（侧边栏+顶栏+内容）
│       ├── FileList.vue        # 文件列表（核心页面）
│       ├── Settings.vue        # 用户设置
│       ├── MyShares.vue        # 我的分享
│       ├── RecycleBin.vue      # 回收站
│       └── share/
│           └── SharePage.vue   # 公开分享页（无需登录）
```

## 四、数据库设计

### 4.1 user 表

| 字段 | 类型 | 说明 |
|------|------|------|
| id | BIGINT PK AUTO_INCREMENT | 用户 ID |
| username | VARCHAR(50) UNIQUE | 用户名 |
| password_hash | VARCHAR(255) | bcrypt 哈希 |
| nickname | VARCHAR(50) | 昵称 |
| avatar | VARCHAR(255) | 头像 URL |
| storage_used | BIGINT DEFAULT 0 | 已用存储（字节） |
| storage_limit | BIGINT DEFAULT 1073741824 | 存储上限（默认 1GB） |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

### 4.2 file 表

| 字段 | 类型 | 说明 |
|------|------|------|
| id | BIGINT PK AUTO_INCREMENT | 文件 ID |
| user_id | BIGINT INDEX | 所属用户 |
| parent_id | BIGINT DEFAULT 0 | 父目录 ID（0=根目录） |
| filename | VARCHAR(255) | 文件名 |
| file_hash | VARCHAR(64) INDEX | SHA256 哈希（秒传/去重） |
| file_size | BIGINT DEFAULT 0 | 文件大小（字节） |
| file_path | VARCHAR(512) | 物理存储路径 |
| is_dir | TINYINT DEFAULT 0 | 是否目录 |
| is_deleted | TINYINT DEFAULT 0 | 软删除标记 |
| ref_count | INT DEFAULT 1 | 引用计数（去重） |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| deleted_at | DATETIME | 删除时间 |

### 4.3 share 表

| 字段 | 类型 | 说明 |
|------|------|------|
| id | BIGINT PK AUTO_INCREMENT | 分享 ID |
| share_url | VARCHAR(12) UNIQUE | 分享标识码 |
| share_code | VARCHAR(4) | 验证码（可选） |
| user_id | BIGINT | 分享者 |
| file_id | BIGINT | 分享的文件 |
| need_code | TINYINT DEFAULT 0 | 是否需要验证码 |
| expire_at | DATETIME NULL | 过期时间 |
| max_download_count | INT DEFAULT -1 | 最大下载次数（-1=无限） |
| view_count | INT DEFAULT 0 | 浏览次数 |
| download_count | INT DEFAULT 0 | 下载次数 |
| save_count | INT DEFAULT 0 | 保存次数 |
| is_active | TINYINT DEFAULT 1 | 是否有效 |
| created_at | DATETIME | 创建时间 |

### 4.4 chunk 表（分块上传任务）

| 字段 | 类型 | 说明 |
|------|------|------|
| id | BIGINT PK AUTO_INCREMENT | 任务 ID |
| upload_id | VARCHAR(64) UNIQUE | 上传会话标识 |
| user_id | BIGINT | 上传者 |
| file_hash | VARCHAR(64) | 文件哈希 |
| filename | VARCHAR(255) | 文件名 |
| file_size | BIGINT | 总大小 |
| total_chunks | INT | 总分块数 |
| uploaded_chunks | TEXT | 已上传分块 CSV（如 "0,1,2"） |
| parent_id | BIGINT DEFAULT 0 | 目标目录 |
| is_completed | TINYINT DEFAULT 0 | 是否完成 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

### 4.5 索引设计

```sql
-- file 表索引
KEY idx_user_parent (user_id, parent_id)      -- 文件列表查询
KEY idx_user_deleted (user_id, is_deleted)    -- 回收站查询
KEY idx_file_hash (file_hash)                 -- 秒传哈希查找
KEY idx_user_updated (user_id, updated_at)    -- 搜索排序

-- share 表索引
UNIQUE KEY uk_share_url (share_url)           -- 分享链接唯一
KEY idx_user_active (user_id, is_active)      -- 我的分享列表

-- chunk 表索引
UNIQUE KEY uk_upload_id (upload_id)           -- 上传任务唯一
KEY idx_user_completed (user_id, is_completed) -- 进度查询
```

## 五、后端模块详解

### 5.1 用户模块 `/api/user/`

| 接口 | 方法 | 路径 | 认证 | 功能 |
|------|------|------|------|------|
| 注册 | POST | /register | 否 | 创建用户，bcrypt 哈希密码 |
| 登录 | POST | /login | 否 | 验证密码，签发 JWT 双 Token |
| 刷新 | POST | /refresh | 否 | 用 refresh_token 换新 access_token |
| 登出 | POST | /logout | 是 | 删除 Redis 中的 refresh_token |
| 用户信息 | GET | /info | 是 | 获取用户信息 + 存储占比 |
| 修改信息 | PUT | /info | 是 | 修改昵称/头像 |
| 修改密码 | PUT | /password | 是 | 验证旧密码，更新密码哈希 |

**认证流程**：

1. **注册**：校验用户名（3-50 字符，字母数字下划线）和密码（≥8 字符，含字母和数字），使用 glibc `crypt_r()` 进行 bcrypt 哈希（cost factor 12），存入数据库。
2. **登录**：验证密码后签发双 Token——Access Token（30 分钟）和 Refresh Token（7 天），Refresh Token 存入 Redis。登录失败统一返回"用户名或密码错误"，不泄露用户名是否存在。
3. **Token 刷新**：Refresh Token 采用一次性使用策略——验证旧 token 的 JWT 签名和 Redis 存在性，从 Redis 删除旧 token，签发新的双 Token 并存回 Redis。即使 Refresh Token 泄露，攻击者只能使用一次，原主再次刷新时会发现 token 已被吊销。
4. **鉴权中间件**：AuthMiddleware 绑定在路由级别，从 `Authorization: Bearer <token>` 提取 JWT，验证签名和过期时间，将 user_id 和 username 存入 `req->attributes()` 供下游 Controller 使用。跳过 OPTIONS 请求（CORS 预检）。

**JWT 自实现**（JwtUtil）：

```
Header  = {"alg":"HS256","typ":"JWT"}          → Base64URL 编码
Payload = {"user_id":1,"type":"access","iat":...,"exp":...} → Base64URL 编码
Signature = HMAC-SHA256(headerB64.payloadB64, secret) → Base64URL 编码
Token = headerB64.payloadB64.signatureB64
```

- 使用 OpenSSL `HMAC()` 函数签名
- payload 中 `type` 字段区分 access/refresh，`verifyAccessToken` 只接受 type=access
- 验证时重算签名比对 + 检查 exp 过期时间

### 5.2 文件模块 `/api/file/`

| 接口 | 方法 | 路径 | 认证 | 功能 |
|------|------|------|------|------|
| 创建文件夹 | POST | /folder | 是 | 在指定目录下创建文件夹 |
| 简单上传 | POST | /upload | 是 | 小文件直接上传（二进制 body） |
| 分块初始化 | POST | /chunk/init | 是 | 创建分块上传任务 |
| 分块上传 | POST | /chunk/upload | 是 | 上传单个分块 |
| 分块合并 | POST | /chunk/complete | 是 | 合并所有分块为完整文件 |
| 下载 | GET | /download/{id} | 是 | 下载文件 |
| 文件列表 | GET | /list | 是 | 分页列出目录内容 |
| 文件信息 | GET | /info/{id} | 是 | 获取单个文件元数据 |
| 搜索 | GET | /search | 是 | 按文件名模糊搜索 |
| 上传进度 | GET | /chunk/progress/{id} | 是 | 查询分块上传进度 |
| 重命名 | PUT | /rename | 是 | 修改文件名 |
| 移动 | PUT | /move | 是 | 移动文件到新目录 |
| 删除 | DELETE | /delete | 是 | 软删除（进回收站） |

#### 5.2.1 简单上传

前端将文件作为 raw binary 放入 request body，filename 和 parent_id 通过 query params 传递。后端使用 `req->getBody()` 获取二进制数据（drogon 1.9.12 服务端无 `getFiles()` API）。

流程：
1. 检查用户存储限额
2. 计算文件 SHA256 哈希
3. 查找同哈希文件是否已存在 → 秒传
4. 存储到 `<storageDir>/<hash前2位>/<hash完整值>`
5. 创建文件记录，更新用户存储用量

#### 5.2.2 秒传/去重

基于 SHA256 内容寻址：相同内容的文件哈希值相同。

- **上传时**：先计算哈希，在 file 表中查找是否已存在。如果存在，创建新的文件记录指向同一物理文件，`ref_count++`，不传文件内容。
- **删除时**：`ref_count--`，只有 `ref_count = 0` 时才删除物理文件，避免秒传/去重场景下误删其他用户引用的同一文件。

#### 5.2.3 分块上传与断点续传

大文件（≥5MB）采用分块上传，三阶段协议：

**阶段一 init**：客户端发送文件元信息（文件名、大小、哈希、总分块数），后端先检查秒传（哈希命中则直接返回），否则生成 `upload_id`，在 chunk 表创建任务记录，创建临时目录。

**阶段二 upload**：逐块上传，每块 5MB，chunk 数据放在 body，upload_id 和 chunk_index 通过 query params 传递。后端将分块存为 `<tmpDir>/<uploadId>/<chunkIndex>.chunk`，更新 chunk 表的 `uploaded_chunks` 字段（CSV 格式）。

**阶段三 complete**：验证所有分块已上传，按序合并分块为最终文件，流式计算合并后文件的 SHA256 用于完整性校验，创建文件记录，清理临时文件。

**断点续传**：客户端通过 `/chunk/progress/{uploadId}` 查询已上传的分块列表，跳过已传分块，仅上传缺失部分。

#### 5.2.4 SHA256 流式计算

使用 OpenSSL EVP API 进行流式哈希计算，8KB 缓冲区：

```cpp
EVP_MD_CTX* ctx = EVP_MD_CTX_new();
EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
// 可多次调用 EVP_DigestUpdate 喂数据
EVP_DigestFinal_ex(ctx, hash, &hashLen);
```

- **简单上传**：直接计算内存中的 body 数据
- **分块上传合并**：逐个读取 .chunk 文件，流式喂入 EVP_Update，内存占用始终为 8KB，可处理任意大小文件

#### 5.2.5 路径安全

- `FileUtil::safeFilePath`：检查 `..`，用 `std::filesystem::weakly_canonical` 彄一化路径，验证最终路径在 baseDir 之下
- `ValidatorUtil::isValidFilename`：拒绝 `/`、`\`、`:`、`*` 等特殊字符和控制字符

### 5.3 分享模块 `/api/share/`

| 接口 | 方法 | 路径 | 认证 | 功能 |
|------|------|------|------|------|
| 创建分享 | POST | /create | 是 | 生成分享链接 + 验证码 |
| 验证分享 | POST | /verify | 否 | 验证码校验 |
| 分享信息 | GET | /info/{url} | 否 | 获取分享元数据 |
| 文件夹内容 | GET | /files/{url} | 否 | 列出分享文件夹内容 |
| 下载分享 | GET | /download/{url} | 否 | 下载分享文件 |
| 我的分享 | GET | /list | 是 | 列出用户的分享 |
| 取消分享 | PUT | /cancel | 是 | 设为无效 |

**分享链接**：share_url 为 12 位随机字母数字字符串，share_code 为 4 位随机数字（可选）。

**分享验证流程**：
- 无验证码：直接获取文件信息
- 有验证码：先调用 /verify 验证，再获取/下载
- 每次查看 `view_count++`，每次下载 `download_count++`
- 检查：is_active、是否过期、下载次数是否超限

**文件夹分享**：通过 `/files/{url}` 接口浏览目录内容，支持 parent_id 进入子目录，下载时需指定 file_id。

### 5.4 回收站模块 `/api/recycle/`

| 接口 | 方法 | 路径 | 认证 | 功能 |
|------|------|------|------|------|
| 回收站列表 | GET | /list | 是 | 列出已删除文件 |
| 恢复 | PUT | /restore | 是 | 恢复文件（支持批量） |
| 永久删除 | DELETE | /delete | 是 | 物理删除单个文件 |
| 清空 | DELETE | /empty | 是 | 清空所有已删除文件 |

- **软删除**：删除文件时设置 `is_deleted = 1`，文件大小从用户 `storage_used` 扣除；恢复时重新加回。
- **物理删除安全**：永久删除时检查 `ref_count`，只有该哈希无其他引用时才删除物理文件。

### 5.5 中间件

#### AuthMiddleware
- 从 `Authorization: Bearer <token>` 提取 JWT
- 调用 `AuthService::verifyAccessToken` 验证签名和有效期
- 将 user_id 和 username 存入 `req->attributes()`
- 路由级别绑定：在 `ADD_METHOD_TO` 中用字符串名称指定

#### RateLimitMiddleware
- 基于 Redis 的计数限流（INCR + EXPIRE）
- 按客户端 IP + 请求路径限流
- 支持 X-Real-IP / X-Forwarded-For 代理场景
- 限流策略：
  - 注册/登录：5 次/分钟
  - 分块上传：100 次/分钟
  - 分享验证：10 次/分钟
  - 文件下载：10 次/分钟
  - 默认：60 次/分钟
- 超限返回 429

### 5.6 drogon 框架注意事项

1. **中间件注册**：`HttpMiddleware<T, false>` 第二个模板参数 false 表示不自动创建，需手动 new 并 `registerMiddleware`，在 `ADD_METHOD_TO` 中用字符串名称绑定
2. **Attributes 类型**：`asInt64()` 返回 `int64_t`（Linux 上 = long），`get<long long>()` 会抛异常，须用 `get<int64_t>()` 或 `get<long>()`
3. **Field::as\<T\>**：drogon 1.9.12 无 `as<int64_t>` 特化，读 BIGINT 列用 `as<long long>()`
4. **result 遍历**：`auto& row = result[0]` 编译失败（非常量引用绑定临时对象），须用 `auto row = result[0]`
5. **getParameter**：drogon 1.9.12 `getParameter(key)` 只接受 1 个参数，无默认值版；用 `getParameters().find()` 手动处理
6. **文件上传**：drogon 1.9.12 服务端无 `getFiles()` API，使用 `req->getBody()` 获取 raw body
7. **Json::Value 赋值**：`data["key"] = row["col"].as<long long>()` 歧义，须显式 `(Json::Value::Int64)` 转型
8. **body.data() 类型**：`string_view::data()` 返回 `const char*`，传给 `sha256(const unsigned char*, size_t)` 须 `reinterpret_cast`

## 六、前端实现

### 6.1 路由与导航守卫

| 路径 | 组件 | 认证 |
|------|------|------|
| /login | Login.vue | 仅未登录 |
| /register | Register.vue | 仅未登录 |
| / | FileList.vue | 需登录 |
| /settings | Settings.vue | 需登录 |
| /shares | MyShares.vue | 需登录 |
| /recycle | RecycleBin.vue | 需登录 |
| /share/:shareUrl | SharePage.vue | 公开 |

导航守卫逻辑：
- 需认证页面 + 未登录 → 跳转 /login
- 仅访客页面 + 已登录 + 非 public → 跳转 /
- public 页面 → 允许访问

### 6.2 Token 自动刷新

Axios 响应拦截器检测 401 或 code=1002 时触发刷新：
- `isRefreshing` 标志保证只刷新一次
- 并发请求通过 `subscribeTokenRefresh` 排队
- 刷新完成后 `onTokenRefreshed` 统一回调
- 刷新失败清除本地存储，跳转登录页

### 6.3 文件上传前端策略

- **< 5MB**：简单上传，整个文件作为 binary body 发送
- **≥ 5MB**：分块上传，前端按 5MB 切片，调用 init → upload(逐块) → complete
- **秒传**：前端使用 spark-md5 计算 SHA256，init 时发送 hash，后端判断是否已存在
- **上传进度**：使用 Axios `onUploadProgress` 回调
- **拖拽操作**：HTML5 Drag & Drop API 实现拖拽文件到文件夹

## 七、构建与部署

### 7.1 后端构建

#### 依赖安装

```bash
# Ubuntu 20.04
sudo apt update
sudo apt install -y build-essential cmake libssl-dev libhiredis-dev \
    libjsoncpp-dev uuid-dev zlib1g-dev libmysqlclient-dev libcrypt-dev

# 安装 drogon
git clone https://github.com/drogonframework/drogon.git
cd drogon && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) && sudo make install
```

#### 编译项目

```bash
cd cloud-drive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

编译产物为 `build/cloud-drive` 可执行文件。

#### 配置文件

`config/config.json` 需根据部署环境修改：

```json
{
    "app": {
        "listen": "0.0.0.0",
        "port": 8080
    },
    "db": {
        "host": "127.0.0.1",
        "port": 3306,
        "dbname": "cloud_drive",
        "user": "<数据库用户名>",
        "passwd": "<数据库密码>"
    },
    "redis": {
        "host": "127.0.0.1",
        "port": 6379
    },
    "custom_config": {
        "storage": {
            "path": "/data/cloud-drive/files",
            "tmp_path": "/data/cloud-drive/tmp",
            "max_file_size": 10737418240
        },
        "jwt": {
            "secret": "<JWT密钥>",
            "access_token_expire_seconds": 1800,
            "refresh_token_expire_seconds": 604800
        }
    }
}
```

### 7.2 数据库初始化

```bash
# 创建数据库和用户
mysql -u root -p
CREATE DATABASE cloud_drive CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER '<用户名>'@'localhost' IDENTIFIED BY '<密码>';
GRANT ALL PRIVILEGES ON cloud_drive.* TO '<用户名>'@'localhost';
FLUSH PRIVILEGES;

# 执行建表脚本
mysql -u <用户名> -p cloud_drive < sql/file_tables.sql
```

注意：SQL 脚本中的 file 表和 upload_task 表为初始设计，实际代码使用的表结构见本文档第四章，需根据实际表结构创建 user、share、chunk 表并调整 file 表字段。

### 7.3 前端构建

```bash
cd cloud-drive-frontend
npm install
npm run build        # 产出 dist/ 目录
```

### 7.4 Nginx 配置

```nginx
# HTTP → HTTPS 重定向
server {
    listen 80;
    server_name <域名>;
    return 301 https://<域名>$request_uri;
}

# 主服务
server {
    listen 443 ssl;
    server_name <域名>;
    root /var/www/cloud-drive;

    ssl_certificate     /etc/letsencrypt/live/<域名>/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/<域名>/privkey.pem;

    # Vue Router history 模式
    location / {
        try_files $uri $uri/ /index.html;
    }

    # API 反向代理
    location /api/ {
        proxy_pass http://127.0.0.1:8080/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        client_max_body_size 500m;
    }

    # Gzip 压缩
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml;

    # 静态资源缓存
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff2?)$ {
        expires 7d;
        add_header Cache-Control "public, immutable";
    }
}
```

### 7.5 HTTPS 证书

```bash
# 安装 certbot
sudo apt install certbot python3-certbot-nginx

# 申请证书
sudo certbot --nginx -d <域名>

# 自动续期（certbot.timer 自动启用）
sudo systemctl status certbot.timer
```

### 7.6 后端 systemd 服务

```ini
# /etc/systemd/system/cloud-drive.service
[Unit]
Description=CloudDrive Backend Service
After=network.target mysql.service redis.service

[Service]
Type=simple
User=<系统用户>
WorkingDirectory=<项目路径>/build
ExecStart=<项目路径>/build/cloud-drive
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

管理命令：

```bash
sudo systemctl enable cloud-drive   # 开机自启
sudo systemctl start cloud-drive    # 启动
sudo systemctl stop cloud-drive     # 停止
sudo systemctl restart cloud-drive  # 重启
sudo systemctl status cloud-drive   # 状态
journalctl -u cloud-drive -f        # 实时日志
```

### 7.7 前端部署

```bash
# 本地构建
cd cloud-drive-frontend && npm run build

# 打包上传（排除 macOS 资源文件）
tar czf dist.tar.gz --exclude='._*' dist/
scp dist.tar.gz <用户>@<服务器>:/tmp/

# 服务器解压
sudo tar xzf /tmp/dist.tar.gz -C /var/www/cloud-drive/ --strip-components=1
```

### 7.8 目录结构（服务器）

```
/data/cloud-drive/
├── files/          # 文件存储目录
│   ├── ab/         # 哈希前两位子目录
│   │   └── ab3f7c...
│   └── ...
├── tmp/            # 分块上传临时目录
│   └── <uploadId>/
│       ├── 0.chunk
│       ├── 1.chunk
│       └── ...
└── logs/           # 运行日志

/var/www/cloud-drive/   # 前端静态文件
```

## 八、API 测试

### 8.1 用户模块测试

```bash
# 注册
curl -X POST http://localhost:8080/api/user/register \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"Test1234","nickname":"test"}'

# 登录
curl -X POST http://localhost:8080/api/user/login \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"Test1234"}'

# 获取用户信息（需替换 <token>）
curl http://localhost:8080/api/user/info \
  -H "Authorization: Bearer <access_token>"

# 刷新 Token
curl -X POST http://localhost:8080/api/user/refresh \
  -H "Content-Type: application/json" \
  -d '{"refresh_token":"<refresh_token>"}'
```

### 8.2 文件模块测试

```bash
# 创建文件夹
curl -X POST http://localhost:8080/api/file/folder \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"folder_name":"test_dir","parent_id":0}'

# 简单上传
curl -X POST "http://localhost:8080/api/file/upload?filename=test.txt&parent_id=0" \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @test.txt

# 分块上传 - 初始化
curl -X POST http://localhost:8080/api/file/chunk/init \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"filename":"large.zip","file_size":104857600,"file_hash":"<sha256>","total_chunks":20,"parent_id":0}'

# 分块上传 - 上传分块
curl -X POST "http://localhost:8080/api/file/chunk/upload?upload_id=<upload_id>&chunk_index=0" \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @chunk_0.bin

# 分块上传 - 查询进度
curl "http://localhost:8080/api/file/chunk/progress/<upload_id>" \
  -H "Authorization: Bearer <token>"

# 分块上传 - 完成
curl -X POST http://localhost:8080/api/file/chunk/complete \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"upload_id":"<upload_id>"}'

# 文件列表
curl "http://localhost:8080/api/file/list?parent_id=0&page=1&page_size=50" \
  -H "Authorization: Bearer <token>"

# 下载文件
curl -o downloaded.file "http://localhost:8080/api/file/download/<file_id>" \
  -H "Authorization: Bearer <token>"

# 搜索
curl "http://localhost:8080/api/file/search?keyword=test&page=1&page_size=50" \
  -H "Authorization: Bearer <token>"

# 重命名
curl -X PUT http://localhost:8080/api/file/rename \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_id":1,"new_name":"renamed.txt"}'

# 移动
curl -X PUT http://localhost:8080/api/file/move \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_id":1,"new_parent_id":2}'

# 删除（进回收站）
curl -X DELETE http://localhost:8080/api/file/delete \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_id":1}'
```

### 8.3 分享模块测试

```bash
# 创建分享
curl -X POST http://localhost:8080/api/share/create \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_id":1,"need_code":true,"expire_at":"2026-12-31 23:59:59"}'

# 获取分享信息（公开）
curl http://localhost:8080/api/share/info/<share_url>

# 验证分享
curl -X POST http://localhost:8080/api/share/verify \
  -H "Content-Type: application/json" \
  -d '{"share_url":"<share_url>","share_code":"1234"}'

# 下载分享文件
curl -o shared.file "http://localhost:8080/api/share/download/<share_url>?code=1234&file_id=1"
```

### 8.4 回收站模块测试

```bash
# 回收站列表
curl http://localhost:8080/api/recycle/list?page=1&page_size=50 \
  -H "Authorization: Bearer <token>"

# 恢复
curl -X PUT http://localhost:8080/api/recycle/restore \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_ids":[1,2,3]}'

# 永久删除
curl -X DELETE http://localhost:8080/api/recycle/delete \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"file_id":1}'

# 清空
curl -X DELETE http://localhost:8080/api/recycle/empty \
  -H "Authorization: Bearer <token>"
```

### 8.5 健康检查

```bash
curl http://localhost:8080/health
# 返回: {"version":"1.0.0","mysql":"ok","redis":"degraded"}
```

## 九、项目统计

- 后端源文件：31 个（16 .h + 15 .cpp），核心代码约 4500 行
- 前端源文件：18 个（8 .vue + 10 .js），核心代码约 2200 行
- REST API：32 个（含健康检查）
  - 用户模块：7 个
  - 文件模块：13 个
  - 分享模块：7 个
  - 回收站模块：4 个
  - 健康检查：1 个
