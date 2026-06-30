# PrismLauncher-custom 构建说明

## 已完成的工作

✅ **Yggdrasil 第三方账号登录功能已实现**

### 新增文件
1. `launcher/minecraft/auth/steps/YggdrasilStep.h` - Yggdrasil 认证步骤头文件
2. `launcher/minecraft/auth/steps/YggdrasilStep.cpp` - Yggdrasil 认证步骤实现
3. `launcher/ui/dialogs/YggdrasilLoginDialog.h` - 登录对话框头文件
4. `launcher/ui/dialogs/YggdrasilLoginDialog.cpp` - 登录对话框实现
5. `launcher/ui/dialogs/YggdrasilLoginDialog.ui` - 登录对话框 UI

### 修改文件
1. `launcher/minecraft/auth/AccountData.h` - 添加 Yggdrasil 类型枚举
2. `launcher/minecraft/auth/AccountData.cpp` - 添加 YggdrasilToken 序列化/反序列化
3. `launcher/minecraft/auth/MinecraftAccount.h` - 添加 createYggdrasil 工厂方法声明
4. `launcher/minecraft/auth/MinecraftAccount.cpp` - 添加 createYggdrasil 工厂方法实现
5. `launcher/minecraft/auth/AuthFlow.cpp` - 添加 Yggdrasil 认证流程处理
6. `launcher/ui/pages/global/AccountListPage.h` - 添加 on_actionAddYggdrasil_triggered 方法声明
7. `launcher/ui/pages/global/AccountListPage.cpp` - 添加 on_actionAddYggdrasil_triggered 方法实现
8. `launcher/ui/pages/global/AccountListPage.ui` - 添加 "Add Third-party" 按钮
9. `launcher/CMakeLists.txt` - 添加新文件到构建系统

## 功能说明

- 支持 Yggdrasil 认证协议（Minecraft Java Edition 第三方服务器）
- 支持自定义认证服务器 URL（可选）
- 在账户列表页面添加了 "Add Third-party" 按钮
- 登录对话框包含用户名、密码和服务器 URL 输入框

## 构建环境要求

### 必需软件
1. **CMake** (已安装: v4.3.3)
   - 路径: `C:\Program Files\CMake\bin\cmake.exe`

2. **C++ 编译器** (需要手动安装)
   - 推荐: MinGW-w64 (GCC 14.2.0 或更高版本)
   - 或: MSVC (Visual Studio 2019/2022 Build Tools)

3. **Qt 6** (需要手动安装)
   - 版本: Qt 6.5 或更高版本
   - 组件: Qt Core, Qt GUI, Qt Widgets, Qt Network, Qt Concurrent

4. **其他依赖**
   - zlib
   - OpenSSL
   - cmark (Markdown 解析库)

### 安装编译器（选择一种）

#### 方案 A: 安装 MinGW-w64
1. 访问 https://github.com/brechtsanders/winlibs_mingw/releases
2. 下载 `posix-seh-gcc-14.2.0-mingw-w64ucrt-14.0.0-r1.zip`
3. 解压到 `C:\mingw64`
4. 将 `C:\mingw64\bin` 添加到系统 PATH

#### 方案 B: 安装 Visual Studio Build Tools
1. 下载 Visual Studio Build Tools 2022
2. 安装 "C++ build tools" 工作负载
3. 使用 "x64 Native Tools Command Prompt" 编译

### 安装 Qt 6
1. 访问 https://www.qt.io/download-qt-installer
2. 下载 Qt 在线安装程序
3. 安装 Qt 6.5 或更高版本
4. 选择 "MSVC 2019 64-bit" 或 "MinGW 64-bit" 组件

## 构建步骤

### 使用 MinGW 编译

```bash
# 1. 打开 MSYS2 MinGW 64-bit 终端（或设置好环境变量的终端）

# 2. 进入项目目录
cd C:/Users/29073/miclaw/project/PrismLauncher-custom

# 3. 创建构建目录
mkdir build
cd build

# 4. 运行 CMake 配置
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/mingw_64"

# 5. 编译
cmake --build . --config Release -j8

# 6. 安装（可选）
cmake --install .
```

### 使用 MSVC 编译

```bash
# 1. 打开 "x64 Native Tools Command Prompt for VS 2022"

# 2. 进入项目目录
cd C:\Users\29073\miclaw\project\PrismLauncher-custom

# 3. 创建构建目录
mkdir build
cd build

# 4. 运行 CMake 配置
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\msvc2019_64"

# 5. 编译
cmake --build . --config Release

# 6. 安装（可选）
cmake --install . --config Release
```

## 常见问题

### 1. CMake 找不到 Qt
```
CMake Error: Could not find a package configuration file provided by "Qt6" with any of the following names: Qt6Config.cmake
```

**解决方案**: 设置 `CMAKE_PREFIX_PATH` 指向 Qt 安装目录
```bash
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/mingw_64"
```

### 2. 编译器找不到
```
CMake Error: CMAKE_C_COMPILER not found
```

**解决方案**: 确保编译器已安装并在 PATH 中
```bash
# 检查 GCC
gcc --version

# 检查 MSVC
cl
```

### 3. 链接错误
```
undefined reference to `xxx`
```

**解决方案**: 确保所有依赖库已正确安装

## 验证安装

编译成功后，可执行文件将位于：
- MinGW: `build/launcher/PrismLauncher.exe`
- MSVC: `build/launcher/Release/PrismLauncher.exe`

运行程序，点击 "Add Third-party" 按钮测试 Yggdrasil 登录功能。

## 技术支持

如遇到构建问题，请检查：
1. 所有依赖是否正确安装
2. 环境变量是否正确设置
3. CMake 版本是否兼容（推荐 3.20+）
4. Qt 版本是否匹配（推荐 6.5+）
