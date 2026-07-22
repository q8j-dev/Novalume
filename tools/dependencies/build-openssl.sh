#!/usr/bin/env bash
set -euo pipefail

revision=8cf17aaeb4599f8af87fefd810b5b5fee90fe69e
version=3.5.7
platform=${1:-}
root=$(cd "$(dirname "$0")/../.." && pwd)
base="$root/out/dependencies/openssl-$version/$platform"
source="$base/source"
prefix="$base/install"
work="/tmp/novalume-openssl-$version-$platform"
build_source="$work/source"
stage_root="$work/stage-root"
logical_prefix="/opt/novalume/openssl-$version/$platform"
stage="$stage_root$logical_prefix"

case "$platform" in
    macos-arm64)
        configure_target=darwin64-arm64-cc
        sdk=macosx
        deployment_target=13.0
        ;;
    ios-arm64)
        configure_target=ios64-xcrun
        sdk=iphoneos
        deployment_target=15.0
        ;;
    linux-x64)
        configure_target=linux-x86_64
        sdk=
        deployment_target=
        ;;
    linux-arm64)
        configure_target=linux-aarch64
        sdk=
        deployment_target=
        ;;
    *)
        echo "usage: $0 macos-arm64|ios-arm64|linux-x64|linux-arm64" >&2
        exit 2
        ;;
esac

if [[ ! -d "$source/.git" ]]; then
    mkdir -p "$base"
    git clone --filter=blob:none --no-checkout https://github.com/openssl/openssl.git "$source"
fi

git -C "$source" fetch --depth=1 origin "$revision"
git -C "$source" checkout --detach "$revision"
export SOURCE_DATE_EPOCH
SOURCE_DATE_EPOCH=$(git -C "$source" show -s --format=%ct "$revision")
export ZERO_AR_DATE=1

if [[ -n "$sdk" ]]; then
    export SDKROOT
    SDKROOT=$(xcrun --sdk "$sdk" --show-sdk-path)
    if [[ "$platform" == macos-* ]]; then
        export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
    else
        export IPHONEOS_DEPLOYMENT_TARGET="$deployment_target"
    fi
fi

cmake -E rm -rf "$work"
mkdir -p "$build_source"
git -C "$source" archive "$revision" | tar -xf - -C "$build_source"

cd "$build_source"
./Configure "$configure_target" \
    --prefix="$logical_prefix" \
    --openssldir="$logical_prefix/ssl" \
    --libdir=lib \
    no-apps no-docs no-dso no-module no-shared no-tests
make -j2
make install_sw DESTDIR="$stage_root"
mkdir -p "$stage/share/licenses/openssl"
cp "$build_source/LICENSE.txt" "$stage/share/licenses/openssl/"

test -f "$stage/include/openssl/evp.h"
test -f "$stage/lib/libcrypto.a"
test -f "$stage/share/licenses/openssl/LICENSE.txt"
cmake -E rm -rf "$prefix"
mkdir -p "$prefix"
cmake -E copy_directory "$stage" "$prefix"
printf '%s\n' "$prefix"
