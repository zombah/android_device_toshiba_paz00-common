/*
 * Copyright (C) 2011 The Android Open Source Project
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

/*
 * Contains implementation of a class CameraFactory that manages cameras
 * available
 */

#define LOG_NDEBUG 0
#define DEFAULT_DEVICE_FRONT "/dev/video1"
#define DEFAULT_DEVICE_BACK  "/dev/video0"
#define CONFIG_FILE "/etc/camera.cfg"
#define LOG_TAG "Camera_Factory"

#include <cutils/log.h>
#include <cutils/properties.h>
#include "CameraFactory.h"

extern camera_module_t HAL_MODULE_INFO_SYM;

/* A global instance of CameraFactory is statically instantiated and
 * initialized when camera HAL is loaded.
 */
android::CameraFactory  gCameraFactory;

namespace android {

CameraFactory::CameraFactory()
{
    LOGD("CameraFactory::CameraFactory");
    mCamera = NULL;
    mCameraDevices = NULL;
    mCameraOrientation = NULL;
    parseConfig(CONFIG_FILE);
}

CameraFactory::~CameraFactory()
{
    LOGD("CameraFactory::~CameraFactory");
    for (int i=0; i < getCameraNum(); i++) {
        delete mCamera[i];
    }
    free(mCamera);
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate Camera
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int CameraFactory::cameraDeviceOpen(const hw_module_t* module,int camera_id, hw_device_t** device)
{
    LOGD("CameraFactory::cameraDeviceOpen: id = %d", camera_id);

    *device = NULL;

    if (!mCamera || camera_id < 0 || camera_id >= getCameraNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getCameraNum());
        return -EINVAL;
    }

    if (!mCamera[camera_id]) {
        mCamera[camera_id] = new CameraHardware(module, mCameraDevices[camera_id]);
    }
    return mCamera[camera_id]->connectCamera(device);
}

/* Returns the number of available cameras */
int CameraFactory::getCameraNum()
{
    LOGD("CameraFactory::getCameraNum: %d", mCameraNum);
    return mCameraNum;
}

int CameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    LOGD("CameraFactory::getCameraInfo: id = %d, info = %p", camera_id, info);

    if (camera_id < 0 || camera_id >= getCameraNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
                __FUNCTION__, camera_id, getCameraNum());
        return -EINVAL;
    }

    return CameraHardware::getCameraInfo(info, mCameraOrientation[camera_id]);
}

// Parse a simple configuration file
void CameraFactory::parseConfig(const char* configFile)
{
    LOGD("CameraFactory::parseConfig: configFile = %s", configFile);

    FILE* config = fopen(configFile, "r");
    if (config != NULL) {
        char line[128];
        char arg1[128];
        char arg2[128];

        while (fgets(line, sizeof line, config) != NULL) {
            int lineStart = strspn(line, " \t\n\v" );

            if (line[lineStart] == '#')
                continue;

            sscanf(line, "%s %s", arg1, arg2);

            if (strcmp(arg1, "front")) {
                newCameraConfig(CAMERA_FACING_FRONT, arg2);
            } else if (strcmp(arg1, "back")) {
                newCameraConfig(CAMERA_FACING_BACK, arg2);
            } else {
                LOGD("CameraFactory::parseConfig: Unrecognized config line '%s'", line);
            }
        }
    } else {
        LOGD("%s not found, using camera configuration defaults", CONFIG_FILE);
        if (access(DEFAULT_DEVICE_BACK, F_OK) != -1){
            LOGD("Found device %s", DEFAULT_DEVICE_BACK);
            newCameraConfig(CAMERA_FACING_BACK, DEFAULT_DEVICE_BACK);
        }
        if (access(DEFAULT_DEVICE_FRONT, F_OK) != -1){
            LOGD("Found device %s", DEFAULT_DEVICE_FRONT);
            newCameraConfig(CAMERA_FACING_FRONT, DEFAULT_DEVICE_FRONT);
        }
    }
}

// Although realloc could be a costly operation, we only execute this function usually 2 times
void CameraFactory::newCameraConfig(int facing, const char* location)
{
    // Keep track of cameras
    mCameraNum++;

    // Grow the information arrays
    mCamera = (CameraHardware**) realloc(mCamera, mCameraNum * sizeof(CameraHardware*));
    mCameraDevices = (char**) realloc(mCameraDevices, mCameraNum * sizeof(char*));
    mCameraOrientation = (int*) realloc(mCameraOrientation, mCameraNum * sizeof(int));

    // Store the values for each camera_id
    mCamera[mCameraNum - 1] = NULL;
    mCameraDevices[mCameraNum - 1] = strdup(location);
    mCameraOrientation[mCameraNum - 1] = facing;
    LOGD("CameraFactory::newCameraConfig: %d -> %s", mCameraOrientation[mCameraNum - 1], mCameraDevices[mCameraNum - 1]);
}

/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

int CameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
    LOGD("CameraFactory::device_open: name = %s", name);

    /*
     * Simply verify the parameters, and dispatch the call inside the
     * CameraFactory instance.
     */

    if (module != &HAL_MODULE_INFO_SYM.common) {
        LOGE("%s: Invalid module %p expected %p",
                __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        LOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    int camera_id = atoi(name);
    return gCameraFactory.cameraDeviceOpen(module, camera_id, device);
}

int CameraFactory::get_number_of_cameras(void)
{
    LOGD("CameraFactory::get_number_of_cameras");
    return gCameraFactory.getCameraNum();
}

int CameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
    LOGD("CameraFactory::get_camera_info");
    return gCameraFactory.getCameraInfo(camera_id, info);
}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t CameraFactory::mCameraModuleMethods = {
    open: CameraFactory::device_open
};

}; /* namespace android */
