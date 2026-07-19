# Third-party notices

This file covers dependencies in the default modernization graph. It must be
expanded as audio, text, serialization, and other replacements are integrated.

| Component | Revision | License | Use |
|---|---|---|---|
| bgfx | `759bdeb936ea95e4ac13d1ba8d4ce2e91c5c17d2` | BSD-2-Clause | rendering abstraction |
| bx | `c98e98cde15498250fedbc41ad6da75a896fbae0` | BSD-2-Clause | bgfx support library |
| bimg | `c3cf9afef058f99846de57050c74b183815d5ee7` | BSD-2-Clause | image/texture support |
| bgfx.cmake | `572868c0cb952add48019d267223453958e958b8` | CC0-1.0 | source build integration |
| miniaudio 0.11.25 | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | MIT-0 | device-independent shared audio mixer, decoding, and spatialization |
| GameNetworkingSockets 1.5.0 | `4fbfe83ef4d59a12dc32baeff2c33e511af93157` | BSD-3-Clause | encrypted native UDP transport |
| protobuf 35.1 | `35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03` | BSD-3-Clause | GameNetworkingSockets schema/runtime dependency |
| Abseil 20250512.1 | `76bb24329e8bf5f39704eb10d21b9a80befa7c81` | Apache-2.0 | protobuf support library |
| OpenSSL 3.5.7 LTS | `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e` | Apache-2.0 | non-Windows GameNetworkingSockets cryptography |
| FreeType 2.14.3 | `0a0221a1347e2f1e07c395263540026e9a0aa7c7` | FTL OR GPL-2.0-or-later | font loading and glyph rasterization |
| HarfBuzz 14.2.1 | `56feae4035bdd48f62ba2b8d8c16232d4d89b3a4` | MIT | OpenType text shaping |
| utf8proc 2.11.3 | `e5e799221b45bbb90f5fdc5c69b6b8dfbf017e78` | MIT | Unicode 17 normalization and properties |
| Windows BCrypt | Windows SDK | LicenseRef-Microsoft-Windows-SDK | Windows GameNetworkingSockets cryptography |

License texts are supplied by their fetched upstream source trees and will be
copied into the final application notices during packaging.
