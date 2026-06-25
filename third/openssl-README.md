# third/openssl 说明

本目录的 OpenSSL 产物（`x64-windows/`）由 vcpkg 编译后复制而来，供 QHarnesscc 的 `qharness_core`（httplib SSLClient）链接。

## 来源
- **版本**：OpenSSL 3.6.3
- **构建**：vcpkg dynamic（`x64-windows` triplet，`/MD` release CRT）
- **来源 vcpkg**：`E:/vcpkg`（`vcpkg install openssl:x64-windows`）

## 结构
```
third/openssl/x64-windows/
├── include/openssl/   # 头文件（ssl.h、crypto.h、opensslv.h 等；release/debug 共用）
├── lib/               # Release import 库（libcrypto.lib、libssl.lib，/MD）
├── bin/               # Release 运行时 dll（libcrypto-3-x64.dll、libssl-3-x64.dll、legacy.dll，/MD）
├── libd/              # Debug import 库（libcrypto.lib、libssl.lib，/MDd）
└── bind/              # Debug 运行时 dll（libcrypto-3-x64.dll、libssl-3-x64.dll、legacy.dll，/MDd）
```

## 被忽略
`third/openssl/` 在 `.gitignore`（不 commit 二进制产物）。clone 后需自行获取（见下）。

## 重新获取（换机器/重装）
1. 装 vcpkg：`git clone https://github.com/microsoft/vcpkg.git <vcpkg-dir>` + `<vcpkg-dir>/bootstrap-vcpkg.bat`。
2. `<vcpkg-dir>/vcpkg.exe install openssl:x64-windows`（注意：GitHub 大文件下载需稳定网络；本机系统代理 `127.0.0.1:7892` 不可用时 `unset HTTP_PROXY HTTPS_PROXY` 直连）。
3. 复制产物到本目录（去 pdb 减体积）：
   ```bash
   mkdir -p third/openssl/x64-windows
   cp -r <vcpkg-dir>/installed/x64-windows/include third/openssl/x64-windows/
   cp -r <vcpkg-dir>/installed/x64-windows/lib     third/openssl/x64-windows/
   cp -r <vcpkg-dir>/installed/x64-windows/bin     third/openssl/x64-windows/
   rm -f third/openssl/x64-windows/{bin,lib}/*.pdb
   ```

## 注意
- **多 config（Release + Debug 均支持）**：`core/CMakeLists.txt` 手动按 `$<IF:$<CONFIG:Debug>,...>` 选 lib/dll——Release 用 `lib/`+`bin/`（`/MD`），Debug 用 `libd/`+`bind/`（`/MDd`），CRT 各自匹配。post-build 按 config 复制对应 dll 到 `build/out/<config>/`。Release 与 Debug 测试均 194 checks/0 failures 通过。
- vcpkg `x64-windows` triplet 默认同时装 release + debug（产物在 `installed/x64-windows/lib`+`bin` 与 `debug/lib`+`debug/bin`）；本目录把 release 放 `lib/`+`bin/`，debug 放 `libd/`+`bind/`（平铺命名）。
- `OPENSSL_ROOT_DIR = ${CMAKE_SOURCE_DIR}/third/openssl/x64-windows`（`core/CMakeLists.txt`，`FORCE`）。

## 重新获取（含 debug）
```bash
mkdir -p third/openssl/x64-windows
V=<vcpkg-dir>/installed/x64-windows
# release
cp -r "$V/include" third/openssl/x64-windows/
cp -r "$V/lib"     third/openssl/x64-windows/
cp -r "$V/bin"     third/openssl/x64-windows/
# debug → libd/ bind/
mkdir -p third/openssl/x64-windows/{libd,bind}
cp "$V/debug/lib/"libssl.lib "$V/debug/lib/"libcrypto.lib third/openssl/x64-windows/libd/
cp "$V/debug/bin/"*.dll                       third/openssl/x64-windows/bind/
rm -f third/openssl/x64-windows/{bin,lib}/*.pdb   # release 去 pdb
```
