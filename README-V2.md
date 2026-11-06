# AflatoonMubdi V2 — feature and validation notes

## Included V2 work

- Persistent settings: all audio, DNS, socket-tuning, speed and wall-clock values are restored from `NSUserDefaults`; changes are saved as controls change and again when the app backgrounds or terminates.
- Profiles Manager: `GAMING`, `VOICE`, and `NORMAL` each have local SAVE/LOAD snapshots. Profiles are device-local and do not sync to another participant.
- Live Network Monitor: `NWPathMonitor` reports online/offline and the active interface (Wi-Fi/cellular/ethernet). The HUD's displayed latency is an HTTPS HEAD round-trip to Apple's test endpoint, not ICMP ping and not necessarily the game's server latency.
- WireGuard control bridge: the Net tab can import a `.conf` and open a companion app using `aflatoonvpn://connect` / `aflatoonvpn://disconnect`. A separately signed host app plus Packet Tunnel extension is required. A Theos dylib alone cannot create or own an iOS Packet Tunnel.

## Agora best-effort audio integration

See `AGORA-AUDIO-GUESS.md`. The current implementation uses an AudioUnitRender compatibility hook and assumes bus 1 is microphone capture and bus 0 is playback. It is not a native Agora raw-audio-frame delegate. Verify the two-device path before treating outgoing processing as confirmed.

## Two-party / online audio behavior

- OUT/MIC processing (pitch, EQ, delay and tempo) is applied to the local microphone capture buffer at AudioUnitRender bus 1. If the target app actually uses this capture path and then transmits that buffer, the remote participant hears the processed voice.
- IN/ROOM processing is applied to the playback buffer at bus 0. It changes only what this device hears. It cannot modify the other participant's phone.
- For both participants to hear their own outgoing processed voice, each participant needs the same compatible tweak/configuration on their own device. Each side's incoming playback processing is also local.
- A delay slider intentionally adds latency. It does not reduce network ping or improve connection quality. Tempo changes can cause repeated/skipped audio segments during live streaming.
- This hook is not a universal VoIP interception layer and is not a native Agora raw-frame observer. If the app uses AVAudioEngine, WebRTC's internal audio device, a custom capture path, or another bus/format, the counters may remain idle and the effect will not reach the stream. Confirm with the live counters and an actual two-device call.

## WireGuard safety / setup

`WireGuardIntegration/Profiles/MARCO-FREEVPN.conf` is a template with `REPLACE_WITH_NEW_PRIVATE_KEY`. The private key previously uploaded in chat was exposed; do not reuse it. Obtain a newly generated profile/key from the provider. The profile is split-routed according to its supplied AllowedIPs; do not assume all traffic is tunneled. The host app's provider bundle ID must match the Packet Tunnel extension bundle ID, and both targets need valid Apple signing/provisioning and Network Extension entitlement.

## Build and test status

- ZIP integrity and source-level consistency checks can be run without an Apple SDK.
- A real Theos build requires the user's configured Theos/iOS SDK environment.
- The companion app and Packet Tunnel require Xcode, WireGuardKit, Apple signing, and the Network Extension entitlement.
- No claim is made that the code was installed or tested on a physical iPhone or that a live VPN handshake was achieved in this environment.
