/* ST-Ericsson U300 RIL
**
** Copyright (C) Ericsson AB 2011
** Copyright (C) ST-Ericsson AB 2008-2010
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** Based on reference-ril by The Android Open Source Project.
**
*/

#include <telephony/ril.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"

#include "u300-ril-device.h"
#include "u300-ril-messaging.h"
#include "u300-ril-sim.h"
#include "u300-ril-network.h"
#include "u300-ril.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define RADIO_POWER_ATTEMPTS 10
static RIL_RadioState sState = RADIO_STATE_UNAVAILABLE;
static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static char** s_deviceInfo = NULL;
static pthread_mutex_t s_deviceInfo_mutex = PTHREAD_MUTEX_INITIALIZER;

char *getTime(void)
{
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    char *currtime = NULL;
    char *resp = NULL;

    err = at_send_command_singleline("AT+CCLK?", "+CCLK:", &atresponse);

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read current time */
    err = at_tok_nextstr(&line, &currtime);
    if (err < 0)
        goto error;

    /* Skip the first two digits of year */
    resp = strdup(currtime+2);

finally:
    at_response_free(atresponse);
    return resp;

error:
    ALOGE("%s() Failed to read current time", __func__);
    goto finally;
}

void sendTime(void *p)
{
    time_t t;
    struct tm tm;
    char *timestr;
    char *currtime;
    char str[20];
    char tz[6];
    int num[4];
    int tzi;
    int i;
    (void) p;

    tzset();
    t = time(NULL);

    if (!(localtime_r(&t, &tm)))
        return;
    if (!(strftime(tz, 12, "%z", &tm)))
        return;

    for (i = 0; i < 4; i++)
        num[i] = tz[i+1] - '0';

    /* convert timezone hours to timezone quarters of hours */
    tzi = (num[0] * 10 + num[1]) * 4 + (num[2] * 10 + num[3]) / 15;
    strftime(str, 20, "%y/%m/%d,%T", &tm);
    asprintf(&timestr, "%s%c%02d", str, tz[0], tzi);

    /* Read time first to make sure an update is necessary */
    currtime = getTime();
    if (NULL == currtime)
        return;

    if (NULL == strstr(currtime, timestr))
        at_send_command("AT+CCLK=\"%s\"", timestr);
    else
        ALOGW("%s() Skipping setting same time again!", __func__);

    free(timestr);
    free(currtime);
    return;
}

void clearDeviceInfo(void)
{
    int i = 0;
    int err;

    if ((err = pthread_mutex_lock(&s_deviceInfo_mutex)) != 0)
        ALOGE("%s() failed to take device info mutex: %s!", __func__, strerror(err));
    else {
        if (s_deviceInfo != NULL) {
            /* s_deviceInfo list shall end with NULL */
            while (s_deviceInfo[i] != NULL) {
                free(s_deviceInfo[i]);
                i++;
            }
            free(s_deviceInfo);
        }

        if ((err = pthread_mutex_unlock(&s_deviceInfo_mutex)) != 0)
            ALOGE("%s() failed to release device info mutex: %s!", __func__, strerror(err));
    }
}

/* Needs to be called while unsolicited responses are not yet enabled, because
 * response prefix in at_send_command_multiline calls is "\0".
 */
