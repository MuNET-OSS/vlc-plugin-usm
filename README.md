# VLC USM 解复用插件

这是一个 VLC 3 的 CRIWARE USM 解复用插件。它会从 USM 文件的 `@SFV`
chunk 中取出 VP9 视频帧，按配置的 CRIWARE key 解密，然后交给 VLC 自带的
VP9 解码器播放。

当前支持：

- 直接打开 `.dat` / USM 文件。
- 加密 VP9 视频解密。
- 从默认 key 文件读取密钥，适合在文件管理器里双击打开。
- 基于时间轴拖动进度。

## 安装 Arch 包

仓库根目录已经可以生成 Arch 包：

```sh
cd packaging/arch
PKGDEST=../.. makepkg -f
cd ../..
sudo pacman -U vlc-plugin-usm-0.1.0-3-x86_64.pkg.tar.zst
```

安装后，VLC 的插件缓存会由 pacman hook 自动更新。

## 配置密钥

推荐把 key 写到默认配置文件里，这样双击文件打开时也能自动解密：

```sh
mkdir -p ~/.config/vlc
printf '0x7F4551499DF55E68\n' > ~/.config/vlc/usm-keys.txt
```

默认读取顺序：

1. `$XDG_CONFIG_HOME/vlc/usm-keys.txt`
2. `~/.config/vlc/usm-keys.txt`

key 文件支持十进制或 `0x` 前缀的十六进制；多个 key 可以用逗号、分号、
空白或换行分隔。`#` 后面的内容会当作注释。

也可以启动时临时传 key：

```sh
vlc --usm-keys 0x7F4551499DF55E68 /path/to/movie.dat
```

或者指定一个自定义 key 文件：

```sh
vlc --usm-key-file /path/to/usm-keys.txt /path/to/movie.dat
```

## 构建和测试

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 本地调试插件

可以不安装到系统，先用本地插件目录测试：

```sh
mkdir -p build/vlc-plugins/demux
cp build/libusm_plugin.so build/vlc-plugins/demux/
/usr/lib/vlc/vlc-cache-gen build/vlc-plugins

VLC_PLUGIN_PATH="$PWD/build/vlc-plugins" vlc --no-plugins-cache \
    --usm-keys 0x7F4551499DF55E68 \
    /path/to/movie.dat
```

如果本地插件目录和系统已安装插件同时存在，VLC 可能仍然选中系统插件。做最终
验证时建议安装新包后再测试。

## 已知限制

- 当前只输出视频流，没有输出音频流。
- seek 通过从文件头重扫到目标时间附近的 VP9 keyframe 实现；大文件拖进度时
  可能会比普通容器慢一些。
