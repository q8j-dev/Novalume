# Selected place compatibility

## Acceptance scenario

The only final scenario is **Backrooms Game - Fully Scripted - FREE**. The RBXL
and RBXLX are two encodings of this one place, not separate scenarios.

| Encoding | SHA-256 |
|---|---|
| RBXL | `647b5dfc5424f3a78e2d8959eabb7a59bf47b9910f59ff2dd09b22a60a834a29` |
| RBXLX | `b1a47bea80de10691b9c2e5de5bd4cf2da60b1797a823052e773c63a65d92e0d` |

It was selected because it is compact enough for guarded repeated testing but
contains scripts, lighting, terrain/material data, modern meshes, particles,
modern UI layout/styling, and three spatial/interactive sounds.

## Current observed requirements

The RBXLX structural inventory includes 7 `CanvasGroup`, 5 `UICorner`, 3
`UIStroke`, 2 `UIScale`, 2 `UIPadding`, 2 `UIListLayout`, 2 `UIGridLayout`, one
`UIGradient`, one `UIAspectRatioConstraint`, one `UITextSizeConstraint`, four
`ScreenGui`, three `Sound`, four `MeshPart`, four `SurfaceAppearance`, two
`MaterialVariant`, terrain, post effects, local/server/module scripts, and a
tool-driven flashlight interaction.

Two looping part-attached `Sound` instances use asset `9065112164`, volume
`0.25`, linear rolloff from 1 to 40 studs, playback speed 1, and disabled custom
regions. The tool sound uses asset `115959318`, volume 1, linear rolloff from 5
to 20 studs, and does not loop. The selected `.rbxlp` now embeds the exact
authorized OGG payloads for both IDs. Runtime resolution is package-local and
does not silently fetch or substitute audio.

## Evidence matrix

| Requirement | Current evidence | State |
|---|---|---|
| Both encodings recognized | the signed packaged Player loads each file through the normal binary/XML serializer, loopback server/client join, DataModel, scripts, simulation, and VisualEngine | passed for this pair |
| Modern Sound schema | real `Sound` reflection includes playback speed, playing, rolloff, region, and acoustic fields; the verifier requires the exact three authored values and IDs | passed |
| Audio playback | the actual packaged Player resolves both embedded OGGs before HTTP, decodes Vorbis through the production audio runtime, creates three real spatial voices from exact serialized emitter clones, advances all three sounds, mixes non-silent output, and tears down the live channels | passed |
| Current UI resources | scoped hash-verified Player overlay packages with explicit precedence | passed packaging; UI acceptance remains deliberately deferred |
| Current UI semantics | the listed CanvasGroup/style/layout classes deserialize and run through the real UI engine | runtime-compatible; perceptual UI acceptance deferred |
| 3D/rendering | both encodings render real place geometry through VisualEngine on bgfx Metal for 300 frames | passed |
| Scripts/offline services | server, local, and module scripts run over the local authoritative server/client session; `Script.RunContext` is reflected with current enum behavior | passed for observed scripts |
| RBXL/RBXLX equivalence | isolated semantic inventories match exactly: 4,112 instances, 780 parts, 16 scripts, one prompt, and 16 FontFaces | passed |
| Packaged gameplay | both encodings establish the place-owned Scriptable first-person camera, move the R15 character, load 14/14 exact meshes, and invoke the supplied `Animate.PlayEmote("wave")` path | passed |

The non-UI load/simulate/render/gameplay checkpoint passes for both encodings.
The latest RBXL proof rendered 55 scene batches, 10,604 faces, and 19,409
vertices; the latest RBXLX proof rendered 16 scene batches and completed 300
frames with 66 final-frame draws. Frame-local counts can differ because the
place scripts randomize and stream visible corridor state; the isolated
serializer inventory above is the deterministic encoding-equivalence gate.

The selected-place non-UI checkpoint is closed, including its authored audio.
The 300-frame actual-app proof reported all three payloads loaded and spatial,
maximum playback positions `4.49306`, `4.49306`, and `0.18576` seconds, and
mixed RMS `0.0105223`. The verifier uses exact clones of the three ServerStorage
emitter templates only under its explicit flag because the place itself exposes
them through pickup/gameplay actions; it does not install replacement sounds.
The complete semantic, temporal, and perceptual 2026 UI matrix remains
deliberately deferred until UI work resumes.
