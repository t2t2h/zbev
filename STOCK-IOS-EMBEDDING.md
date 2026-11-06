# Stock iOS / non-jailbreak embedding notes

## Important distinction
A dylib is not a standalone iOS app and cannot be injected into an arbitrary App Store app on a stock, non-jailbroken device. For a legitimate test, embed it in an app you own/control, add it to the app's Frameworks/Embed Frameworks phase, use an `@rpath` install name, and sign the host app and embedded dylib with a provisioning profile that permits the app to run. Do not rely on `DYLD_INSERT_LIBRARIES` on stock iOS.

## GitHub Actions profiles
- `embedded-safe` is the default. It avoids patching protected system images and skips `__DATA_CONST` import slots, which may be protected on stock iOS. It is a best-effort embedded build, not a guarantee that all hooks will attach. It may not see AudioUnitRender if the relevant import lives in a protected/chained-fixup section.
- `jailbreak` retains the broader legacy hook behavior for a jailbroken test device. Do not use this profile on a stock device.

## Crash-reduction changes
- DNS and socket hooks are registered for app/embedded images only; no system-image patch request is made.
- `embedded-safe` skips writes to `__DATA_CONST` and never attempts `vm_protect` on those pages.
- Mach-O load-command, section, symbol-table and string-table bounds are checked before traversing import tables.
- Audio processing remains pass-through unless the assumed AudioUnitRender callback is actually reached and its format validates.

## Feature limitations in embedded-safe mode
The project still uses a runtime import-hook approach, not an official Agora SDK integration. On stock iOS, dyld chained fixups, pointer authentication, or imports located outside ordinary writable `__DATA` may prevent hooks from attaching. In that case the dylib can load while audio/DNS/clock hooks remain inactive. A guaranteed non-jailbreak solution for Agora audio is to integrate processing into the source of the host app using Agora's supported raw-audio-frame APIs and preserve its existing observer/delegate.

## Signing / testing
1. Build with GitHub Actions using `embedded-safe`.
2. Download the dylib artifact.
3. Add it to an app you own in Xcode under Frameworks, Libraries, and Embedded Content; set it to Embed & Sign.
4. Ensure the dylib install name resolves through the app's `@rpath` and the final app is re-signed after embedding.
5. Test on a spare device. First verify app launch with all feature switches off; then test each feature separately and inspect device crash logs.

No source-only change can guarantee zero crashes on every iOS version/device. This package has not been built with Apple's Xcode SDK or tested on a physical stock iPhone in this environment.
