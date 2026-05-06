-- 文件表
CREATE TABLE IF NOT EXISTS `file` (
    `id` BIGINT NOT NULL AUTO_INCREMENT,
    `user_id` BIGINT NOT NULL COMMENT '所属用户ID',
    `parent_id` BIGINT NOT NULL DEFAULT 0 COMMENT '父目录ID, 0表示根目录',
    `filename` VARCHAR(255) NOT NULL COMMENT '文件名',
    `file_type` TINYINT NOT NULL DEFAULT 0 COMMENT '0=文件, 1=文件夹',
    `file_size` BIGINT NOT NULL DEFAULT 0 COMMENT '文件大小(字节)',
    `file_hash` VARCHAR(64) DEFAULT NULL COMMENT '文件SHA256哈希(用于秒传/去重)',
    `storage_path` VARCHAR(512) DEFAULT NULL COMMENT '实际存储路径',
    `mime_type` VARCHAR(128) DEFAULT NULL COMMENT 'MIME类型',
    `is_deleted` TINYINT NOT NULL DEFAULT 0 COMMENT '0=正常, 1=已删除(回收站)',
    `deleted_at` DATETIME DEFAULT NULL COMMENT '删除时间',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_user_parent` (`user_id`, `parent_id`),
    KEY `idx_user_deleted` (`user_id`, `is_deleted`),
    KEY `idx_file_hash` (`file_hash`),
    KEY `idx_user_updated` (`user_id`, `updated_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='文件表';

-- 上传任务表 (断点续传)
CREATE TABLE IF NOT EXISTS `upload_task` (
    `id` BIGINT NOT NULL AUTO_INCREMENT,
    `user_id` BIGINT NOT NULL,
    `upload_id` VARCHAR(64) NOT NULL COMMENT '上传唯一标识',
    `filename` VARCHAR(255) NOT NULL,
    `file_size` BIGINT NOT NULL DEFAULT 0,
    `file_hash` VARCHAR(64) DEFAULT NULL,
    `total_chunks` INT NOT NULL DEFAULT 0 COMMENT '总分块数',
    `uploaded_chunks` VARCHAR(2048) DEFAULT '' COMMENT '已上传的分块索引(逗号分隔)',
    `parent_id` BIGINT NOT NULL DEFAULT 0,
    `status` TINYINT NOT NULL DEFAULT 0 COMMENT '0=上传中, 1=已完成, 2=已取消',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_upload_id` (`upload_id`),
    KEY `idx_user_status` (`user_id`, `status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='上传任务表';
