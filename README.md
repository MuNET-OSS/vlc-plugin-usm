# VLC USM 解复用插件

这是一个 VLC 3 的 CRIWARE USM 解复用插件。它会从 USM 文件的 `@SFV`
chunk 中取出 VP9 视频帧，按配置的 CRIWARE key 解密，然后交给 VLC 自带的
VP9 解码器播放。

当前支持：

- 直接打开 `.dat` / USM 文件。
- 加密 VP9 视频解密。
- 从默认 key 文件读取密钥，适合在文件管理器里双击打开。
- 基于时间轴拖动进度。

## 安装

构建完成后将libusm_plugin.dll放入以下路径：

```
# VLC Windows版默认安装路径：
C:\Program Files\VideoLAN\VLC\plugins\demux（经测试暂不可用）

# MSYS2版VLC默认安装路径：
C:\msys64\mingw64\lib\vlc\plugins\demux
```

安装后若不识别请手动删除 `plugins.dat` 并使用
```powershell
& "C:\Program Files\VideoLAN\VLC\vlc-cache-gen.exe" ./plugins
```
重建缓存。~~（貌似没用）~~

## 配置密钥

~~推荐把 key 写到默认配置文件里，这样双击文件打开时也能自动解密：~~

```sh
mkdir -p ~/.config/vlc
printf '0x7F4551499DF55E68\n' > ~/.config/vlc/usm-keys.txt
```

~~默认读取顺序：~~

~~1. `$XDG_CONFIG_HOME/vlc/usm-keys.txt`~~
~~2. `~/.config/vlc/usm-keys.txt`~~

~~key 文件支持十进制或 `0x` 前缀的十六进制；多个 key 可以用逗号、分号、
空白或换行分隔。`#` 后面的内容会当作注释。~~
（Windows下未经测试）


也可以启动时临时传 key：

```powershell
& "\path\to\vlc\vlc.exe" --usm-keys 0x7F4551499DF55E68 \path\to\movie.dat
```

或者指定一个自定义 key 文件：

```powershell
& "\path\to\vlc\vlc.exe" --usm-key-file \path\to\usm-keys.txt \path\to\movie.dat
```

## 构建
~~我不知道，我用vscode cmake插件构建的~~
构建前请先修改 `CMakeList' 中的 'set(ENV{PKG_CONFIG_PATH} "")` 为你自己的 `pkgconfig` 路径

## 已知限制

- 当前只输出视频流，没有输出音频流。
- seek 通过从文件头重扫到目标时间附近的 VP9 keyframe 实现；大文件拖进度时
  可能会比普通容器慢一些。
