#!/usr/bin/env bash
set -euo pipefail

version=8.1.2
archive_hash=464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c
platform=${1:-}
root=$(cd "$(dirname "$0")/../.." && pwd)
base="$root/out/dependencies/ffmpeg-$version/$platform"
archive="$base/ffmpeg-$version.tar.xz"
prefix="$base/install"
temporary_root=${TMPDIR:-/tmp}
temporary_root=${temporary_root%/}
work="$temporary_root/novalume-ffmpeg-$version-$platform"
source="$work/source"
build="$work/build"
logical_prefix="/opt/novalume/ffmpeg-$version/$platform"
stage_root="$work/stage-root"
stage="$stage_root$logical_prefix"

case "$platform" in
    macos-arm64)
        arch=arm64
        target_os=darwin
        sdk=macosx
        deployment_target=13.0
        deployment_flag="-mmacosx-version-min=$deployment_target"
        linkage_options=(--enable-shared --disable-static --install-name-dir=@rpath)
        required_library=libavformat.dylib
        export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
        cross_options=(--disable-cross-compile)
        tool_options=()
        feature_options=()
        extra_cflags="-arch $arch $deployment_flag"
        extra_ldflags="-arch $arch $deployment_flag -Wl,-headerpad_max_install_names"
        ;;
    ios-arm64)
        arch=arm64
        target_os=darwin
        sdk=iphoneos
        deployment_target=15.0
        deployment_flag="-miphoneos-version-min=$deployment_target"
        linkage_options=(--disable-shared --enable-static)
        required_library=libavformat.a
        export IPHONEOS_DEPLOYMENT_TARGET="$deployment_target"
        cross_options=(--enable-cross-compile)
        tool_options=()
        feature_options=()
        extra_cflags="-arch $arch $deployment_flag"
        extra_ldflags="-arch $arch $deployment_flag -Wl,-headerpad_max_install_names"
        ;;
    emscripten-wasm32)
        arch=wasm32
        target_os=none
        linkage_options=(--disable-shared --enable-static)
        required_library=libavformat.a
        cross_options=(--enable-cross-compile)
        tool_options=(--cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm=emnm --strip=emstrip)
        feature_options=(--disable-asm --disable-inline-asm --enable-pthreads --disable-w32threads --disable-os2threads)
        extra_cflags="-pthread"
        extra_ldflags="-pthread"
        ;;
    *)
        echo "usage: $0 macos-arm64|ios-arm64|emscripten-wasm32" >&2
        exit 2
        ;;
esac

mkdir -p "$base"
if [[ ! -f "$archive" ]]; then
    curl --fail --location --proto '=https' --tlsv1.2 \
        "https://ffmpeg.org/releases/ffmpeg-$version.tar.xz" \
        --output "$archive"
fi

actual_hash=$(shasum -a 256 "$archive" | awk '{print $1}')
if [[ "$actual_hash" != "$archive_hash" ]]; then
    echo "FFmpeg archive hash mismatch" >&2
    exit 1
fi

if [[ ! -x "$source/configure" ]]; then
    mkdir -p "$source"
    tar -xJf "$archive" --strip-components=1 -C "$source"
fi

cmake -E rm -rf "$build" "$stage_root"
mkdir -p "$build"
if [[ "$target_os" == darwin ]]; then
    export SDKROOT
    SDKROOT=$(xcrun --sdk "$sdk" --show-sdk-path)
    cc=$(xcrun --sdk "$sdk" --find clang)
    tool_options+=("--cc=$cc")
    if [[ "$platform" == ios-* ]]; then
        cross_options+=("--sysroot=$SDKROOT")
    fi
fi

cd "$build"
"$source/configure" \
    --prefix="$logical_prefix" \
    --arch="$arch" \
    --target-os="$target_os" \
    "${tool_options[@]}" \
    "${cross_options[@]}" \
    "${linkage_options[@]}" \
    "${feature_options[@]}" \
    --enable-pic \
    --disable-gpl \
    --disable-nonfree \
    --disable-version3 \
    --disable-programs \
    --disable-doc \
    --disable-debug \
    --disable-network \
    --disable-autodetect \
    --disable-encoders \
    --disable-muxers \
    --disable-devices \
    --disable-filters \
    --disable-avdevice \
    --disable-avfilter \
    --disable-hwaccels \
    --extra-cflags="$extra_cflags -ffile-prefix-map=$source=ffmpeg-$version/source -fdebug-prefix-map=$source=ffmpeg-$version/source -ffile-prefix-map=$build=ffmpeg-$version/build -fdebug-prefix-map=$build=ffmpeg-$version/build" \
    --extra-ldflags="$extra_ldflags"
sed -i '' "s|$source|ffmpeg-$version/source|g; s|$build|ffmpeg-$version/build|g" config.h
make -j2
make install DESTDIR="$stage_root"
mkdir -p "$stage/share/licenses/ffmpeg"
cp "$source/LICENSE.md" "$source/COPYING.LGPLv2.1" "$stage/share/licenses/ffmpeg/"
mkdir -p "$stage/share/sources/ffmpeg"
cp "$archive" "$stage/share/sources/ffmpeg/"

test -f "$stage/include/libavformat/avformat.h"
test -f "$stage/lib/$required_library"
test -f "$stage/share/licenses/ffmpeg/COPYING.LGPLv2.1"
test -f "$stage/share/sources/ffmpeg/ffmpeg-$version.tar.xz"
cmake -E rm -rf "$prefix"
mkdir -p "$prefix"
/usr/bin/ditto "$stage" "$prefix"
printf '%s\n' "$prefix"
