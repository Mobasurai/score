#!/bin/bash -eux

# Note: this is run from an ArchLinux container due to too recent glibc
pacman -Syyu --noconfirm
pacman -S wget ninja cmake tar sed clang lld pkgconf double-conversion libb2 fuse2 --noconfirm

export SDK_ARCHIVE=sdk-rpi-aarch64.tar.xz
wget -nv https://github.com/ossia/score-sdk/releases/download/sdk28/$SDK_ARCHIVE -O $SDK_ARCHIVE
mkdir -p /opt/ossia-sdk-rpi-aarch64
tar xaf $SDK_ARCHIVE --strip-components=2 --directory /opt/ossia-sdk-rpi-aarch64/

wget -nv https://github.com/ossia/score-sdk/releases/download/sdk28/icu72.tar.gz -O icu72.tar.gz
tar xaf icu*.tar.gz
mv libicu* /usr/lib/

rm *.tar.*

source ci/common.deps.sh
