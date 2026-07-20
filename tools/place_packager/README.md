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

Places whose own scripts expect Studio Play Solo semantics can opt into the
package's explicit offline execution mode with `--local-solo`. The Player then
runs its normal loopback server/client session while reporting that session as
Studio simulation to authored scripts; ordinary RBXLP packages remain Player
sessions.

A local-solo package may include narrowly scoped server and client compatibility
scripts with `--bootstrap-script server.lua` and
`--client-bootstrap-script client.lua`. The Player runs those scripts only in the
package's authoritative loopback server DataModel. This lets archived games
replace a cloud matchmaking/elevator handoff without changing their place XML.

The builder rejects incomplete archives by default. `--allow-missing` is only
for assets that the archiver proves are inaccessible (for example, deleted or
archived assets); their IDs remain explicit in the embedded manifest.
