# Third-party notices

This file covers dependencies in the default modernization graph.

| Component | Revision | License | Use |
|---|---|---|---|
| bgfx | `759bdeb936ea95e4ac13d1ba8d4ce2e91c5c17d2` | BSD-2-Clause | rendering abstraction |
| bx | `c98e98cde15498250fedbc41ad6da75a896fbae0` | BSD-2-Clause | bgfx support library |
| bimg | `c3cf9afef058f99846de57050c74b183815d5ee7` | BSD-2-Clause | image/texture support |
| bgfx.cmake | `572868c0cb952add48019d267223453958e958b8` | CC0-1.0 | source build integration |
| miniaudio 0.11.25 | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | MIT-0 | device-independent shared audio mixer, decoding, and spatialization |
| FFmpeg 8.1.2 | `464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c` | LGPL-2.1-or-later with IJG terms | media demuxing, decoding, pixel conversion, and audio resampling |
| GameNetworkingSockets 1.5.0 | `4fbfe83ef4d59a12dc32baeff2c33e511af93157` | BSD-3-Clause | encrypted native UDP transport |
| protobuf 35.1 | `35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03` | BSD-3-Clause | GameNetworkingSockets schema/runtime dependency |
| Abseil 20250512.1 | `76bb24329e8bf5f39704eb10d21b9a80befa7c81` | Apache-2.0 | protobuf support library |
| OpenSSL 3.5.7 LTS | `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e` | Apache-2.0 | non-Windows GameNetworkingSockets cryptography |
| curl | `68720b4837284335b2d63cb358f8f6ce65f5bc55` | curl | HTTP/HTTPS client transport |
| zstd 1.5.7 | `v1.5.7` | BSD-3-Clause OR GPL-2.0-only | modern place/model compression |
| Luau | `5e76a0a162d82cbe13a09a4beacd09d7e53cc856` | MIT | current script VM and bytecode runtime |
| Draco 1.5.7 | `8786740086a9f4d83f44aa83badfbea4dce7a1b5` | Apache-2.0 | compressed mesh decoding |
| Boost | retained source snapshot | BSL-1.0 | portable C++ support runtime |
| LZ4 | retained 2013 source snapshot | BSD-2-Clause | legacy place and bytecode compression compatibility |
| zlib 1.2.12 | macOS SDK | Zlib | curl and Boost runtime compression support |
| FreeType 2.14.3 | `0a0221a1347e2f1e07c395263540026e9a0aa7c7` | FTL OR GPL-2.0-or-later | font loading and glyph rasterization |
| HarfBuzz 14.2.1 | `56feae4035bdd48f62ba2b8d8c16232d4d89b3a4` | MIT | OpenType text shaping |
| utf8proc 2.11.3 | `e5e799221b45bbb90f5fdc5c69b6b8dfbf017e78` | MIT | Unicode 17 normalization and properties |
| SheenBidi 3.0.0 | `cfe430e7375a7845b679adae9d51dac6deaa8858` | Apache-2.0 | Unicode Bidirectional Algorithm and visual run ordering |
| SDL 3.4.10 | `8e37db5e797b6167f3a00d697d816a684bd259c7` | Zlib | native Linux window, input, clipboard, and document-dialog host |
| Windows BCrypt | Windows SDK | LicenseRef-Microsoft-Windows-SDK | Windows GameNetworkingSockets cryptography |

The FFmpeg build uses `libavformat`, `libavcodec`, `libavutil`, `libswscale`,
and `libswresample` as replaceable shared libraries on macOS. Its exact source
archive is included in `Resources/sources/FFmpeg`, and is also available from
<https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz> at the hash recorded above.
The packaged license tree contains the upstream license and notice files for
the components listed here, including FFmpeg's license overview and LGPL 2.1
text.

This software is based in part on the work of the Independent JPEG Group. The
FFmpeg build does not modify the IJG-derived files identified by FFmpeg's
upstream `LICENSE.md`.
