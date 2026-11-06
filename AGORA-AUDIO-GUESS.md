# Agora audio path — best-effort integration (assumption-based)

This package does **not** include Agora SDK headers or an app-specific Agora engine/delegate. The chosen fallback is to intercept `AudioUnitRender`, because many iOS real-time audio stacks (including some Agora configurations) use Apple's RemoteIO/VoiceProcessingIO AudioUnit for microphone capture and playback.

## Assumed routing

- Render bus 1: treated as microphone capture. Pitch/EQ/outgoing time processing is applied in-place after the original `AudioUnitRender` returns. If Agora subsequently encodes this same PCM buffer, the remote peer can hear the processed voice.
- Render bus 0: treated as playback. Incoming-room processing is local to this device.
- Each participant needs the tweak on their own device for symmetric outgoing processing. This is not a server-side or cross-device setting sync feature.

## Important uncertainty

Agora's actual audio path varies by SDK version, configuration, audio device module, and app integration. Some apps use Agora's raw audio frame observer (`IAudioFrameObserver` / platform wrapper), custom audio devices, or another capture pipeline. In those cases this fallback may see no mic frames, or the bus mapping may differ. The counters show only calls observed on these assumed buses; they do not prove that processed frames reached Agora's encoder.

A production-grade direct integration should use the exact Agora iOS SDK version and the app's existing audio-frame observer/delegate, preserving the app's current observer and forwarding callbacks. That cannot be safely synthesized from a dylib without the target app/SDK version and its observer setup.

## Safety changes in this build

- Added a null check for the original `AudioUnitRender` function before calling it.
- Validates planar channel layout and both audio buffer sizes before processing, avoiding an out-of-bounds write when planar buffers have different lengths.
- UI labels explicitly identify the implementation as an AudioUnit fallback, not a native Agora raw-frame adapter.

## Two-device verification checklist

1. Install the same build on both test devices and join the same Agora test channel.
2. Enable only OUT pitch/EQ on device A. Have device B confirm the voice changes; device A's local sidetone is not proof.
3. Reverse A/B and repeat.
4. Enable IN processing only on A and confirm only A hears the remote participant changed.
5. Watch `bus1 pulls` and `bus0 pulls`. If bus1 remains idle during speech, this fallback is not attached to Agora's capture path in that app.
6. Test Bluetooth, speaker, wired headset, route changes, mute/unmute, and reconnect. Disable processing if audio glitches or CPU usage rises.

No claim is made that this assumption-based path has been built with Theos or verified in a live Agora call in this environment.
