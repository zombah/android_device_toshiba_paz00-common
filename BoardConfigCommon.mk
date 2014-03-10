# Camera Setup
USE_CAMERA_STUB 			:= false

# inherit from the proprietary version
-include vendor/toshiba/paz00/BoardConfigVendor.mk

TARGET_BOARD_PLATFORM 			:= tegra
# Product set overrides result image name
#TARGET_PRODUCT				:= tegra
TARGET_BOARD_INFO_FILE 			:= device/toshiba/paz00-common/board-info.txt
TARGET_CPU_ABI 				:= armeabi-v7a
TARGET_CPU_ABI2 			:= armeabi
TARGET_CPU_VARIANT			:= generic
TARGET_ARCH_VARIANT 			:= armv7-a
TARGET_ARCH_VARIANT_CPU 		:= cortex-a9
TARGET_ARCH_VARIANT_FPU 		:= vfpv3-d16
TARGET_CPU_SMP 				:= true
# New 4.2 additions
TARGET_ARCH				:= arm

# Compiler Optimization - This is a @codefireX specific flag to use -O3 everywhere.
ARCH_ARM_HIGH_OPTIMIZATION              := true
# ANDROID, LINUX-ARM AND TLS REGISTER EMULATION
ARCH_ARM_HAVE_TLS_REGISTER              := true
# Avoid the generation of ldrcc instructions
NEED_WORKAROUND_CORTEX_A9_745320        := true
#define to use all of the Linaro Cortex-A9 optimized string funcs,
#instead of subset known to work on all machines
USE_ALL_OPTIMIZED_STRING_FUNCS          := true
# customize the malloced address to be 16-byte aligned
BOARD_MALLOC_ALIGNMENT                  := 16
TARGET_EXTRA_CFLAGS                     := $(call cc-option,-mtune=cortex-a9) $(call cc-option,-mcpu=cortex-a9)

ARCH_ARM_USE_NON_NEON_MEMCPY 		:= true

TARGET_NO_BOOTLOADER                    := true
TARGET_BOOTLOADER_BOARD_NAME            := paz00

# Kernel
#TARGET_KERNEL_SOURCE 			:= kernel/toshiba/paz00
#TARGET_KERNEL_CONFIG 			:= paz00_android_defconfig

# Modem
TARGET_NO_RADIOIMAGE 			:= true

# CWM Recovery settings
# custom recovery ui, seems to be obsolete
#BOARD_CUSTOM_RECOVERY_KEYMAPPING 	:= ../../device/toshiba/paz00-common/recovery/recovery_ui.c
BOARD_UMS_LUNFILE 			:= "/sys/class/android_usb/android0/f_mass_storage/lun/file"
BOARD_HAS_NO_SELECT_BUTTON              := true
# Large fonts
BOARD_USE_CUSTOM_RECOVERY_FONT 		:= \"roboto_23x41.h\"
BOARD_HAS_LARGE_FILESYSTEM              := true
#TARGET_RECOVERY_INITRC 			:= device/toshiba/paz00-common/prebuild/init.recovery.rc
RECOVERY_FSTAB_VERSION 			:= 2
TARGET_RECOVERY_FSTAB 			:= device/toshiba/paz00-common/prebuild/fstab.paz00

BOARD_WPA_SUPPLICANT_DRIVER             := NL80211
#BOARD_WPA_SUPPLICANT_DRIVER             := WEXT
WPA_SUPPLICANT_VERSION                  := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB        := private_lib_driver_cmd
BOARD_HOSTAPD_DRIVER                    := NL80211
#BOARD_HOSTAPD_DRIVER                    := WEXT
BOARD_HOSTAPD_PRIVATE_LIB               := private_lib_driver_cmd
# Unknow options
BOARD_LEGACY_NL80211_STA_EVENTS         := false
BOARD_NO_APSME_ATTR			:= false

# Wifi base
BOARD_WLAN_DEVICE                       := wlan0
WIFI_DRIVER_MODULE_NAME                 := "rt2800usb"
WIFI_DRIVER_MODULE_PATH                 := "/system/lib/modules/rt2800usb.ko"
WIFI_DRIVER_MODULE_ARG                  := "nohwcrypt=1"
WIFI_DRIVER_FW_PATH_STA                 := "/system/vendor/firmware/rt2870.bin"
WIFI_DRIVER_FW_PATH_P2P                 := "/system/vendor/firmware/rt2870.bin"
WIFI_DRIVER_FW_PATH_AP                  := "/system/vendor/firmware/rt2870.bin"
WIFI_FIRMWARE_LOADER                    := ""

