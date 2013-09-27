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
TARGET_ARCH_VARIANT 			:= armv7-a
TARGET_ARCH_VARIANT_CPU 		:= cortex-a9
TARGET_ARCH_VARIANT_FPU 		:= vfpv3-d16
TARGET_CPU_SMP 				:= true
# New 4.2 additions
TARGET_ARCH				:= arm


TARGET_NO_BOOTLOADER 			:= true
TARGET_BOOTLOADER_BOARD_NAME 		:= paz00

# Tegra2 specific tweaks
ARCH_ARM_HAVE_TLS_REGISTER 		:= true

# Kernel
#TARGET_KERNEL_SOURCE 			:= kernel/toshiba/paz00
#TARGET_KERNEL_CONFIG 			:= paz00_android_defconfig

USE_OPENGL_RENDERER 			:= true

# Modem
TARGET_NO_RADIOIMAGE 			:= true

# CWM Recovery settings
# custom recovery ui, seems to be obsolete
#BOARD_CUSTOM_RECOVERY_KEYMAPPING 	:= ../../device/toshiba/paz00-common/recovery/recovery_ui.c
BOARD_UMS_LUNFILE 			:= "/sys/class/android_usb/android0/f_mass_storage/lun/file"
BOARD_HAS_NO_SELECT_BUTTON              := true
# Large fonts
#BOARD_USE_CUSTOM_RECOVERY_FONT 		:= \"roboto_15x24.h\"
BOARD_HAS_LARGE_FILESYSTEM              := true
#TARGET_RECOVERY_INITRC 			:= device/toshiba/paz00-common/prebuild/init.recovery.rc

#BOARD_WPA_SUPPLICANT_DRIVER             := NL80211
BOARD_WPA_SUPPLICANT_DRIVER             := WEXT
WPA_SUPPLICANT_VERSION                  := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB        := private_lib_driver_cmd
#BOARD_HOSTAPD_DRIVER                    := NL80211
BOARD_HOSTAPD_DRIVER                    := WEXT
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

BOARD_EGL_CFG 				:= device/toshiba/paz00-common/prebuild/egl.cfg
TARGET_OTA_ASSERT_DEVICE 		:= paz00,ac100,GT-P7510

# Partitions 
TARGET_USERIMAGES_USE_EXT4		:= true
BOARD_BOOTIMAGE_PARTITION_SIZE 		:= 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE 	:= 5242880
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

