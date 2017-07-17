# Maintainer: jun7 <jun7@hush.com>

pkgname=wyebadblock
pkgver=1.1
pkgrel=1
branch=master
pkgdesc="A adblock extension for wyeb, also webkit2gtk browsers may be."
arch=('x86_64')
license=('GPL')
url="https://github.com/jun7/wyebadblock"
depends=('webkit2gtk')
makedepends=('git')
source=("git://github.com/jun7/wyebadblock.git#branch=$branch")
md5sums=('SKIP')

pkgver(){
	cd "$srcdir/wyebadblock"
	printf "$pkgver.%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

prepare() {
	cd "$srcdir/wyebadblock"
	git pull --rebase origin $branch
}

build() {
	cd "$srcdir/wyebadblock"
	make
}

package() {
	cd "$srcdir/wyebadblock"
	install -Dm755 adblock.so   "$pkgdir/usr/share/wyebrowser/adblock.so"
}
