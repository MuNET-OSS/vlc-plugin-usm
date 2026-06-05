# VLC USM demux plugin

This is a VLC 3 demux plugin for CRIWARE USM files. It extracts encrypted VP9
payloads from `@SFV` chunks, tries keys from a configurable list, and hands VP9
frames to VLC's native decoder.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Try with a local VLC plugin cache

```sh
mkdir -p build/vlc-plugins/demux
cp build/libusm_plugin.so build/vlc-plugins/demux/
/usr/lib/vlc/vlc-cache-gen build/vlc-plugins
vlc --no-plugins-cache --plugin-path "$PWD/build/vlc-plugins" \
    --usm-keys 0x7F4551499DF55E68 \
    /home/clansty/Downloads/KALEID_/KALEID_MV_MAS.dat
```

On recent VLC builds, use `VLC_PLUGIN_PATH` if `--plugin-path` is unavailable:

```sh
VLC_PLUGIN_PATH="$PWD/build/vlc-plugins" vlc --no-plugins-cache \
    --usm-keys 0x7F4551499DF55E68 \
    /home/clansty/Downloads/KALEID_/KALEID_MV_MAS.dat
```

## Arch package

```sh
cd packaging/arch
PKGDEST=../.. makepkg -f
cd ../..
sudo pacman -U vlc-plugin-usm-0.1.0-1-x86_64.pkg.tar.zst
```

`--usm-keys` accepts comma, semicolon, whitespace, or newline separated keys.
Each key can be decimal or hexadecimal with a `0x` prefix.

`--usm-key-file` points to a text file using the same key format. Lines can
include comments after `#`.