void readDeviceInfo(void)
{
    ATResponse *atresponse = NULL;
    ATLine *line;
    int linecnt = 0;
    int err;

    clearDeviceInfo();

    err = at_send_command_multiline("AT*EEVINFO", "\0", &atresponse);
    if (err != AT_NOERROR) {
        /* Older device types might implement AT*EVERS instead of *EEVINFO */
        err = at_send_command_multiline("AT*EVERS", "\0", &atresponse);
        if (err != AT_NOERROR)
            return;
    }

    /* First just count intermediate responses */
    linecnt = 0;
    line = atresponse->p_intermediates;
    while (line) {
        linecnt++;
        line = line->p_next;
    }

    if ((err = pthread_mutex_lock(&s_deviceInfo_mutex)) != 0)
        ALOGE("%s() failed to take device info mutex: %s!", __func__, strerror(err));
    else {
        if (linecnt > 0) {
            s_deviceInfo = calloc(linecnt + 1, sizeof(char *));

            if (s_deviceInfo) {
                /* Now read and store the intermediate responses */
                linecnt = 0;
                line = atresponse->p_intermediates;
                while (line) {
                    s_deviceInfo[linecnt] = strdup(line->line);
                    if (s_deviceInfo[linecnt])
                        linecnt++;
                    else
                        ALOGW("%s() failed to allocate memory", __func__);

                    line = line->p_next;
                }
                /* Mark end of list with NULL */
                s_deviceInfo[linecnt] = NULL;
            }
            else
                ALOGW("%s() failed to allocate memory", __func__);
        }

        if ((err = pthread_mutex_unlock(&s_deviceInfo_mutex)) != 0)
            ALOGE("%s() failed to release device info mutex: %s!", __func__, strerror(err));
    }
    at_response_free(atresponse);
}

char *getDeviceInfo(const char *info)
{
    int i = 0;
    int err;
    char* resp = NULL;

    if ((err = pthread_mutex_lock(&s_deviceInfo_mutex)) != 0)
        ALOGE("%s() failed to take device info mutex: %s!", __func__, strerror(err));
    else {
        if (s_deviceInfo != NULL) {
            /* s_deviceInfo list always ends with a NULL */
            while (s_deviceInfo[i] != NULL) {
                if (strStartsWith(s_deviceInfo[i], info)) {
                    resp = calloc(strlen(s_deviceInfo[i]), sizeof(char));
                    sscanf(s_deviceInfo[i]+strlen(info), "%*s %s", resp);
                    break;
                }
                i++;
            }
        }
        if ((err = pthread_mutex_unlock(&s_deviceInfo_mutex)) != 0)
            ALOGE("%s() failed to release device info mutex: %s!", __func__, strerror(err));
    }

    if (resp == NULL)
        ALOGW("%s() didn't find information for %s", __func__, info);

    return resp;
}

/**
 * RIL_REQUEST_GET_IMSI
*/
void requestGetIMSI(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    ATResponse *atresponse = NULL;
    int err;

    err = at_send_command_numeric("AT+CIMI", &atresponse);

    if (err != AT_NOERROR)
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS,
                              atresponse->p_intermediates->line,
                              sizeof(char *));
        at_response_free(atresponse);
    }
}

/* RIL_REQUEST_DEVICE_IDENTITY
 *
 * Request the device ESN / MEID / IMEI / IMEISV.
 *
 */
void requestDeviceIdentity(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    char* response[4];

    response[0] = getDeviceInfo("IMEI Data"); /* IMEI */
    response[1] = getDeviceInfo("SVN"); /* IMEISV */

    /* CDMA not supported */
    response[2] = "";
    response[3] = "";

    if (response[0] && response[1])
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
    else
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

    free(response[0]);
    free(response[1]);
}

/* Deprecated */
/**
 * RIL_REQUEST_GET_IMEI
 *
 * Get the device IMEI, including check digit.
*/
void requestGetIMEI(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    char *imei;

    imei = getDeviceInfo("IMEI Data");
    if (imei)
        RIL_onRequestComplete(t, RIL_E_SUCCESS, imei, sizeof(char *));
    else
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

    free(imei);
}

/* Deprecated */
/**
 * RIL_REQUEST_GET_IMEISV
 *
 * Get the device IMEISV, which should be two decimal digits.
*/
void requestGetIMEISV(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    char *svn;

    /* IMEISV */
    svn = getDeviceInfo("SVN");
    if (svn)
        RIL_onRequestComplete(t, RIL_E_SUCCESS, svn, sizeof(char *));
    else
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

    free(svn);
}

