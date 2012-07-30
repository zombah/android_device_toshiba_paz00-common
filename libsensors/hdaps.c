/**
 * HDAPS accelerometer sensor
 *
 * Copyright (C) 2011 The Android-x86 Open Source Project
 *
 * by Stefan Seidel <stefans@android-x86.org>
 *
 * Licensed under GPLv2 or later
 *
 **/

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>

#include <cutils/atomic.h>
#include <cutils/log.h>
#include <hardware/sensors.h>

//#define DEBUG_SENSOR		1

#define CONVERT			(GRAVITY_EARTH / 156.0f)
#define SENSOR_NAME		"hdaps"
#define INPUT_DIR		"/dev/input"
#define ARRAY_SIZE(a)		(sizeof(a) / sizeof(a[0]))
#define ID_ACCELERATION		(SENSORS_HANDLE_BASE + 0)

#define AMIN(a,b)		(((a)<(fabs(b)))?(a):(b))
#define SQUARE(x)		((x)*(x))
#define COS_ASIN(m,x)		(sqrt(SQUARE(m)-SQUARE(AMIN(m,x))))
#define COS_ASIN_2D(m,x,y)	(COS_ASIN(m,x)*COS_ASIN(m,y)/(m))

static unsigned int forced_delay = 0;

struct sensors_poll_context_t {
	struct sensors_poll_device_t device;
	int fd;
};

static int common__close(struct hw_device_t *dev) {
	struct sensors_poll_context_t *ctx = (struct sensors_poll_context_t *) dev;
	if (ctx) {
		free(ctx);
	}

	return 0;
}

static int device__activate(struct sensors_poll_device_t *dev, int handle,
		int enabled) {

	return 0;
}

static int device__set_delay(struct sensors_poll_device_t *device, int handle,
		int64_t ns) {
	forced_delay = ns / 1000;
	return 0;

}

static int device__poll(struct sensors_poll_device_t *device,
		sensors_event_t* data, int count) {

	struct input_event event;
	int ret;
	struct sensors_poll_context_t *dev =
			(struct sensors_poll_context_t *) device;

	if (dev->fd < 0)
		return 0;

	while (1) {

		ret = read(dev->fd, &event, sizeof(event));

#ifdef DEBUG_SENSOR
		LOGD("hdaps event %d - %d - %d\n", event.type, event.code, event.value);
#endif
		if (event.type == EV_ABS) {
			switch (event.code) {
			// Even though this mapping results in wrong results with some apps,
			// it just means that these apps are broken, i.e. they rely on the
			// fact that phone have portrait displays. Laptops/tablets however
			// have landscape displays, and the axes are relative to the default
			// screen orientation, not relative to portrait orientation.
			// See the nVidia Tegra accelerometer docs if you want to know for sure.
			case ABS_X:
				data->acceleration.x = (float) event.value * CONVERT;
				break;
			case ABS_Y:
				// we invert the y-axis here because we assume the laptop is
				// in tablet mode (and thus the display is rotated 180 degrees.
				data->acceleration.y = -(float) event.value * CONVERT;
				break;
			}
		} else if (event.type == EV_SYN) {
			data->timestamp = (int64_t) ((int64_t) event.time.tv_sec
					* 1000000000 + (int64_t) event.time.tv_usec * 1000);
			// hdaps doesn't have z-axis, so simulate it by rotation matrix solution
			data ->acceleration.z = COS_ASIN_2D(GRAVITY_EARTH, data->acceleration.x, data->acceleration.y);
			data->sensor = ID_ACCELERATION;
			data->type = SENSOR_TYPE_ACCELEROMETER;
			data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
			// spare the CPU if desired
			if (forced_delay)
				usleep(forced_delay);
			return 1;
		}
	}

	return -errno;
}

static int open_input_device(void) {
	char *filename;
	int fd;
	DIR *dir;
	struct dirent *de;
	char name[80];
	char devname[256];
	dir = opendir(INPUT_DIR);
	if (dir == NULL)
		return -1;

	strcpy(devname, INPUT_DIR);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		strcpy(filename, de->d_name);
		fd = open(devname, O_RDONLY);
		if (fd < 0) {
			continue;
		}

		if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
			name[0] = '\0';
		}

		if (!strcmp(name, SENSOR_NAME)) {
#ifdef DEBUG_SENSOR
			LOGI("devname is %s \n", devname);
#endif
		} else {
			close(fd);
			continue;
		}
		closedir(dir);

		return fd;

	}
	closedir(dir);

	return -1;
}

static const struct sensor_t sSensorList[] = {
	{	.name = "HDAPS accelerometer",
		.vendor = "Linux kernel",
		.version = 1,
		.handle = ID_ACCELERATION,
		.type = SENSOR_TYPE_ACCELEROMETER,
		.maxRange = (GRAVITY_EARTH * 6.0f),
		.resolution = (GRAVITY_EARTH * 6.0f) / 1024.0f,
		.power = 0.84f,
		.reserved = {},
	},
};

static int open_sensors(const struct hw_module_t* module, const char* name,
		struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
		struct sensor_t const** list) {
	*list = sSensorList;

	return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
		.open = open_sensors };

const struct sensors_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 1,
		.version_minor = 1,
		.id = SENSORS_HARDWARE_MODULE_ID,
		.name = "hdaps accelerometer sensor",
		.author = "Stefan Seidel",
		.methods = &sensors_module_methods,
		.dso = NULL,
		.reserved = {},
	},
	.get_sensors_list = sensors__get_sensors_list
};

static int open_sensors(const struct hw_module_t* module, const char* name,
		struct hw_device_t** device) {
	int status = -EINVAL;

	struct sensors_poll_context_t *dev = malloc(
			sizeof(struct sensors_poll_context_t));
	memset(&dev->device, 0, sizeof(struct sensors_poll_device_t));

	dev->device.common.tag = HARDWARE_DEVICE_TAG;
	dev->device.common.version = 0;
	dev->device.common.module = (struct hw_module_t*) module;
	dev->device.common.close = common__close;
	dev->device.activate = device__activate;
	dev->device.setDelay = device__set_delay;
	dev->device.poll = device__poll;

	if ((dev->fd = open_input_device()) < 0) {
		LOGE("g sensor get class path error \n");
	} else {
		*device = &dev->device.common;
		status = 0;
	}

	return status;
}
