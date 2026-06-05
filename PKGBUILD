# Maintainer: clansty
pkgname=vlc-plugin-usm
pkgver=0.1.0
pkgrel=1
pkgdesc='VLC demux plugin for CRIWARE USM video files'
arch=('x86_64')
url='https://example.invalid/vlc-plugin-usm'
license=('LGPL-2.1-or-later')
depends=('vlc' 'libvpx')
makedepends=('cmake' 'pkgconf')
_srcdir='/home/clansty/Projects/Arcade/vlc-plugin-usm'
source=("git+file://${_srcdir}")
sha256sums=('SKIP')

build() {
  cmake -S "${srcdir}/${pkgname}" -B build \
    -DCMAKE_BUILD_TYPE=None \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_SKIP_RPATH=ON
  cmake --build build
}

check() {
  ctest --test-dir build --output-on-failure
}

package() {
  DESTDIR="${pkgdir}" cmake --install build
}
