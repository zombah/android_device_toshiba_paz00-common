/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#define BTM_DEF_LOCAL_NAME "Toshiba AC100"
// At present either USB or UART is supported
#define BLUETOOTH_HCI_USE_USB          TRUE
// Bluetooth Low Power Mode is supported on BT4.0
#define HCILP_INCLUDED                 FALSE

// 10.2 Updates
#define BLE_INCLUDED FALSE
#define BTA_GATT_INCLUDED FALSE
#define SMP_INCLUDED FALSE
#define PAN_NAP_DISABLED TRUE

#endif
