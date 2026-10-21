-- 学生考勤管理系统 数据库初始化脚本
-- 用法: sqlite3 demodb.db3 < db_init.sql

CREATE TABLE IF NOT EXISTS departments (
    departID   INTEGER PRIMARY KEY,
    department VARCHAR(50) NOT NULL);

CREATE TABLE IF NOT EXISTS majors (
    majorID INTEGER PRIMARY KEY,
    major   VARCHAR(50) NOT NULL);

CREATE TABLE IF NOT EXISTS studInfo (
    studID        INTEGER PRIMARY KEY,
    name          VARCHAR(20) NOT NULL,
    gender        VARCHAR(4),
    departID      INTEGER REFERENCES departments(departID),
    majorID       INTEGER REFERENCES majors(majorID),
    face_features BLOB,   -- 1024 维 float 特征
    face_image    TEXT);  -- 人脸图片路径

CREATE TABLE IF NOT EXISTS attendance (
    attID      INTEGER PRIMARY KEY AUTOINCREMENT,
    studID     INTEGER NOT NULL REFERENCES studInfo(studID),
    attDate    DATE NOT NULL,
    attTime    TIME NOT NULL,
    status     VARCHAR(10) DEFAULT 'present', -- present/late/absent/leave
    confidence REAL,
    face_image TEXT);

CREATE TABLE IF NOT EXISTS att_settings (
    key   VARCHAR(50) PRIMARY KEY,
    value TEXT);

INSERT OR IGNORE INTO att_settings VALUES ('Fr_Threshold', '0.65');
INSERT OR IGNORE INTO att_settings VALUES ('camera_id', '0');

-- 示例数据
INSERT OR IGNORE INTO departments VALUES (1, '信息与通信工程学院');
INSERT OR IGNORE INTO departments VALUES (2, '计算机科学与工程学院');
INSERT OR IGNORE INTO majors VALUES (1, '通信工程');
INSERT OR IGNORE INTO majors VALUES (2, '计算机科学与技术');
INSERT OR IGNORE INTO studInfo (studID, name, gender, departID, majorID) VALUES
    (2025091601018, '示例学生', '男', 1, 1);
