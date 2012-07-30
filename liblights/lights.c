/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "lights"

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

/** эти дефайны нигде не используются */
//#define LIGHT_ATTENTION	1
//#define LIGHT_NOTIFY 	2

/******************************************************************************/


static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * device methods
 */

static int write_int(char const *path, int value)
{
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
	pthread_mutex_init(&g_lock, NULL);

}

static int rgb_to_brightness(struct light_state_t const *state)
{
	/* use max of the RGB components for brightness */
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

static int
set_light_backlight(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);

	pthread_mutex_lock(&g_lock);
	err = write_int("/sys/class/backlight/pwm-backlight/brightness", brightness);
	pthread_mutex_unlock(&g_lock);

	return err;
}

/**
 Непосредственно включение/выключение диодов тошибы
mode:
 1 - включить;
 0 - выключить;
*/
static int
set_leds_locked(int mode)
{
	int err = 0;
	err = write_int("/sys/class/leds/nvec-led/brightness", mode);
	return err;
}


/** Попытка реализации моргания по уведомлениям */
static int
set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);

    // считывание переменных
    unsigned int color = state->color;
    int flashOnMS = state->flashOnMS;
    int flashOffMS = state->flashOffMS;

    // управление включением/выключением
    // пока примитивный алгоритм
    if ((color != 0)||(flashOnMS != 0)) set_leds_locked(1);
    else if ((color == 0)||((flashOnMS == 0)&&(flashOnMS == 0))) set_leds_locked(0);

    // TODO: можно смотреть значения flashOnMS, flashOffMS
    // и устанавливать подходящий режим из 1..8
    // перед этим надо будет ещё проверять flashMode

    pthread_mutex_unlock(&g_lock);
    return 0;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
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

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;

	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
		set_light = set_light_notifications;
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

static struct hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};

/*
 * The lights Module
 */
const struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "Toshiba ac100 lights Module",
	.author = "ac100 Russian community",
	.methods = &lights_module_methods,
};
