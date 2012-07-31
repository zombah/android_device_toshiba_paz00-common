#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Base config files
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/prebuild/init.paz00.rc:root/init.paz00.rc \
    device/toshiba/paz00-common/prebuild/init.nv_dev_board.usb.rc:root/init.nv_dev_board.usb.rc \
    device/toshiba/paz00-common/prebuild/init.local.rc:system/etc/init.local.rc \
    device/toshiba/paz00-common/prebuild/ueventd.paz00.rc:root/ueventd.paz00.rc \
    device/toshiba/paz00-common/prebuild/media_profiles.xml:system/etc/media_profiles.xml \
    device/toshiba/paz00-common/prebuild/excluded-input-devices.xml:system/etc/excluded-input-devices.xml \
    device/toshiba/paz00-common/prebuild/egalax_i2c.idc:system/usr/idc/egalax_i2c.idc \
    device/toshiba/paz00-common/prebuild/egalax_ts.idc:system/usr/idc/egalax_ts.idc \
    device/toshiba/paz00-common/prebuild/01NVOptimalization:system/etc/init.d/01NVOptimalization \
    device/toshiba/paz00-common/prebuild/02PmDebug:system/etc/init.d/02PmDebug \
    frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/base/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml 

# Keychars
# Keylayout
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/keymaps/cpcap-key.kcm:system/usr/keychars/cpcap-key.kcm \
    device/toshiba/paz00-common/keymaps/gpio-keys.kcm:system/usr/keychars/gpio-keys.kcm \
    device/toshiba/paz00-common/keymaps/nvec_cir.kcm:system/usr/keychars/nvec_cir.kcm \
    device/toshiba/paz00-common/keymaps/nvec_keyboard.kcm:system/usr/keychars/nvec_keyboard.kcm \
    device/toshiba/paz00-common/keymaps/tegra-kbc.kcm:system/usr/keychars/tegra-kbc.kcm \
    device/toshiba/paz00-common/keymaps/cpcap-key.kl:system/usr/keylayout/cpcap-key.kl \
    device/toshiba/paz00-common/keymaps/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
    device/toshiba/paz00-common/keymaps/nvec_cir.kl:system/usr/keylayout/nvec_cir.kl \
    device/toshiba/paz00-common/keymaps/nvec_keyboard.kl:system/usr/keylayout/nvec_keyboard.kl \
    device/toshiba/paz00-common/keymaps/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl 

# Vold
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/prebuild/vold.fstab:system/etc/vold.fstab

PRODUCT_COPY_FILES += device/toshiba/paz00-common/prebuild/tiny_hw.xml:system/etc/sound/paz00.xml


# WiFi/BT Firmware
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/prebuild/firmware/rt2870.bin:system/vendor/firmware/rt2870.bin \
    device/toshiba/paz00-common/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    device/toshiba/paz00-common/wifi/hostapd.conf:system/etc/wifi/hostapd.conf

# Some files for 3G
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/ppp/ip-up:/system/etc/ppp/ip-up \
    device/toshiba/paz00-common/ppp/ip-down:/system/etc/ppp/ip-down

# Alsa config files
PRODUCT_COPY_FILES += \
    device/toshiba/paz00-common/alsa/alsa.conf:/system/usr/share/alsa/alsa.conf \
    device/toshiba/paz00-common/alsa/cards/aliases.conf:/system/usr/share/alsa/cards/aliases.conf \
    device/toshiba/paz00-common/alsa/pcm/modem.conf:/system/usr/share/alsa/pcm/modem.conf \
    device/toshiba/paz00-common/alsa/pcm/iec958.conf:/system/usr/share/alsa/pcm/iec958.conf \
    device/toshiba/paz00-common/alsa/pcm/dpl.conf:/system/usr/share/alsa/pcm/dpl.conf \
    device/toshiba/paz00-common/alsa/pcm/surround50.conf:/system/usr/share/alsa/pcm/surround50.conf \
    device/toshiba/paz00-common/alsa/pcm/center_lfe.conf:/system/usr/share/alsa/pcm/center_lfe.conf \
    device/toshiba/paz00-common/alsa/pcm/surround51.conf:/system/usr/share/alsa/pcm/surround51.conf \
    device/toshiba/paz00-common/alsa/pcm/dsnoop.conf:/system/usr/share/alsa/pcm/dsnoop.conf \
    device/toshiba/paz00-common/alsa/pcm/side.conf:/system/usr/share/alsa/pcm/side.conf \
    device/toshiba/paz00-common/alsa/pcm/dmix.conf:/system/usr/share/alsa/pcm/dmix.conf \
    device/toshiba/paz00-common/alsa/pcm/default.conf:/system/usr/share/alsa/pcm/default.conf \
    device/toshiba/paz00-common/alsa/pcm/surround40.conf:/system/usr/share/alsa/pcm/surround40.conf \
    device/toshiba/paz00-common/alsa/pcm/surround41.conf:/system/usr/share/alsa/pcm/surround41.conf \
    device/toshiba/paz00-common/alsa/pcm/front.conf:/system/usr/share/alsa/pcm/front.conf \
    device/toshiba/paz00-common/alsa/pcm/rear.conf:/system/usr/share/alsa/pcm/rear.conf \
    device/toshiba/paz00-common/alsa/pcm/surround71.conf:/system/usr/share/alsa/pcm/surround71.conf

PRODUCT_PACKAGES := \
    make_ext4fs \
    com.android.future.usb.accessory \
    hwprops

PRODUCT_PROPERTY_OVERRIDES := \
    ro.opengles.version=131072 \
    wifi.interface=wlan0 \
    keyguard.no_require_sim=true \
    hwui.render_dirty_regions=false \
    ro.sf.lcd_density=120 

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.secure=0 \
    persist.sys.strictmode.visual=0

ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0
ADDITIONAL_DEFAULT_PROPERTIES += persist.sys.strictmode.visual=0

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_AAPT_CONFIG := xlarge mdpi

DEVICE_PACKAGE_OVERLAYS := \
    device/toshiba/paz00-common/overlay

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
    librs_jni \
    liba2dp \
    lights.tegra \
    com.android.future.usb.accessory \
    camera.tegra \
    libpkip \
    libaudioutils \
    tinyplay \
    tinycap \
    tinymix \
    audio.primary.tegra \
    audio.a2dp.default \
    FolioParts \
    wmiconfig

# Filesystem management tools
PRODUCT_PACKAGES += \
    make_ext4fs

# Force rotation to landscape, not clean solution
#PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
#    ro.sf.hwrotation=270 \
#    ro.sf.fakerotation=true

# Extra apps
PRODUCT_PACKAGES += \
    libhuaweigeneric-ril \
    libmbm-ril \
    FileManager \
    dropbear \
    VideoChatCameraTestApp \
    RpcPerformance \
    procstatlog \
    sensors.tegra \
    hciattach \
    vim 

$(call inherit-product-if-exists, vendor/toshiba/paz00/device-vendor.mk)
$(call inherit-product, frameworks/base/build/phone-hdpi-512-dalvik-heap.mk)
