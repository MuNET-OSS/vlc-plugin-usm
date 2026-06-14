# VLC USM 解复用插件

这是一个 VLC 3 的 CRIWARE USM 解复用插件。它会从 USM 文件的 `@SFV`
chunk 中取出 VP9 视频帧，按配置的 CRIWARE key 解密，然后交给 VLC 自带的
VP9 解码器播放。

当前支持：

- 直接打开 `.dat` / USM 文件。
- 加密 VP9 视频解密。
- 从默认 key 文件读取密钥，适合在文件管理器里双击打开。
- 基于时间轴拖动进度。

## 构建

### Linux

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows (MSYS2 / MinGW64)

在 MSYS2 的 **MINGW64** shell 里安装依赖并构建：

```sh
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkgconf mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-libvpx mingw-w64-x86_64-vlc

cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

从 MINGW64 shell 构建时 `pkg-config` 会自动找到 `vlc-plugin` 和 `vpx`，无需手动配置
路径。若在其它环境构建，用环境变量 `PKG_CONFIG_PATH` 或 `-DCMAKE_PREFIX_PATH` 指定。

## 安装

### Arch 包

```sh
cd packaging/arch
PKGDEST=../.. makepkg -f
cd ../..
sudo pacman -U vlc-plugin-usm-0.1.0-3-x86_64.pkg.tar.zst
```

安装后 VLC 的插件缓存会由 pacman hook 自动更新。

### Windows

把构建出的 `libusm_plugin.dll` 放进 VLC 的 `plugins/demux` 目录：

- MSYS2 版 VLC：`C:\msys64\mingw64\lib\vlc\plugins\demux\`
- 官网版 VLC：`C:\Program Files\VideoLAN\VLC\plugins\demux\`

> 注意：本插件依赖 libvpx，MSYS2 构建出的 DLL 会动态链接 `libvpx-1.dll` 以及
> mingw 运行时（`libgcc_s_seh-1.dll`、`libwinpthread-1.dll`）。MSYS2 版 VLC 因为
> 这些 DLL 在 `mingw64\bin`（已在 PATH 上）能正常加载；官网版 VLC 不自带这些 DLL，
> 直接拷过去会因找不到依赖而静默加载失败。让官网版可用的方案仍在完善中。

安装后若不识别，删除缓存 `plugins.dat` 并重建：

```powershell
& "C:\Program Files\VideoLAN\VLC\vlc-cache-gen.exe" "C:\Program Files\VideoLAN\VLC\plugins"
```

## 配置密钥

推荐把 key 写到 VLC 配置目录下的默认文件里，这样双击文件打开时也能自动解密。

Linux：

```sh
mkdir -p ~/.config/vlc
printf '0x7F4551499DF55E68\n' > ~/.config/vlc/usm-keys.txt
```

Windows（官网版 VLC）：

```powershell
New-Item -ItemType Directory -Force "$env:APPDATA\vlc" | Out-Null
Set-Content "$env:APPDATA\vlc\usm-keys.txt" "0x7F4551499DF55E68"
```

默认读取顺序：

- Windows：`%APPDATA%\vlc\usm-keys.txt`
- Linux：`$XDG_CONFIG_HOME/vlc/usm-keys.txt`，否则 `~/.config/vlc/usm-keys.txt`

key 文件支持十进制或 `0x` 前缀的十六进制；多个 key 可以用逗号、分号、空白或换行
分隔。`#` 后面的内容会当作注释。配置多个 key 时，插件会用 libvpx 试解码自动选出
能正确解密的那个。

也可以启动时临时传 key：

```sh
vlc --usm-keys 0x7F4551499DF55E68 /path/to/movie.dat
```

```powershell
& "C:\Program Files\VideoLAN\VLC\vlc.exe" --usm-keys 0x7F4551499DF55E68 C:\path\to\movie.dat
```

或者指定一个自定义 key 文件：

```sh
vlc --usm-key-file /path/to/usm-keys.txt /path/to/movie.dat
```

## 已知限制

- 当前只输出视频流，没有输出音频流。
- seek 通过从文件头重扫到目标时间附近的 VP9 keyframe 实现；大文件拖进度时
  可能会比普通容器慢一些。
