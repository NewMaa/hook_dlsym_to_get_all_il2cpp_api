APP_ABI := arm64-v8a
APP_STL := c++_static
APP_PLATFORM := android-21
APP_OPTIM := release
APP_STRIP_MODE := --strip-all

# 高级LTO优化
NDK_APP_LDFLAGS := -flto=full
APP_CFLAGS := -fno-omit-frame-pointer
APP_LDFLAGS := -Wl,--exclude-libs=ALL

# PIE强制启用（Android 5.0+要求）
APP_PIE := true