/*
 * Copyright (C) 2011 Linaro Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <linux/uinput.h>

/* look up source file system/core/init/devices.c for exact node */
#define UINPUT_DEV "/dev/uinput"

#define DEV_NAME "Fake Touchscreen"

static int uinput_fd = 0;

static void uinput_touch_init(const char* uinput_dev,
                              const char* dev_name)
{
    struct uinput_user_dev dev;

    uinput_fd = open(uinput_dev, O_WRONLY);
    if (uinput_fd <= 0) {
        perror("Error opening uinput device.\n");
        return;
    }
    memset(&dev, 0, sizeof(dev));
    strcpy(dev.name, dev_name);
    write(uinput_fd, &dev, sizeof(dev));

    /* touch screen event */
    ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_TOUCH);

    /* register userspace input device */
    ioctl(uinput_fd, UI_DEV_CREATE, 0);
}

static void uinput_touch_deinit()
{
    if (uinput_fd > 0) {
        close(uinput_fd);
    }
}

int main(int argc, char* argv[])
{
    uinput_touch_init(UINPUT_DEV, DEV_NAME);

    while (1) {
        sleep(60);
    }

    uinput_touch_deinit();

    return 0;
}

