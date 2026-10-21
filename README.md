# 学生考勤管理系统（Attendance_System）

电子科技大学《开源软件》课程项目。基于 **Qt Widgets + SQLite + SeetaFace6 + OpenCV** 的人脸识别考勤系统。

## 功能

- **人脸打卡**：摄像头实时预览，检测 → 质量评估（亮度/分辨率/清晰度/完整性/姿态）→ 特征提取 → 相似度比对，支持 2 秒间隔自动打卡
- **人脸注册**：从摄像头采集人脸，写入特征 BLOB 到数据库
- **考勤记录**：按学号/状态/日期范围检索，手动增删改（OnManualSubmit），关闭前提示未保存修改
- **统计**：注册人数、今日出勤、出勤率、每人出勤/迟到/缺勤/请假汇总
- 防重复打卡（每人每天一次）；一键重置所有数据（需确认）

## 构建（Windows / Qt 6 + MSVC）

### 依赖

- Qt 6（MSVC 套件）、Visual Studio
- OpenCV 4.12（`opencv_world4120`）
- [SeetaFace6](https://github.com/SeetaFace6Open/index) Windows SDK（sf6.0）
- SeetaFace 模型文件（`face_detector.csta` 等，放入模型目录）

### SDK 路径

工程默认 SDK 根目录为 `D:/claude code/program_kaggle/program_from_czw/facial_recognition`，
如路径不同，编译前设置环境变量：

```bat
set SEETA_SDK_ROOT=D:/path/to/facial_recognition
```

该目录结构：`opencv/build`、`sf6.0_windows/sf6.0_windows`、`sf3.0_models/sf3.0_models`（模型）。

模型目录在运行时按以下顺序探测：`<exe目录>/models` → SDK 默认位置。也可把模型复制到 exe 旁的 `models/` 子目录实现绿色部署。

### 编译

```bat
build_msvc.bat       （自动调用 qmake + nmake）
```

或手动：

```bat
qmake attendance.pro
nmake -f Makefile.Debug
```

## 数据库表结构

- `studInfo`：学生表，扩展 `face_features BLOB`（1024 维特征）、`face_image TEXT`
- `attendance(attID, studID→studInfo, attDate, attTime, status, confidence, face_image)`
- `att_settings`：阈值等配置（识别阈值默认 0.65）

## 目录说明

| 文件 | 说明 |
|---|---|
| `facerecognizer.cpp` | SeetaFace6 引擎封装：模型加载/摄像头/检测/特征/比对 |
| `camerawidget.cpp` | 摄像头预览控件 |
| `registerdialog.cpp` | 人脸注册对话框 |
| `attendancedialog.cpp` | 考勤记录编辑对话框 |
| `mainwindow.cpp` | 主窗口：打卡/注册/记录/统计四个 Tab |
| `attendance.pro` | qmake 工程文件 |
