# third/openssl 说明

本目录的 OpenSSL 产物由 vcpkg 编译后复制而来，供 QHarnesscc 的 `qharness_core`（httplib SSLClient）链接。**已提交进版本管理**（项目自包含，clone 即用）。

## 来源
- **版本**：OpenSSL 3.6.3
- **构建**：vcpkg dynamic（`x64-windows` triplet）；release `/MD`，debug `/MDd`
- **来源 vcpkg**：`E:/vcpkg`（`vcpkg install openssl:x64-windows`，默认同时装 release + debug）

## 结构（release + debug 双 config）
```
third/openssl/
├── include/openssl/   # 头文件（release/debug 共用）
├── lib/               # Release import 库（libcrypto.lib、libssl.lib，/MD）
├── bin/               # Release 运行时 dll（libcrypto-3-x64.dll、libssl-3-x64.dll、legacy.dll）
├── libd/              # Debug import 库（libcrypto.lib、libssl.lib，/MDd）
└── bind/              # Debug 运行时 dll（libcrypto-3-x64.dll、libssl-3-x64.dll、legacy.dll）
```

## CMake 集成
`core/CMakeLists.txt`：`OPENSSL_ROOT_DIR = ${CMAKE_SOURCE_DIR}/third/openssl`（FORCE），手动多 config（`$<IF:$<CONFIG:Debug>,libd/,lib/>` 按 config 选 lib/dll，弃 `find_package` 单 config），post-build 按 config 复制 dll 到 `build/out/<config>/`。Release + Debug 测试均 194/0 通过。

## 版本管理
本目录**已提交**（项目自包含）。`.gitignore` 用 `!third/openssl/**` 例外全局 `*.lib`/`*.dll` 忽略；`.pdb`（调试符号）未含（复制时已删）。

## 重新获取（升级 OpenSSL / 换机器）
```bash
V=<vcpkg-dir>/installed/x64-windows
mkdir -p third/openssl/{libd,bind}
cp -r "$V/include" third/openssl/        # release 头
cp -r "$V/lib"     third/openssl/        # release lib
cp -r "$V/bin"     third/openssl/        # release dll
cp "$V/debug/lib/"libssl.lib "$V/debug/lib/"libcrypto.lib third/openssl/libd/   # debug lib
cp "$V/debug/bin/"*.dll                       third/openssl/bind/              # debug dll
rm -f third/openssl/{bin,lib,bind}/*.pdb                                       # 去 pdb
git add third/openssl/                    # 提交新版本
```
vcpkg 安装注意：GitHub 大文件下载需稳定网络；本机系统代理 `127.0.0.1:7892` 不可用时 `unset HTTP_PROXY HTTPS_PROXY` 直连。
