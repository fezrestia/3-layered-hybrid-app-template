# Application.mk

APP_PLATFORM := android-19

APP_ABI := \
        armeabi \
        armeabi-v7a \
        mips \
        x86 \

# This overrides internal state. So, in future, this line can not be supported.
NDK_APP.local.cleaned_binaries=true
