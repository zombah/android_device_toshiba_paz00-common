/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2012 ac100 Russian community
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0
#define LOG_TAG "lights"
#define LOGE ALOGE

#include <cutils/log.h>

#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>

#include <hardware/lights.h>
#include <hardware/hardware.h>

#define LIGHT_ATTENTION 1
#define LIGHT_NOTIFY    2

/******************************************************************************/

static struct light_state_t *g_notify;
static struct light_state_t *g_attention;
static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * device methods
 */

static int write_int(char const *path, int value)
{
	ALOGV("write_int is called");
	int fd;
	static int already_warned = -1;
	fd = open(path, O_RDWR);
	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == -1) {
			LOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int write_string(char const *path, char const *value)
{
	ALOGV("write_string is called");
        int fd;
        static int already_warned = -1;
        fd = open(path, O_RDWR);
        if (fd >= 0) {
                char buffer[20];
                int bytes = sprintf(buffer, "%s\n", value);
                int amt = write(fd, buffer, bytes);
                close(fd);
                return amt == -1 ? -errno : 0;
        } else {
                if (already_warned == -1) {
                        LOGE("write_int failed to open %s\n", path);
                        already_warned = 1;
                }
                return -errno;
        }
}

void init_globals(void)
{
	ALOGV("inits_globals is called");
        pthread_mutex_init(&g_lock, NULL);

        g_attention = malloc(sizeof(struct light_state_t));
        memset(g_attention, 0, sizeof(*g_attention));
        g_notify = malloc(sizeof(struct light_state_t));
        memset(g_notify, 0, sizeof(*g_notify));
}

static int rgb_to_brightness(struct light_state_t const *state)
{
	ALOGV("rgb_to_brightness is called");
	int color = state->color & 0x00ffffff;
	int red = (color >> 16) & 0x000000ff;
        int green = (color >> 8) & 0x000000ff;
        int blue = color & 0x000000ff;

        int brightness = red;
        if (green > brightness)
                brightness = green;
        if (blue > brightness)
                brightness = blue;

        return brightness;
}

static int set_light_backlight(struct light_device_t *dev,
			       struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);
	ALOGV("%s brightness=%d, brightnessMode=%d, flashMode=%d, onMS=%d, offMS=%d, colorRGB=%08X\n",
                        __func__, brightness, state->brightnessMode, state->flashMode, state->flashOnMS, state->flashOffMS, state->color);
	//+++ power save
	int range_num = 1;
	int old_brightness_range_indexs[] = {160,255};
	int new_brightness_range_indexs[] = {160,182};
	int old_brightness = brightness;
	int i=0;
	int old_brightness_in_range = -1;// 0:1~10 , 1:10~102 , 2:102~255
	for(i=0;i<range_num;i++){
		int t_range_min = old_brightness_range_indexs[i];
		int t_range_max = old_brightness_range_indexs[i+1];
		if( brightness>=t_range_min && brightness<=t_range_max){
			old_brightness_in_range = i;
			break;
		}
	}
	if(old_brightness_in_range>=0){
		int old_br_min = old_brightness_range_indexs[old_brightness_in_range];
		int olb_br_max = old_brightness_range_indexs[old_brightness_in_range+1];
		int new_br_min = new_brightness_range_indexs[old_brightness_in_range];
		int new_br_max = new_brightness_range_indexs[old_brightness_in_range+1];
		int new_brightness = new_br_min +
				(new_br_max-new_br_min)*(old_brightness-old_br_min)/(olb_br_max-old_br_min);
		brightness = new_brightness;
	}
	//---
	pthread_mutex_lock(&g_lock);
	err = write_int("/sys/class/backlight/backlight/brightness",
			brightness);
	pthread_mutex_unlock(&g_lock);

	return err;
}

static int
set_notification_light(struct light_state_t const* state)
{
	ALOGV("set_notification_light is called");
        unsigned int brightness = rgb_to_brightness(state);
        int blink = state->flashOnMS;

        ALOGV("set_notification_light brightnessMode=%d, colorRGB=%08X, flashMode=%d, onMS=%d, offMS=%d\n",
                        state->brightnessMode, state->color, state->flashMode, state->flashOnMS, state->flashOffMS);

        write_int("/sys/class/leds/paz00-led/brightness", blink);

	ALOGV("blink written");

        return 0;
}

static void
handle_notification_light_locked(int type)
{
	ALOGV("handle_notification_light_locked is called");
        struct light_state_t *new_state = 0;
        int attn_mode = 0;

        if (g_attention->flashMode == LIGHT_FLASH_HARDWARE)
                attn_mode = g_attention->flashOnMS;

        switch (type) {
                case LIGHT_ATTENTION: {
                        if (attn_mode == 0) {
                                /* go back to notify state */
                                new_state = g_notify;
                        } else {
                                new_state = g_attention;
                        }
		ALOGV("%s: case LIGHT_ATTENTION", __func__);
                break;
                }
                case LIGHT_NOTIFY: {
                        if (attn_mode != 0) {
                                /* attention takes priority over notify state */
                                new_state = g_attention;
                        } else {
                                new_state = g_notify;
                        }
		ALOGV("%s: case LIGHT_NOTIFY", __func__);
                break;
                }
        }
        if (new_state == 0) {
                ALOGE("%s: unknown type (%d)\n", __func__, type);
                return;
        }

        set_notification_light(new_state);
}

static int
set_light_notifications(struct light_device_t* dev,
                struct light_state_t const* state)
{
	ALOGV("set_light_notifications is called");
        pthread_mutex_lock(&g_lock);

        g_notify->color = state->color;
        if (state->flashMode != LIGHT_FLASH_NONE) {
                g_notify->flashMode = LIGHT_FLASH_HARDWARE;
                g_notify->flashOnMS = state->flashOnMS;
                g_notify->flashOffMS = state->flashOffMS;
        } else {
                g_notify->flashOnMS = 0;
                g_notify->flashOffMS = 0;
        }
        handle_notification_light_locked(LIGHT_NOTIFY);

        pthread_mutex_unlock(&g_lock);
        return 0;
}

static int
set_light_attention(struct light_device_t* dev,
                struct light_state_t const* state)
{
	ALOGV("set_light_attention is called");
        pthread_mutex_lock(&g_lock);

	g_attention->brightnessMode = state->brightnessMode;
        g_attention->flashMode = state->flashMode;
        g_attention->flashOnMS = state->flashOnMS;
        g_attention->color = state->color;
        g_attention->flashOffMS = state->flashOffMS;
        handle_notification_light_locked(LIGHT_ATTENTION);

        pthread_mutex_unlock(&g_lock);
        return 0;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
	ALOGV("close_light is called");
	if (dev)
		free(dev);
	return 0;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t *module, char const *name,
		       struct hw_device_t **device)
{
	pthread_t lighting_poll_thread;

	int (*set_light) (struct light_device_t *dev,
			  struct light_state_t const *state);

	ALOGV("open_lights: open with %s", name);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
		set_light = set_light_notifications;
	else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
                set_light = set_light_attention;
	else
		return -EINVAL;

	pthread_once(&g_init, init_globals);

	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t *)module;
	dev->common.close = (int (*)(struct hw_device_t *))close_lights;
	dev->set_light = set_light;

	*device = (struct hw_device_t *)dev;

	return 0;
}

static struct hw_module_methods_t lights_methods =
{
	.open =  open_lights,
};

/*
 * The backlight Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM =
{
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "NVIDIA Kai lights module",
	.author = "NVIDIA",
	.methods = &lights_methods,
};
