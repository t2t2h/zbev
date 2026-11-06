// Rebind.h - one shared symbol-pointer rebinder (fishhook style), no Substrate / no jailbreak.
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
// Register a replacement BEFORE calling ap_rebind_start().
//   name   : C symbol name without the leading underscore
//   system : 1 = also patch Apple system frameworks (needed for getaddrinfo/socket/AudioUnitRender)
//            0 = patch only the app + its own frameworks (needed for the clock hooks,
//                otherwise libsystem would count the fake time twice)
void ap_rebind_add(const char *name, void *replacement, int system);
// Hook a symbol through a jailbreak runtime when available (ElleKit's
// Substrate-compatible MSHookFunction, or libhooker's LHHookFunction).
// If neither runtime is present, register it with the built-in rebinder.
// originalStorage must point to a function-pointer variable.
int ap_hook_symbol(const char *name, void *replacement, void **originalStorage, int system);
// Returns: "ElleKit/Substrate API", "libhooker", or "fishhook fallback".
const char *ap_hook_backend_name(void);
// Patches registered imports in loaded images and images loaded later.
void ap_rebind_start(void);
#ifdef __cplusplus
}
#endif
