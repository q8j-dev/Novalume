#!/usr/bin/env bash
set -euo pipefail

revision=8cf17aaeb4599f8af87fefd810b5b5fee90fe69e
version=3.5.7
platform=${1:-}
root=$(cd "$(dirname "$0")/../.." && pwd)
base="$root/out/dependencies/openssl-$version/$platform"
source="$base/source"
prefix="$base/install"

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

if [[ -n "$sdk" ]]; then
    export SDKROOT
    SDKROOT=$(xcrun --sdk "$sdk" --show-sdk-path)
    if [[ "$platform" == macos-* ]]; then
        export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
    else
        export IPHONEOS_DEPLOYMENT_TARGET="$deployment_target"
    fi
fi

cd "$source"
./Configure "$configure_target" \
    --prefix="$prefix" \
    --openssldir="$prefix/ssl" \
    --libdir=lib \
    no-apps no-docs no-dso no-module no-shared no-tests
if command -v nproc >/dev/null 2>&1; then
    build_jobs=$(nproc)
else
    build_jobs=$(sysctl -n hw.logicalcpu)
fi
make -j "$build_jobs"
make install_sw

test -f "$prefix/include/openssl/evp.h"
test -f "$prefix/lib/libcrypto.a"
printf '%s\n' "$prefix"
