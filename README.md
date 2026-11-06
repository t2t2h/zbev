# AflatoonMubdi
© AFLATOON | أفلاطون — exclusive to MUBDI

One standalone dylib (no Substrate, no jailbreak) for sideloading with ESign.
Merges **MubdiSpeed** (game speed) + **AflatoonMenu** (DNS / voice / EQ) into one project,
one hook registry, one menu (cyber-security look, your photo as the background).

## Tabs
- **net**   DNS override (6 resolvers, this app only), real ping HUD, net.boost (socket tuning, NOT a VPN)
- **mic**   pitch + 3-band EQ on YOUR outgoing voice (what the other side receives)
- **room**  pitch of the voices YOU hear (local playback only)
- **time**  NEW — real delay (0-2000 ms) and tempo (0.5x-2.0x), separately for
            OUT (your mic -> the other side hears it) and IN (what you hear), plus `link.both`
- **speed** game clock scaling x0.1 - x10 (mach_absolute_time / clock_gettime / gettimeofday)
- **info**  credits + honest limits
- bottom console prints REAL counters from the hooks (audio pulls, DNS overrides, game/real time)

## Honest limits
- Time/voice effects only exist where the app captures/plays voice through AudioUnitRender
  (bus 1 = mic, bus 0 = playback). Apps using another audio path won't show "LIVE" in the console.
- OUT changes what is SENT, so the other side really hears it; nothing can change the other
  person's phone itself. IN only changes what your device plays.
- Tempo on a live stream cannot run faster/slower forever: it re-centres every ~0.25 s
  (slow = skips a little audio, fast = repeats a little). Delay has no such artifact.
- Speed hack: only affects logic that runs on the device; server-checked games ignore/flag it.
- net.boost / DNS are not a VPN: no tunnel, no encryption, no IP change.

## Build
Push to GitHub -> Actions -> download artifact `AflatoonMubdi-dylib`.


## WireGuard profile import

The Net tab now has a profile-import action. Select `WireGuardIntegration/Profiles/MARCO-FREEVPN.conf` from Files and share it to the official WireGuard iOS app. Connect from WireGuard itself. The tweak is still a dylib and cannot directly run Apple's Packet Tunnel extension. See `WireGuardIntegration/README.md` and `Profiles/PROFILE-REVIEW.md` for limitations and credential handling.


## ElleKit / libhooker

See `ELLEKIT_LIBHOOKER_INTEGRATION.md` for the jailbreak runtime adapter and build profiles. The jailbreak runtime must already be installed by the device jailbreak; it is not bundled into this dylib.
