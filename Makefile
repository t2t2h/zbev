TARGET := iphone:clang:latest:14.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = AflatoonMubdi

AflatoonMubdi_FILES = Main.m Hook.m UI.m Audio.c DSP.c Time.c DNS.c Rebind.c SpeedCore.c Image.c Agora.m
AflatoonMubdi_CFLAGS = -O2 -Wno-deprecated-declarations

# Stock iOS embedded mode: patch only the host app's ordinary __DATA imports.
# Never attempt to modify protected system images or __DATA_CONST pages.
ifeq ($(AP_BUILD_MODE),embedded-safe)
  AflatoonMubdi_CFLAGS += -DAP_SAFE_EMBEDDED=1 -DAP_JAILBREAK_RUNTIME=0
else
  # Runtime hooks are resolved dynamically from ElleKit or libhooker.
  # Do not statically link private jailbreak frameworks into the dylib.
  AflatoonMubdi_CFLAGS += -DAP_SAFE_EMBEDDED=0 -DAP_JAILBREAK_RUNTIME=1
endif
AflatoonMubdi_OBJCFLAGS = -fobjc-arc
AflatoonMubdi_FRAMEWORKS = UIKit Foundation AudioToolbox QuartzCore CoreGraphics Network
AflatoonMubdi_INSTALL_PATH = @executable_path/Frameworks

include $(THEOS_MAKE_PATH)/library.mk
