BUILD_OLD_LIBCAMERA:=

ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:=-fno-short-enums -DHAVE_CONFIG_H 

LOCAL_C_INCLUDES += \
	external/jpeg
	
LOCAL_SRC_FILES:= \
	CameraFactory.cpp \
	CameraHal.cpp \
	CameraHardware.cpp \
	Converter.cpp \
	Utils.cpp \
	V4L2Camera.cpp \
	SurfaceDesc.cpp \
	SurfaceSize.cpp 

LOCAL_SHARED_LIBRARIES:= libutils libbinder libui liblog libcamera_client libcutils libmedia libandroid_runtime libhardware_legacy libc libstdc++ libm libjpeg libandroid

LOCAL_MODULE:= camera.tegra
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

endif # not BUILD_TINY_ANDROID