# Bluetooth
BOARD_HAVE_BLUETOOTH 			:= true
BLUETOOTH_HCI_USE_USB 			:= true
BOARD_HAVE_BLUETOOTH_BCM 		:= true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/toshiba/paz00-common/bluetooth
BOARD_BLUETOOTH_DOES_NOT_USE_RFKILL    	:= true

BOARD_KERNEL_BASE 			:= 0x10000000
BOARD_PAGE_SIZE 			:= 0x00000800

# Audio
BOARD_USES_ALSA_AUDIO			:= false
BUILD_WITH_ALSA_UTILS			:= false
BOARD_USES_GENERIC_AUDIO 		:= false
BOARD_USES_AUDIO_LEGACY 		:= false
TARGET_USES_OLD_LIBSENSORS_HAL 		:= false
# Tinyhal
BOARD_USES_TINY_AUDIO_HW 		:= true
# Grouper tinyhal
BOARD_USES_GROUPER_TINYHAL		:= false
# OMX
BOARD_OMX_NEEDS_LEGACY_AUDIO            := true

# EGL
USE_OPENGL_RENDERER                     := true
BOARD_EGL_CFG 				:= device/toshiba/paz00-common/prebuild/egl.cfg
BOARD_USES_TEGRA_HWC			:= false
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK 	:= true
BOARD_EGL_NEEDS_FNW 			:= true
BOARD_USE_MHEAP_SCREENSHOT 		:= true
BOARD_EGL_WORKAROUND_BUG_10194508 	:= true
BOARD_EGL_NEEDS_LEGACY_FB               := true

#BOARD_USES_OVERLAY 			:= true
#BOARD_USES_HGL 				:= true
BOARD_HDMI_MIRROR_MODE 			:= Scale
#SKIP_SET_METADATA 			:= true
#BOARD_USES_HWCOMPOSER 			:= true
#ENABLE_WEBGL 				:= true
#BOARD_EGL_SKIP_FIRST_DEQUEUE 		:= true
TARGET_OVERLAY_ALWAYS_DETERMINES_FORMAT := true
USE_SET_METADATA 			:= false

# Use nicer font rendering
BOARD_USE_SKIA_LCDTEXT 			:= true
BOARD_NO_ALLOW_DEQUEUE_CURRENT_BUFFER 	:= true

TARGET_OTA_ASSERT_DEVICE 		:= paz00,ac100,GT-P7510

# Partitions 
TARGET_USERIMAGES_USE_EXT4		:= true
BOARD_SYSTEMIMAGE_PARTITION_SIZE 	:= 536970912
BOARD_USERDATAIMAGE_PARTITION_SIZE 	:= 1294991360
BOARD_FLASH_BLOCK_SIZE 			:= 131072

# Disable spase in image creation, otherwise image not mountble and need to be processed with simg2img
TARGET_USERIMAGES_SPARSE_EXT_DISABLED 	:= true

# dont build recovery
#TARGET_NO_RECOVERY 			:= true

# Vold settings
BOARD_VOLD_MAX_PARTITIONS 		:= 16
TARGET_USE_CUSTOM_LUN_FILE_PATH 	:= "/sys/devices/platform/tegra-udc.%d/gadget/lun%d/file"

# Use a smaller subset of system fonts to keep image size lower
#SMALLER_FONT_FOOTPRINT 			:= false

# Use less memory for dalvik
TARGET_ARCH_LOWMEM 			:= true

# Build Huaweigeneric-ril
BOARD_USES_HUAWEIGENERIC_RIL		:= false
# Build Ericsson mbm-ril
BOARD_USES_MBM_RIL			:= true

# SELinux stuff
HAVE_SELINUX 				:= true

ifeq ($(HAVE_SELINUX),true)

BOARD_SEPOLICY_DIRS += \
        device/toshiba/paz00-common/sepolicy

BOARD_SEPOLICY_UNION := \
    file_contexts \
    app.te \
    device.te \
    drmserver.te \
    file.te \
    genfs_contexts \
    healthd.te \
    init.te \
    media_app.te \
    release_app.te \
    mediaserver.te \
    platform_app.te \
    sensors_config.te \
    shared_app.te \
    surfaceflinger.te \
    system_app.te \
    system.te \
    untrusted_app.te \
    vold.te \
    wpa_socket.te \
    wpa.te \
    zygote.te

endif

# Override healthd HAL
BOARD_HAL_STATIC_LIBRARIES 		:= libhealthd.tegra