/**
 * RIL_REQUEST_BASEBAND_VERSION
 *
 * Return string value indicating baseband version, eg
 * response from AT+CGMR.
*/
void requestBasebandVersion(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    char *ver;

    ver = getDeviceInfo("Protocol FW Version");
    if (ver)
        RIL_onRequestComplete(t, RIL_E_SUCCESS, ver, sizeof(char *));
    else
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

    free(ver);
}

/** Do post- SIM ready initialization. */
void onSIMReady(void *p)
{
    int err = 0;
    int screenState;
    (void) p;

    /* Check if ME is ready to set preferred message storage */
    checkMessageStorageReady(NULL);

    /* Select message service */
    at_send_command("AT+CSMS=0");

   /* Configure new messages indication
    *  mode = 2 - Buffer unsolicited result code in TA when TA-TE link is
    *             reserved(e.g. in on.line data mode) and flush them to the
    *             TE after reservation. Otherwise forward them directly to
    *             the TE.
    *  mt   = 2 - SMS-DELIVERs (except class 2 messages and messages in the
    *             message waiting indication group (store message)) are
    *             routed directly to TE using unsolicited result code:
    *             +CMT: [<alpha>],<length><CR><LF><pdu> (PDU mode)
    *             Class 2 messages are handled as if <mt> = 1
    *  bm   = 2 - New CBMs are routed directly to the TE using unsolicited
    *             result code:
    *             +CBM: <length><CR><LF><pdu> (PDU mode)
    *  ds   = 1 - SMS-STATUS-REPORTs are routed to the TE using unsolicited
    *             result code: +CDS: <length><CR><LF><pdu> (PDU mode)
    *  bfr  = 0 - TA buffer of unsolicited result codes defined within this
    *             command is flushed to the TE when <mode> 1...3 is entered
    *             (OK response is given before flushing the codes).
    */
    at_send_command("AT+CNMI=2,2,2,1,0");

    /* Subscribe to network status events */
    at_send_command("AT*E2REG=1");

    /* Configure Short Message (SMS) Format
     *  mode = 0 - PDU mode.
     */
    at_send_command("AT+CMGF=0");

    /* Subscribe to time zone/NITZ reporting.
     *
     */
    err = at_send_command("AT*ETZR=3");
    if (err != AT_NOERROR) {
        ALOGD("%s() Degrading nitz to mode 2", __func__);
        at_send_command("AT*ETZR=2");
    }

    /* Delete Internet Account Configuration.
     *  Some FW versions has an issue, whereby internet account configuration
     *  needs to be cleared explicitly.
     */
    at_send_command("AT*EIAD=0,0");

    /* Make sure currect screenstate is set */
    getScreenStateLock();
    screenState = getScreenState();
    setScreenState(screenState);
    releaseScreenStateLock();

}

static const char *radioStateToString(RIL_RadioState radioState)
{
    const char *state;

    switch (radioState) {
    case RADIO_STATE_OFF:
        state = "RADIO_STATE_OFF";
        break;
    case RADIO_STATE_UNAVAILABLE:
        state = "RADIO_STATE_UNAVAILABLE";
        break;
    case RADIO_STATE_SIM_NOT_READY:
        state = "RADIO_STATE_SIM_NOT_READY";
        break;
    case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        state = "RADIO_STATE_SIM_LOCKED_OR_ABSENT";
        break;
    case RADIO_STATE_SIM_READY:
        state = "RADIO_STATE_SIM_READY";
        break;
    case RADIO_STATE_RUIM_NOT_READY:
        state = "RADIO_STATE_RUIM_NOT_READY";
        break;
    case RADIO_STATE_RUIM_READY:
        state = "RADIO_STATE_RUIM_READY";
        break;
    case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
        state = "RADIO_STATE_RUIM_READY";
        break;
    case RADIO_STATE_NV_NOT_READY:
        state = "RADIO_STATE_NV_NOT_READY";
        break;
    case RADIO_STATE_NV_READY:
        state = "RADIO_STATE_NV_READY";
        break;
    case RADIO_STATE_ON:
        state = "RADIO_STATE_ON";
        break;
    default:
        state = "RADIO_STATE_<> Unknown!";
        break;
    }

    return state;
}

