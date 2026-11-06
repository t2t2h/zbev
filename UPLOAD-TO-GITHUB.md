# Upload this project to GitHub

This archive is the complete project source in one clean folder layout. It is **not** a prebuilt dylib.

## Important
- Extract this ZIP first.
- Upload the **contents of the extracted folder** to the root of a GitHub repository (not the ZIP file itself).
- Keep `.github/workflows/build.yml` at the repository root so GitHub Actions can find it.
- Keep `Makefile` at the repository root.
- Do not upload private VPN `.conf` profiles or signing keys. Add your own local profile only if you need the separate WireGuard host-app/Packet-Tunnel example.

## Build
1. Open the repository on GitHub.
2. Open **Actions** and enable workflows if GitHub asks.
3. Choose **Build AflatoonMubdi dylib**.
4. Select **Run workflow** and choose the build profile.
5. Download the artifact only after the run succeeds.

## Compatibility
- ElleKit/libhooker hook backends require a compatible jailbreak environment and the respective runtime to be installed. They are not bundled into the dylib.
- `embedded-safe` is a limited best-effort build path for authorized embedding in an app you own; it does not inject into arbitrary App Store apps and is not guaranteed to work on stock iOS.
- This package has not been compiled or device-tested here. A successful GitHub Actions build and on-device testing are still required.
