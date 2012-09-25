#ifeq  ($(strip $(BUILD_INCLUDE_ABOOTIMG)),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := abootimg.c
LOCAL_CFLAGS := -O3 -Wall -DHAS_BLKID
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libext2_blkid
#LOCAL_STATIC_LIBRARIES	:= libc libext2_blkid
LOCAL_C_INCLUDES := external/kernel-headers/original external/e2fsprogs/lib
LOCAL_MODULE := abootimg
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

#endif
