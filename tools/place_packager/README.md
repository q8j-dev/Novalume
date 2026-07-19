# RBXLP place packages

`RBXLP` is this client's deterministic offline place format. It stores one
unaltered RBXL/RBXLX file, every archived numeric asset payload, and a provenance
manifest in one CRC-checked file. The Player materializes the package into its
private cache and mounts `assets/<id>.<format>` ahead of network delivery.

First scan and download authorized assets with the separate Roblox Asset
Archiver, then package its verified output:

```sh
python3 tools/place_packager/build_place_package.py \
  --place game.rbxlx --archive /path/to/archive --output game.rbxlp
```

The builder rejects incomplete archives by default. `--allow-missing` is only
for assets that the archiver proves are inaccessible (for example, deleted or
archived assets); their IDs remain explicit in the embedded manifest.
