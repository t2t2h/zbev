// Agora.m - AgoraBoost merged into AflatoonMubdi (AFLATOON)
// Pure Objective-C runtime swizzling: works on stock iOS (no Substrate / ElleKit needed),
// independent from the C-symbol hook backend used by Hook.m.
#import "AgoraStubs.h"
#import <objc/runtime.h>
#import <UIKit/UIKit.h>
#include "Shared.h"

// ---- live switches (menu + NSUserDefaults) ----
volatile int ap_agoraOn       = 1;
volatile int ap_agoraHD       = 1;   // raise video size / fps / bitrate
volatile int ap_agoraHQAudio  = 1;   // force MusicHighQualityStereo profile
volatile int ap_agoraDual     = 1;   // force dual-stream mode
volatile int ap_agoraHooks    = 0;   // number of selectors really swizzled (0 = SDK not found yet)

static NSInteger gMinWidth = 1280, gMinHeight = 720, gMinFrameRate = 24, gMinBitrateKbps = 1500;
static BOOL gInstalled = NO;

// optional AgoraBoostPrefs.plist next to the app bundle (numbers only: Min* keys)
static void AGBLoadNumbers(void) {
    NSString *path = [[NSBundle mainBundle].bundlePath stringByAppendingPathComponent:@"AgoraBoostPrefs.plist"];
    NSDictionary *p = [NSDictionary dictionaryWithContentsOfFile:path];
    if (!p) return;
    if (p[@"MinWidth"])       gMinWidth       = [p[@"MinWidth"] integerValue];
    if (p[@"MinHeight"])      gMinHeight      = [p[@"MinHeight"] integerValue];
    if (p[@"MinFrameRate"])   gMinFrameRate   = [p[@"MinFrameRate"] integerValue];
    if (p[@"MinBitrateKbps"]) gMinBitrateKbps = [p[@"MinBitrateKbps"] integerValue];
    if (p[@"Enabled"])         ap_agoraOn      = [p[@"Enabled"] boolValue];
    if (p[@"ForceHDVideo"])    ap_agoraHD      = [p[@"ForceHDVideo"] boolValue];
    if (p[@"ForceHQAudio"])    ap_agoraHQAudio = [p[@"ForceHQAudio"] boolValue];
    if (p[@"ForceDualStream"]) ap_agoraDual    = [p[@"ForceDualStream"] boolValue];
}

@interface NSObject (AgoraBoostSwizzle)
- (int)agb_setVideoEncoderConfiguration:(AgoraVideoEncoderConfiguration *)config;
- (int)agb_setAudioProfile:(AgoraAudioProfile)profile scenario:(AgoraAudioScenario)scenario;
- (int)agb_enableDualStreamMode:(BOOL)enabled;
@end

@implementation NSObject (AgoraBoostSwizzle)
// After the exchange, calling agb_xxx runs the ORIGINAL implementation.
- (int)agb_setVideoEncoderConfiguration:(AgoraVideoEncoderConfiguration *)config {
    if (ap_agoraOn && ap_agoraHD && config) {
        CGSize dim = config.dimensions;
        if (dim.width * dim.height < (CGFloat)(gMinWidth * gMinHeight))
            config.dimensions = CGSizeMake(gMinWidth, gMinHeight);
        if (config.frameRate < gMinFrameRate) config.frameRate = gMinFrameRate;
        if (config.bitrate < gMinBitrateKbps) config.bitrate = gMinBitrateKbps;
        if (config.minBitrate > 0 && config.minBitrate < gMinBitrateKbps / 2)
            config.minBitrate = gMinBitrateKbps / 2;
    }
    return [self agb_setVideoEncoderConfiguration:config];
}

- (int)agb_setAudioProfile:(AgoraAudioProfile)profile scenario:(AgoraAudioScenario)scenario {
    AgoraAudioProfile p = (ap_agoraOn && ap_agoraHQAudio) ? AgoraAudioProfileMusicHighQualityStereo : profile;
    return [self agb_setAudioProfile:p scenario:scenario];
}

- (int)agb_enableDualStreamMode:(BOOL)enabled {
    BOOL v = (ap_agoraOn && ap_agoraDual) ? YES : enabled;
    return [self agb_enableDualStreamMode:v];
}
@end

static BOOL AGBSwizzle(Class cls, SEL orig, SEL swz) {
    Method o = class_getInstanceMethod(cls, orig);
    Method s = class_getInstanceMethod(cls, swz);
    if (!o || !s) return NO;
    if (class_addMethod(cls, orig, method_getImplementation(s), method_getTypeEncoding(s))) {
        class_replaceMethod(cls, swz, method_getImplementation(o), method_getTypeEncoding(o));
    } else {
        method_exchangeImplementations(o, s);
    }
    return YES;
}

// Safe to call many times: swizzles once, only if AgoraRtcEngineKit exists in this process.
void ap_agora_install(void) {
    if (gInstalled) return;
    Class k = NSClassFromString(@"AgoraRtcEngineKit");
    if (!k) return;                       // app has no Agora (yet) -> do nothing
    gInstalled = YES;
    int n = 0;
    n += AGBSwizzle(k, @selector(setVideoEncoderConfiguration:), @selector(agb_setVideoEncoderConfiguration:));
    n += AGBSwizzle(k, @selector(setAudioProfile:scenario:),     @selector(agb_setAudioProfile:scenario:));
    n += AGBSwizzle(k, @selector(enableDualStreamMode:),         @selector(agb_enableDualStreamMode:));
    ap_agoraHooks = n;
    NSLog(@"[AflatoonMubdi/Agora] swizzled %d selectors", n);
}

void ap_agora_start(void) {
    AGBLoadNumbers();
    ap_agora_install();
    // Agora may be loaded later (dlopen / lazy framework): retry when the app becomes active.
    if (!gInstalled) {
        __block id token = nil;
        token = [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification
                 object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
            ap_agora_install();
            if (gInstalled && token) { [NSNotificationCenter.defaultCenter removeObserver:token]; token = nil; }
        }];
    }
}
