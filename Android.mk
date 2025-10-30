LOCAL_PATH := $(call my-dir)

define include-prebuilt-static-lib
  include $(CLEAR_VARS)
  LOCAL_MODULE := $1
  LOCAL_SRC_FILES := include/$2/lib$1.a
  include $(PREBUILT_STATIC_LIBRARY)
endef

$(eval $(call include-prebuilt-static-lib,dobby,Dobby/$(TARGET_ARCH_ABI)))

include $(CLEAR_VARS)
LOCAL_MODULE := Test

LOCAL_CFLAGS := -Wno-error=format-security -w -Oz \
               -flto -g0 -fvisibility=hidden \
               -fno-rtti -fexceptions \
               -fstack-protector-strong \
               -D_FORTIFY_SOURCE=2 \
               -Wformat -Wformat-security \
               -Werror=format-security

LOCAL_CPPFLAGS := -std=c++23 \
                  -Wno-error=c++11-narrowing \
                  -fpermissive \
                  $(LOCAL_CFLAGS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/include/Dobby \

LOCAL_LDLIBS := -llog -landroid -lm -ldl
LOCAL_STATIC_LIBRARIES := dobby
LOCAL_LDFLAGS += -flto -Wl,--gc-sections -Wl,--exclude-libs=ALL -Wl,--hash-style=gnu -Wl,-z,relro,-z,now

include $(BUILD_SHARED_LIBRARY)