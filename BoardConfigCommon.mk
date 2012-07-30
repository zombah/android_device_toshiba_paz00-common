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

TARGET_NO_BOOTLOADER 			:= true
TARGET_BOOTLOADER_BOARD_NAME 		:= paz00

ARCH_ARM_HAVE_TLS_REGISTER 		:= true

# Kernel
TARGET_KERNEL_SOURCE 			:= kernel/toshiba/paz00
TARGET_KERNEL_CONFIG 			:= paz00_android_defconfig

#BOARD_USES_HGL := true
#BOARD_USES_OVERLAY := true
USE_OPENGL_RENDERER 			:= true

# Modem
TARGET_NO_RADIOIMAGE 			:= true

# Use Old Style USB Mounting Untill we get kernel source
BOARD_USE_USB_MASS_STORAGE_SWITCH 	:= true

# custom recovery ui
BOARD_CUSTOM_RECOVERY_KEYMAPPING 	:= ../../device/toshiba/paz00-common/recovery/recovery_ui.c

# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER 		:= NL80211
WPA_SUPPLICANT_VERSION 			:= VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB 	:= lib_driver_cmd_wl12xx
BOARD_HOSTAPD_DRIVER 			:= NL80211
BOARD_HOSTAPD_PRIVATE_LIB 		:= lib_driver_cmd_wl12xx
BOARD_WLAN_DEVICE 			:= wlan0
WIFI_DRIVER_MODULE_NAME                 := rt2800usb
WIFI_DRIVER_MODULE_PATH                 := /system/lib/modules/rt2800usb.ko

# Bluetooth
BOARD_HAVE_BLUETOOTH 			:= true
BOARD_HAVE_BLUETOOTH_BCM 		:= true
#BOARD_HAVE_BLUETOOTH_CSR 		:= true

BOARD_KERNEL_BASE 			:= 0x10000000
BOARD_PAGE_SIZE 			:= 0x00000800

# Audio
BOARD_USES_ALSA_AUDIO			:= false
BUILD_WITH_ALSA_UTILS			:= false
BOARD_USES_GENERIC_AUDIO 		:= true
BOARD_USES_AUDIO_LEGACY 		:= false
TARGET_USES_OLD_LIBSENSORS_HAL 		:= false

BOARD_EGL_CFG 				:= device/toshiba/paz00-common/prebuild/egl.cfg
TARGET_OTA_ASSERT_DEVICE 		:= paz00,ac100,GT-P7510

# Partitions 
BOARD_HAS_LARGE_FILESYSTEM 		:= true
TARGET_USERIMAGES_USE_EXT4		:= true
BOARD_BOOTIMAGE_PARTITION_SIZE 		:= 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE 	:= 5242880
BOARD_SYSTEMIMAGE_PARTITION_SIZE 	:= 314572800
BOARD_USERDATAIMAGE_PARTITION_SIZE 	:= 1294991360
BOARD_FLASH_BLOCK_SIZE 			:= 131072

# Disable spase in image creation, otherwise image not mountble
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

# Setting this to avoid boot locks on the system from using the "misc" partition.
BOARD_HAS_NO_MISC_PARTITION 		:= true

# dont build recovery
#TARGET_NO_RECOVERY 			:= true

# Indicate that the board has an Internal SD Card
#BOARD_HAS_SDCARD_INTERNAL 		:= true

BOARD_DATA_DEVICE 			:= /dev/block/mmcblk0p6
BOARD_DATA_FILESYSTEM 			:= ext4
BOARD_CACHE_DEVICE 			:= /dev/block/mmcblk0p4
BOARD_CACHE_FILESYSTEM 			:= ext4
BOARD_SYSTEM_DEVICE 			:= /dev/block/mmcblk0p3
BOARD_SYSTEM_FILESYSTEM 		:= ext4
BOARD_HAS_NO_SELECT_BUTTON 		:= true

# Vold settings
BOARD_VOLD_MAX_PARTITIONS 		:= 11

BOARD_NO_ALLOW_DEQUEUE_CURRENT_BUFFER 	:= true

# Use nicer font rendering
BOARD_USE_SKIA_LCDTEXT 			:= true

# skip doc from building
BOARD_SKIP_ANDROID_DOC_BUILD		:= true

# kbd libsensor from android-x86
BOARD_USES_KBDSENSOR 			:= true
BOARD_USES_KBDSENSOR_ROTKEY2		:= true
