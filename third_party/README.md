# Third-party source policy

Dependencies are fetched at configure time into the ignored build tree and are
pinned in `cmake/RbxDependencies.cmake`. Owned source is never mixed into the
dependency checkout. Patches, if required, must be stored as explicit files in
this directory and documented in `THIRD_PARTY_NOTICES.md`.

Current rendering pins form the exact gitlink set recorded by bgfx.cmake commit
`572868c0cb952add48019d267223453958e958b8`:

| Project | Revision | Source | Purpose |
|---|---|---|---|
| bgfx.cmake | `572868c0cb952add48019d267223453958e958b8` | <https://github.com/bkaradzic/bgfx.cmake> | CMake build wrapper |
| bgfx | `759bdeb936ea95e4ac13d1ba8d4ce2e91c5c17d2` | <https://github.com/bkaradzic/bgfx> | portable renderer |
| bx | `c98e98cde15498250fedbc41ad6da75a896fbae0` | <https://github.com/bkaradzic/bx> | bgfx base library |
| bimg | `c3cf9afef058f99846de57050c74b183815d5ee7` | <https://github.com/bkaradzic/bimg> | bgfx image library |
| GameNetworkingSockets | `4fbfe83ef4d59a12dc32baeff2c33e511af93157` | <https://github.com/ValveSoftware/GameNetworkingSockets> | native secure UDP transport |
| protobuf | `35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03` | <https://github.com/protocolbuffers/protobuf> | GNS schema/runtime dependency |
| Abseil | `76bb24329e8bf5f39704eb10d21b9a80befa7c81` | <https://github.com/abseil/abseil-cpp> | protobuf support library |
| OpenSSL 3.5.7 LTS | `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e` | <https://github.com/openssl/openssl> | non-Windows GNS cryptography |
