# Copyright (C) 2011 The Android-x86 Open Source Project

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE := sensors.tegra
LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(BOARD_USES_KBDSENSOR)),true)
LOCAL_SRC_FILES := kbdsensor.cpp

ifeq ($(strip $(BOARD_USES_KBDSENSOR_ROTKEY1)),true)
LOCAL_CFLAGS := -DFN_ROT_0=KEY_F9 -DFN_ROT_90=KEY_F12 -DFN_ROT_180=KEY_F10 -DFN_ROT_270=KEY_F11
endif
ifeq ($(strip $(BOARD_USES_KBDSENSOR_ROTKEY2)),true)
LOCAL_CFLAGS := -DFN_ROT_0=KEY_F5 -DFN_ROT_90=KEY_F8 -DFN_ROT_180=KEY_F6 -DFN_ROT_270=KEY_F7
endif
ifeq ($(filter -DFN_ROT_0=%,$(LOCAL_CFLAGS)),)
LOCAL_CFLAGS := -DFN_ROT_0=KEY_UP -DFN_ROT_90=KEY_RIGHT -DFN_ROT_180=KEY_DOWN -DFN_ROT_270=KEY_LEFT
endif

include $(BUILD_SHARED_LIBRARY)
endif

ifeq ($(strip $(BOARD_USES_HDAPS_ACCEL)),true)
LOCAL_SRC_FILES := hdaps.c
include $(BUILD_SHARED_LIBRARY)
endif

ifeq ($(strip $(BOARD_USES_S103T_SENSOR)),true)
LOCAL_SRC_FILES := s103t_sensor.c
include $(BUILD_SHARED_LIBRARY)
endif
