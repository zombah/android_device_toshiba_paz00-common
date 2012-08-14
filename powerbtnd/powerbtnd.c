/**
 * A daemon to simulate power button of Android
 *
 * Copyright (C) 2011-2012 The Android-x86 Open Source Project
 *
 * by Chih-Wei Huang <cwhuang@linux.org.tw>
 *
 * Licensed under GPLv2 or later
 *
 **/

#define LOG_TAG "powerbtn"

#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <cutils/log.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <cutils/properties.h>

const int MAX_POWERBTNS = 3;

int openfds(struct pollfd pfds[])
{
	int cnt = 0;
	const char *dirname = "/dev/input";
	DIR *dir;
	if ((dir = opendir(dirname))) {
		int fd;
		struct dirent *de;
		while ((de = readdir(dir))) {
			if (de->d_name[0] != 'e') // eventX
				continue;
			char name[PATH_MAX];
			snprintf(name, PATH_MAX, "%s/%s", dirname, de->d_name);
			fd = open(name, O_RDWR | O_NONBLOCK);
			if (fd < 0) {
				LOGE("could not open %s, %s", name, strerror(errno));
				continue;
			}
			name[sizeof(name) - 1] = '\0';
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				LOGE("could not get device name for %s, %s\n", name, strerror(errno));
				name[0] = '\0';
			}

			// TODO: parse /etc/excluded-input-devices.xml
			if (!strcmp(name, "NVEC power button")) {
				LOGI("open %s(%s) ok", de->d_name, name);
				pfds[cnt].events = POLLIN;
				pfds[cnt++].fd = fd;
				if (cnt < MAX_POWERBTNS)
					continue;
				else
					break;
			}
			close(fd);
		}
		closedir(dir);
	}

	return cnt;
}

void send_power(int ufd, int down)
{
	struct input_event iev;
	iev.type  = EV_KEY;
	iev.code  = KEY_POWER;
	iev.value = down;
	write(ufd, &iev, sizeof(iev));
	iev.type  = EV_SYN;
	iev.code  = SYN_REPORT;
	iev.value = 0;
	write(ufd, &iev, sizeof(iev));
}

void simulate_powerkey(int ufd, int longpress)
{
	send_power(ufd, 1);
	if (longpress)
		sleep(2);
	send_power(ufd, 0);
}

int main()
{
	struct pollfd pfds[MAX_POWERBTNS];
	int cnt = openfds(pfds);
	int timeout = -1;
	char prop[PROPERTY_VALUE_MAX];

	int ufd = open("/dev/uinput", O_WRONLY | O_NDELAY);
	if (ufd >= 0) {
		struct uinput_user_dev ud;
		memset(&ud, 0, sizeof(ud));
		strcpy(ud.name, "Android Power Button");
		write(ufd, &ud, sizeof(ud));
		ioctl(ufd, UI_SET_EVBIT, EV_KEY);
		ioctl(ufd, UI_SET_KEYBIT, KEY_POWER);
		ioctl(ufd, UI_DEV_CREATE, 0);
	} else {
		LOGE("could not open uinput device: %s", strerror(errno));
		return -1;
	}

	property_get("poweroff.doubleclick", prop, NULL);

	for (;;) {
		int i;
		int pollres = poll(pfds, cnt, timeout) ;
		LOGV("pollres=%d %d\n", pollres, timeout);
		if (pollres < 0) {
			LOGE("poll error: %s", strerror(errno));
			break;
		}
		if (pollres == 0) { // timeout, send one power key
			simulate_powerkey(ufd, 0);
			timeout = -1;
			continue;
		}
		for (i = 0; i < cnt; ++i) {
			if (pfds[i].revents & POLLIN) {
				struct input_event iev;
				size_t res = read(pfds[i].fd, &iev, sizeof(iev));
				if (res < sizeof(iev)) {
					LOGW("insufficient input data(%d)? fd=%d", res, pfds[i].fd);
					continue;
				}
				LOGV("type=%d scancode=%d value=%d from fd=%d", iev.type, iev.code, iev.value, pfds[i].fd);
				if (iev.type == EV_KEY && iev.code == KEY_POWER && !iev.value) {
					if (prop[0] != '1' || timeout > 0) {
						simulate_powerkey(ufd, 1);
						timeout = -1;
					} else {
						timeout = 1000; // one second
					}
				}
			}
		}
	}

	return 0;
}
