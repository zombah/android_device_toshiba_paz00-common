# powerbtn executable
LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := powerbtnd.c

LOCAL_MODULE := powerbtnd
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils

include $(BUILD_EXECUTABLE)
