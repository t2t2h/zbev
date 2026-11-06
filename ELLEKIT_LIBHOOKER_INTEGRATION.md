# ElleKit / libhooker runtime integration

## What this build does

- The jailbreak build uses a small runtime adapter in `Rebind.c`.
- It first looks for `MSHookFunction` (ElleKit's Substrate-compatible API), then `LHHookFunction` (libhooker).
- If neither symbol is present, it falls back to the project's import-pointer rebinder.
- The adapter hooks the existing symbols used by this project: `getaddrinfo`, `socket`, `AudioUnitRender`, and the clock functions.
- The dylib does **not** contain ElleKit or libhooker, does not install them, and does not inject itself. The jailbreak's bootstrap/package must load the tweak and provide the hook runtime.

## Build profiles

- `embedded-safe`: disables hook installation. This is the non-jailbroken/host-app profile; it does not provide automatic injection.
- `jailbreak-runtime`: enables hooks and runtime detection. Use only in a compatible jailbroken environment where ElleKit or libhooker is already installed and the tweak is loaded by the jailbreak bootstrap.

## Important compatibility notes

- ElleKit's `MSHookFunction` is the preferred path. The libhooker path resolves `LHHookFunction` dynamically and assumes its standard three-argument function-hook ABI. Verify on the target libhooker release before distributing.
- No private runtime library is linked at build time, so the dylib can still be built without having the jailbreak runtime SDK present.
- A successful compile does not prove that every hook is safe or effective in a specific app. AudioUnitRender may not be the app's actual voice capture/playback path; DNS and socket hooks affect only calls that pass through the hooked symbols.
- Do not use `jailbreak-runtime` on stock iOS. It is not a way to bypass code signing or Apple's injection restrictions.
