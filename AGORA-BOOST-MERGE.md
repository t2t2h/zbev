# AgoraBoost merged into AflatoonMubdi

- `Agora.m` + `AgoraStubs.h` = the former AgoraBoost.dylib, now compiled into AflatoonMubdi.dylib (one dylib only).
- Uses pure Objective-C swizzling, so it works in BOTH build modes (`embedded-safe` and `jailbreak-runtime`).
- Controls: **mic** tab, bottom section `agora.boost` (on/off, video.hd, audio.hq, dual.stream). Saved in NSUserDefaults and in profiles.
- Console line `[sys ] agora N/3` shows how many Agora selectors were really hooked (0/3 = app has no AgoraRtcEngineKit).
- Optional `AgoraBoostPrefs.plist` (Min width/height/fps/bitrate) next to the app bundle is still read.
- Restart the app after changing the Agora switches (they apply when the app configures its engine).