void setRadioState(RIL_RadioState newState)
{
    RIL_RadioState oldState;
    int err;

    if ((err = pthread_mutex_lock(&s_state_mutex)) != 0)
        ALOGE("%s() failed to take state mutex: %s!", __func__, strerror(err));

    oldState = sState;

    ALOGI("%s() oldState=%s newState=%s", __func__, radioStateToString(oldState),
         radioStateToString(newState));

    sState = newState;

    if ((err = pthread_mutex_unlock(&s_state_mutex)) != 0)
        ALOGE("%s() failed to release state mutex: %s!", __func__, strerror(err));

    /* Do these outside of the mutex. */
    if (sState != oldState || sState == RADIO_STATE_SIM_LOCKED_OR_ABSENT) {
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                  NULL, 0);

        if (sState == RADIO_STATE_SIM_READY) {
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, checkMessageStorageReady, NULL, NULL);
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, onSIMReady, NULL, NULL);
        } else if (sState == RADIO_STATE_SIM_NOT_READY)
            enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, pollSIMState, NULL, NULL);
    }
}

/** Returns 1 if on, 0 if off, and -1 on error. */
int isRadioOn(void)
{
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    int ret;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &atresponse);
    if (err != AT_NOERROR)
        /* Assume radio is off. */
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &ret);
    if (err < 0)
        goto error;

    switch (ret) {
        case 1:         /* Full functionality (switched on) */
        case 5:         /* GSM only */
        case 6:         /* WCDMA only */
            ret = 1;
            break;

        default:
            ret = 0;
    }

    at_response_free(atresponse);

    return ret;

error:
    at_response_free(atresponse);
    return -1;
}

/*
 * Retry setting radio power a few times
 * Needed since the module reports EMRDY
 * before it is actually ready. Without
 * this we could get CME ERROR 272 (wwan
 * disabled on host) when sending CFUN=1
 */
int retryRadioPower(void)
{
    int err;
    int i;

    ALOGD("%s()", __func__);
    for (i=0; i<RADIO_POWER_ATTEMPTS; i++) {
        sleep(1);
        err = at_send_command("AT+CFUN=%d", getPreferredNetworkType());
        if (err == AT_NOERROR) {
            return 0;
        }
    }

    return -1;
}

/**
 * RIL_REQUEST_RADIO_POWER
 *
 * Toggle radio on and off (for "airplane" mode).
*/
void requestRadioPower(void *data, size_t datalen, RIL_Token t)
{
    (void) datalen;
    int onOff;
    int err;
    int restricted;

    if (datalen < sizeof(int *)) {
        ALOGE("%s() bad data length!", __func__);
        goto error;
    }

    onOff = ((int *) data)[0];

    if (onOff == 0 && sState != RADIO_STATE_OFF) {
        char value[PROPERTY_VALUE_MAX];

        err = at_send_command("AT+CFUN=4");

        if (err != AT_NOERROR)
            goto error;

        if (property_get("sys.shutdown.requested", value, NULL)) {
            setRadioState(RADIO_STATE_UNAVAILABLE);
            err = at_send_command("AT+CFUN=0");
            if (err != AT_NOERROR)
                goto error;
        } else
            setRadioState(RADIO_STATE_OFF);
    } else if (onOff > 0 && sState == RADIO_STATE_OFF) {
        err = at_send_command("AT+CFUN=%d", getPreferredNetworkType());
        if (err != AT_NOERROR) {
            if (retryRadioPower() < 0)
                goto error;
        }
        setRadioState(RADIO_STATE_SIM_NOT_READY);
    } else {
        ALOGE("%s() Erroneous input", __func__);
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    restricted = RIL_RESTRICTED_STATE_NONE;
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESTRICTED_STATE_CHANGED,
                              &restricted, sizeof(int *));

    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * Synchronous call from the RIL to us to return current radio state.
 * RADIO_STATE_UNAVAILABLE should be the initial state.
 */
RIL_RadioState getRadioState(void)
{
    return sState;
}
