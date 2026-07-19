I'm going to sleep soon. Here is what you need to hear.

ISSUES:

- Textures/skybox are not working (on this baseplate, at least), the textures fall back to solid grey, and the skybox falls back to a solid blue. Fix this.
- The 2026 UI is still buggy. The Report and Switch Avatar icons in the hamburger menu have a grey background, instead of being the transparent white icons they're supposed to be.
- The 2026 esc/pause menu needs LOTS of work. It is supposed to look like the image attached, as that is the current state of it in the 2026 Roblox client.
- When moving the camera, my cursor snaps to the middle of the screen. Overall, the cursor system is buggy. 
- The current state of the UI doesn't scale the same way as the 2026 client, and is sometimes stretched. I want the whole scaling system from the 2026 client to be reverse engineered/decompiled, and implemented.
- The "ShadowMap" lighting technology is not implemented in this version of our client, but is in the latest 2026 one. I want this Engine Lighting Technology to be fully reverse engineered/decompiled, and implemented correctly for the games that use it.
- The leaderboard UI is mostly still broken, and has a weird, grey background, and is broken in most areas. Completely fix it.
- None of the features that are in the current 2026 In Experience UI have been implemented. Notably, "Chat", "Report", "Emotes", "Leaderboard (just needs more work), "Respawn", "Switch Avatar (when applicable)", and Music. Music should be completely removed from the UI and not implemented, as it is not useful. These are the main features that need to be implemented. Note that the Chat UI must be the modern UI from the 2026 client, not "LegacyChatService". Upon clicking the unsupported features, the entire "top bar/chrome" UI disappears, for an unknown reason. 
- Some sounds are incorrectly sped up, higher pitched, or outright broken. Example: the volume control example sound (OOF) is high pitched.
- Many esc menu features (such as shift-lock), do not work. As well as this, zooming in and out in-game is really fast and NOTHING like the 2026 client. I want you to decompile/reverse engineer the current client's zooming and overall camera system, so it is 1:1 to the 2026 client.

GOALS: 

- Complete 2026 UI working, completely, with a "Switch to Legacy UI" toggle in Settings (in the esc menu), that when enabled, switches back to the legacy UI (original 2016).
- Complete "Xbox UI", (already implemented in the Durango work) used as a launcher for games, with adapted controls for PC, mobile, and web. Keep the existing controller support (this will be explained later here). Must have completely working navigation, sound, and visuals.
- Complete ShadowMap implementation, from reverse engineered/decompiled 2026 client code. Nothing should be missing from this implementation. Ensure that it has 1:1 visual parity. (Do not open Roblox, or attempt to use the "Computer Use" tool for this. You should just be confident in the implementation that you know it is 1:1 via decompiled/reverse engineered code.
- Complete R15 implementation, with working emotes (if possible), and animations (required). 
- COMPLETE compatibility layer for 2026 RBXL/RBXLX files, and 2026 Roblox games. Analyze the features that "2026-place-files" use, and if something from there is missing in the current source, completely implement it by reverse engineering/decompiling the feature from the 2026 client. Most 2026 Roblox games/RBXL/RBXLXs should work as a result of this. Do not break <2026 games from working. In the future, I will add more RBXLs and RBXLXs for you to study, and expand the compatibility layer.
- Use my GitHub .env to create a PRIVATE repository for the project on my q8j-dev GitHub account. The old name "OpenRBLX" doesn't fit, as it is already being used by another project. Come up with a unique and creative name for this. Do NOT mention that you are the "co-author" of this project, or anything like that (in commits, README, etc). This GitHub repository will be useful, as you can utilize GitHub Actions to build for Windows (x64, ARM), Linux (x64, ARM), iOS, Android, Emscripten (WebGPU preferred) and Xbox (UWP, APPX ready for sideloading in Dev Mode on Series X/S). This is why we need to keep controller support, as the Xbox build will actively need it. Make sure the controller support works for the platforms mentioned (when possible). 
- I do not want to see any visual/audible bugginess when playing on the client. If something may introduce visual/audible bugginess, provide a solid implementation that fixes it.
- Adapt RCCService, and all Roblox server software bundled in the source to the new networking system (https://github.com/ValveSoftware/GameNetworkingSockets). Do not adapt server software that isn't relevant. The server software should be fully compatible with our modernized client, and work for everything that it originally does (full game-server networking). It should additionally work on a modern VPS, such as an Ubuntu VPS.
- Continue to reorganize stuff that hasn't already been organized, and modernize implementations the the client source uses. The same applies to server source.
- Anything else mentioned in the original goal that I haven't covered, I assume you will complete. If you don't remember, recap on the very original goal. 

NOTES:

- If you are adding/fixing/implementing something, do NOT make stubs, examples, placeholders, or fake it. This is against the rules.
- Save this prompt to a file, so you can recap on it when needed.
- For the other platforms (apart from macOS), I assume you know which renderer to pick, and be confident that it will work without any issues. I recommend double checking implementations, like in the renderer, so that you are sure it will fully work on other platforms, as intended.
- Do not ask any questions, or do anything that requires extensive permission (e.g. sudo).
- I will assume that you are capable of properly debugging this if something breaks, and understanding what I want from you.
- This is possible. It will take lots of work, but it's possible, so don't stop until you're fully done.

That's mostly all. In the future, I may have more work beyond this, but this is what you need to do now, so get started.
