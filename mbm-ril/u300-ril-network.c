/* ST-Ericsson U300 RIL
**
** Copyright (C) ST-Ericsson AB 2008-2009
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
** Heavily modified for ST-Ericsson U300 modems.
** Author: Christian Bejram <christian.bejram@stericsson.com>
*/

#include <stdio.h>
#include <telephony/ril.h>
#include <assert.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include "u300-ril.h"
#include "u300-ril-error.h"
#include "u300-ril-messaging.h"
#include "u300-ril-network.h"
#include "u300-ril-sim.h"
#include "u300-ril-pdp.h"
#include "u300-ril-device.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#define REPOLL_OPERATOR_SELECTED 360     /* 6 minutes OK? */
#define MAX_NITZ_LENGTH 32

static const struct timespec TIMEVAL_OPERATOR_SELECT_POLL = { 1, 0 };

static char last_nitz_time[MAX_NITZ_LENGTH];

static int s_creg_stat = 4, s_creg_lac = -1, s_creg_cid = -1;
static int s_cgreg_stat = 4, s_cgreg_lac = -1, s_cgreg_cid = -1, s_cgreg_act = -1;

static int s_gsm_rinfo = 0, s_umts_rinfo = 0;
static int s_reg_change = 0;
static int s_cops_mode = -1;

static void pollOperatorSelected(void *params);

/*
 * s_registrationDeniedReason is used to keep track of registration deny
 * reason for which is called by pollOperatorSelected from
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, so that in case
 * of invalid SIM/ME, Android will not continuously poll for operator.
 *
 * s_registrationDeniedReason is set when receives the registration deny
 * and detail reason from "AT*E2REG?" command, and is reset to
 * DEFAULT_VALUE otherwise.
 */
static CsReg_Deny_DetailReason s_registrationDeniedReason = DEFAULT_VALUE;

/*
 * variable and defines to keep track of preferred network type
 * the PREF_NET_TYPE defines correspond to CFUN arguments for
 * different radio states
 */
#define PREF_NET_TYPE_3G 1
#define PREF_NET_TYPE_2G_ONLY 5
#define PREF_NET_TYPE_3G_ONLY 6

static int pref_net_type = PREF_NET_TYPE_3G;

struct operatorPollParams {
    RIL_Token t;
    int loopcount;
};

/* +CGREG AcT values */
enum CREG_AcT {
    CGREG_ACT_GSM               = 0,
    CGREG_ACT_GSM_COMPACT       = 1, /* Not Supported */
    CGREG_ACT_UTRAN             = 2,
    CGREG_ACT_GSM_EGPRS         = 3,
    CGREG_ACT_UTRAN_HSDPA       = 4,
    CGREG_ACT_UTRAN_HSUPA       = 5,
    CGREG_ACT_UTRAN_HSUPA_HSDPA = 6,
    CGREG_ACT_UTRAN_HSPAP       = 7  /* Dummy Value for HSPA Evol */
};

/* +CGREG stat values */
enum CREG_stat {
    CGREG_STAT_NOT_REG            = 0,
    CGREG_STAT_REG_HOME_NET       = 1,
    CGREG_STAT_NOT_REG_SEARCHING  = 2,
    CGREG_STAT_REG_DENIED         = 3,
    CGREG_STAT_UNKNOWN            = 4,
    CGREG_STAT_ROAMING            = 5
};

/* *ERINFO umts_info values */
enum ERINFO_umts {
    ERINFO_UMTS_NO_UMTS_HSDPA     = 0,
    ERINFO_UMTS_UMTS              = 1,
    ERINFO_UMTS_HSDPA             = 2,
    ERINFO_UMTS_HSPA_EVOL         = 3
};

#define E2REG_UNKNOWN                   -1
#define E2REG_DETACHED                  0
#define E2REG_IN_PROGRESS               1
#define E2REG_ACCESS_CLASS_BARRED       2   /* BARRED */
#define E2REG_NO_RESPONSE               3
#define E2REG_PENDING                   4
#define E2REG_REGISTERED                5
#define E2REG_PS_ONLY_SUSPENDED         6
#define E2REG_NO_ALLOWABLE_PLMN         7   /* Forbidden NET */
#define E2REG_PLMN_NOT_ALLOWED          8   /* Forbidden NET */
#define E2REG_LA_NOT_ALLOWED            9   /* Forbidden NET */
#define E2REG_ROAMING_NOT_ALLOWED       10  /* Forbidden NET */
#define E2REG_PS_ONLY_GPRS_NOT_ALLOWED  11  /* Forbidden NET */
#define E2REG_NO_SUITABLE_CELLS         12  /* Forbidden NET */
#define E2REG_INVALID_SIM_AUTH          13  /* Invalid SIM */
#define E2REG_INVALID_SIM_CONTENT       14  /* Invalid SIM */
#define E2REG_INVALID_SIM_LOCKED        15  /* Invalid SIM */
#define E2REG_INVALID_SIM_IMSI          16  /* Invalid SIM */
#define E2REG_INVALID_SIM_ILLEGAL_MS    17  /* Invalid SIM */
#define E2REG_INVALID_SIM_ILLEGAL_ME    18  /* Invalid SIM */
#define E2REG_PS_ONLY_INVALID_SIM_GPRS  19  /* Invalid SIM */
#define E2REG_INVALID_SIM_NO_GPRS       20  /* Invalid SIM */

static int s_cs_status = E2REG_UNKNOWN;
static int s_ps_status = E2REG_UNKNOWN;

/**
 * Poll +COPS? and return a success, or if the loop counter reaches
 * REPOLL_OPERATOR_SELECTED, return generic failure.
 */
static void pollOperatorSelected(void *params)
{
    int err = 0;
    int response = 0;
    char *line = NULL;
    ATResponse *atresponse = NULL;
    struct operatorPollParams *poll_params;
    RIL_Token t;

    assert(params != NULL);

    poll_params = (struct operatorPollParams *) params;
    t = poll_params->t;

    if (poll_params->loopcount >= REPOLL_OPERATOR_SELECTED)
        goto error;

    /* Only poll COPS? if we are in static state, to prevent waking a
       suspended device (and during boot while module not beeing
       registered to network yet).
    */
    if (((s_cs_status == E2REG_UNKNOWN) || (s_cs_status == E2REG_IN_PROGRESS) ||
        (s_cs_status == E2REG_PENDING)) && ((s_ps_status == E2REG_UNKNOWN) ||
        ((s_ps_status == E2REG_IN_PROGRESS) || (s_ps_status == E2REG_PENDING)))) {
        poll_params->loopcount++;
        enqueueRILEventName(RIL_EVENT_QUEUE_PRIO, pollOperatorSelected,
                        poll_params, &TIMEVAL_OPERATOR_SELECT_POLL, NULL);
        return;
    }

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);
    if (err < 0)
        goto error;

    /* If we don't get more than the COPS: {0-4} we are not registered.
       Loop and try again. */
    if (!at_tok_hasmore(&line)) {
        switch (s_registrationDeniedReason) {
        case IMSI_UNKNOWN_IN_HLR: /* fall through */
        case ILLEGAL_ME:
            RIL_onRequestComplete(t, RIL_E_ILLEGAL_SIM_OR_ME, NULL, 0);
            free(poll_params);
            break;
        default:
            poll_params->loopcount++;
            enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollOperatorSelected,
                            poll_params, &TIMEVAL_OPERATOR_SELECT_POLL);
        }
    } else {
        /* We got operator, throw a success! */
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        free(poll_params);
    }

    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    free(poll_params);
    at_response_free(atresponse);
    return;
}

/**
 * Convert UCS2 hex coded string to UTF-8 coded string.
 *
 * E.g. "004100420043" -> "ABC"
 *
 * Note: Not valid for values corresponding to > 0x7f
 */
static char *convertUcs2ToUtf8(const char *ucs)
{
    int len, cnt;
    int idx, j;
    char *utf8Str;

    if (!ucs)
        return NULL;
    else
        len = strlen(ucs) / 4;

    if (!(utf8Str = malloc(len + 1)))
        return NULL;
    for (idx = 0, j = 0; idx < len; idx++) {
        char temp[5];
        int res;
        strncpy(temp, &ucs[j], 4);
        temp[4] = '\0';
        cnt = sscanf(temp, "%x", &res);
        if (cnt == 0 || cnt == EOF) {
            free(utf8Str);
            return NULL;
        }
        sprintf(&utf8Str[idx], "%c", res);
        j += 4;
    }
    utf8Str[idx] = '\0';
    return utf8Str;
}

/**
 * Converts an AT response string including UCS-2 formatted strings to a
 * corresponding AT response with the strings in UTF-8 format.
 *
 * Typical usage is when receiving an unsolicited response while UCS-2
 * format is temporarily used.
  */
static char* convertResponseToUtf8(const char *mbmargs){
    const char *forward, *back;
    char *output = NULL;
    char *str, *utf8;
    if(!(output = malloc(strlen(mbmargs)))) {
        ALOGE("%s() Failed to allocate memory", __func__);
        return NULL;
    }
    output[0] = '\0';
    forward = back = mbmargs;

    for (;;) {
        /* take anything before the " and put it into output and move back and forward inside the string*/
        if (!(forward = strstr(forward, "\"")))
            break;
        if (!(str = strndup(back, forward-back))) {
            ALOGE("%s() Failed to allocate memory", __func__);
            free(output);
            return NULL;
        }
        sprintf(output, "%s%s", output, str);
        free(str);
        forward++;
        back = forward;

        /* take everything inside the ucs2 string (without the "") and convert it and put the utf8 in output */
        if (!(forward = strstr(forward, "\""))) {
            free(output);
            ALOGE("%s() Bad ucs2 message, couldn't parse it:%s", __func__, mbmargs);
            return NULL;
        }
        /* The case when we have "" */
        if (back == forward){
            sprintf(output, "%s\"\"", output);
            forward++;
            back = forward;
            continue;
        }
        if (!(str = strndup(back, forward-back))) {
            free(output);
            ALOGE("%s() Failed to allocate memory", __func__);
            return NULL;
        }
        if (!(utf8 = convertUcs2ToUtf8(str))) {
            free(str);
            ALOGE("%s() Failed to allocate memory", __func__);
            free(output);
            return NULL;
        }
        sprintf(output, "%s\"%s\"", output, utf8);
        free(str);
        free(utf8);
        forward++;
        back = forward;
    }
    output = realloc(output, strlen(output) + 1);
    return output;
}

/**
 * RIL_UNSOL_NITZ_TIME_RECEIVED
 *
 * Called when radio has received a NITZ time message.
 *
 * "data" is const char * pointing to NITZ time string
 *
 */
void onNetworkTimeReceived(const char *s)
{
    /* Special handling of DST for Android framework
       Module does not include DST correction in NITZ,
       but Android expects it */

    char *line, *tok, *response, *time, *timestamp, *ucs = NULL;
    int tz, dst;

    if (!strstr(s,"/")) {
        ALOGI("%s() Bad format, converting string from ucs2: %s", __func__, s);
        ucs = convertResponseToUtf8(s);
        if (NULL == ucs) {
            ALOGE("%s() Failed converting string from ucs2", __func__);
            return;
        }
        s = (const char *)ucs;
    }

    tok = line = strdup(s);
    if (NULL == tok) {
        ALOGE("%s() Failed to allocate memory", __func__);
        free(ucs);
        return;
    }

    at_tok_start(&tok);

    ALOGD("%s() Got nitz: %s", __func__, s);
    if (at_tok_nextint(&tok, &tz) != 0)
        ALOGE("%s() Failed to parse NITZ tz %s", __func__, s);
    else if (at_tok_nextstr(&tok, &time) != 0)
        ALOGE("%s() Failed to parse NITZ time %s", __func__, s);
    else if (at_tok_nextstr(&tok, &timestamp) != 0)
        ALOGE("%s() Failed to parse NITZ timestamp %s", __func__, s);
    else {
        if (at_tok_nextint(&tok, &dst) != 0) {
            dst = 0;
            ALOGE("%s() Failed to parse NITZ dst, fallbacking to dst=0 %s",
             __func__, s);
        }
        if (!(asprintf(&response, "%s%+03d,%02d", time + 2, tz + (dst * 4), dst))) {
            free(line);
            ALOGE("%s() Failed to allocate string", __func__);
            free(ucs);
            return;
        }

        if (strncmp(response, last_nitz_time, strlen(response)) != 0) {
            RIL_onUnsolicitedResponse(RIL_UNSOL_NITZ_TIME_RECEIVED,
                                      response, sizeof(char *));
            /* If we're in screen state off, we have disabled CREG, but the ETZV
               will catch those few cases. So we send network state changed as
               well on NITZ. */
            if (!getScreenState())
                RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                          NULL, 0);
            strncpy(last_nitz_time, response, strlen(response));
            enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, sendTime,
                            NULL, NULL);
        } else
            ALOGD("%s() Discarding NITZ since it hasn't changed since last update",
             __func__);

        free(response);
    }

    free(ucs);
    free(line);
}

int getSignalStrength(RIL_SignalStrength_v6 *signalStrength){
    ATResponse *atresponse = NULL;
    int err;
    char *line;
    int ber;
    int rssi;

    memset(signalStrength, 0, sizeof(RIL_SignalStrength_v6));

    signalStrength->LTE_SignalStrength.signalStrength = -1;
    signalStrength->LTE_SignalStrength.rsrp = -1;
    signalStrength->LTE_SignalStrength.rsrq = -1;
    signalStrength->LTE_SignalStrength.rssnr = -1;
    signalStrength->LTE_SignalStrength.cqi = -1;

    err = at_send_command_singleline("AT+CSQ", "+CSQ:", &atresponse);

    if (err != AT_NOERROR)
        goto cind;
    
    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto cind;

    err = at_tok_nextint(&line,&rssi);
    if (err < 0)
        goto cind;
    signalStrength->GW_SignalStrength.signalStrength = rssi;

    err = at_tok_nextint(&line, &ber);
    if (err < 0)
        goto cind;
    signalStrength->GW_SignalStrength.bitErrorRate = ber;

    at_response_free(atresponse);
    atresponse = NULL;
    /*
     * If we get 99 as signal strength. Try AT+CIND to give
     * some indication on what signal strength we got.
     *
     * Android calculates rssi and dBm values from this value, so the dBm
     * value presented in android will be wrong, but this is an error on
     * android's end.
     */
    if (rssi == 99) {
cind:
        at_response_free(atresponse);
        atresponse = NULL;

        err = at_send_command_singleline("AT+CIND?", "+CIND:", &atresponse);
        if (err != AT_NOERROR)
            goto error;

        line = atresponse->p_intermediates->line;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        /* discard the first value */
        err = at_tok_nextint(&line,
                             &signalStrength->GW_SignalStrength.signalStrength);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line,
                             &signalStrength->GW_SignalStrength.signalStrength);
        if (err < 0)
            goto error;

        signalStrength->GW_SignalStrength.bitErrorRate = 99;

        /* Convert CIND value so Android understands it correctly */
        if (signalStrength->GW_SignalStrength.signalStrength > 0) {
            signalStrength->GW_SignalStrength.signalStrength *= 4;
            signalStrength->GW_SignalStrength.signalStrength--;
        }
    }

    at_response_free(atresponse);
    return 0;

error:
    at_response_free(atresponse);
    return -1;
}

/**
 * RIL_REQUEST_NEIGHBORINGCELL_IDS
 */
void requestNeighboringCellIDs(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;

    if ((s_cs_status != E2REG_REGISTERED) && (s_ps_status != E2REG_REGISTERED)) {
        No_NCIs(t);
        return;
    }
    if (s_gsm_rinfo)        /* GSM (GPRS,2G) */
        Get_GSM_NCIs(t);
    else if (s_umts_rinfo)  /* UTRAN (WCDMA/UMTS, 3G) */
        Get_WCDMA_NCIs(t);
    else
        No_NCIs(t);
}

/**
 * GSM Network (GPRS, 2G) Neighborhood Cell IDs
 */
void Get_GSM_NCIs(RIL_Token t)
{
    int err = 0;
    char *p = NULL;
    int n = 0;
    ATLine *tmp = NULL;
    ATResponse *gnci_resp = NULL;
    RIL_NeighboringCell *ptr_cells[MAX_NUM_NEIGHBOR_CELLS];

    err = at_send_command_multiline("AT*EGNCI", "*EGNCI:", &gnci_resp);
    if (err != AT_NOERROR) {
        No_NCIs(t);
        goto finally;
    }

    tmp = gnci_resp->p_intermediates;
    while (tmp) {
        if (n > MAX_NUM_NEIGHBOR_CELLS)
            goto error;
        p = tmp->line;
        if (*p == '*') {
            char *line = p;
            char *plmn = NULL;
            char *lac = NULL;
            char *cid = NULL;
            int arfcn = 0;
            int bsic = 0;
            int rxlvl = 0;
            int ilac = 0;
            int icid = 0;

            err = at_tok_start(&line);
            if (err < 0) goto error;
            /* PLMN */
            err = at_tok_nextstr(&line, &plmn);
            if (err < 0) goto error;
            /* LAC */
            err = at_tok_nextstr(&line, &lac);
            if (err < 0) goto error;
            /* CellID */
            err = at_tok_nextstr(&line, &cid);
            if (err < 0) goto error;
            /* ARFCN */
            err = at_tok_nextint(&line, &arfcn);
            if (err < 0) goto error;
            /* BSIC */
            err = at_tok_nextint(&line, &bsic);
            if (err < 0) goto error;
            /* RxLevel */
            err = at_tok_nextint(&line, &rxlvl);
            if (err < 0) goto error;

            /* process data for each cell */
            ptr_cells[n] = alloca(sizeof(RIL_NeighboringCell));
            ptr_cells[n]->rssi = rxlvl;
            ptr_cells[n]->cid = alloca(9 * sizeof(char));
            sscanf(lac,"%x",&ilac);
            sscanf(cid,"%x",&icid);
            sprintf(ptr_cells[n]->cid, "%08x", ((ilac << 16) + icid));
            n++;
        }
        tmp = tmp->p_next;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, ptr_cells,
                          n * sizeof(RIL_NeighboringCell *));

finally:
    at_response_free(gnci_resp);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * WCDMA Network (UTMS, 3G) Neighborhood Cell IDs
 */
void Get_WCDMA_NCIs(RIL_Token t)
{
    int err = 0;
    char *p = NULL;
    int n = 0;
    ATLine *tmp = NULL;
    ATResponse *wnci_resp = NULL;
    RIL_NeighboringCell *ptr_cells[MAX_NUM_NEIGHBOR_CELLS];

    err = at_send_command_multiline("AT*EWNCI", "*EWNCI:", &wnci_resp);
    if (err != AT_NOERROR) {
        No_NCIs(t);
        goto finally;
    }

    tmp = wnci_resp->p_intermediates;
    while (tmp) {
        if (n > MAX_NUM_NEIGHBOR_CELLS)
            goto error;
        p = tmp->line;
        if (*p == '*') {
            char *line = p;
            int uarfcn = 0;
            int psc = 0;
            int rscp = 0;
            int ecno = 0;
            int pathloss = 0;

            err = at_tok_start(&line);
            if (err < 0) goto error;
            /* UARFCN */
            err = at_tok_nextint(&line, &uarfcn);
            if (err < 0) goto error;
            /* PSC */
            err = at_tok_nextint(&line, &psc);
            if (err < 0) goto error;
            /* RSCP */
            err = at_tok_nextint(&line, &rscp);
            if (err < 0) goto error;
            /* ECNO */
            err = at_tok_nextint(&line, &ecno);
            if (err < 0) goto error;
            /* PathLoss */
            err = at_tok_nextint(&line, &pathloss);
            if (err < 0) goto error;

            /* process data for each cell */
            ptr_cells[n] = alloca(sizeof(RIL_NeighboringCell));
            ptr_cells[n]->rssi = rscp;
            ptr_cells[n]->cid = alloca(9 * sizeof(char));
            sprintf(ptr_cells[n]->cid, "%08x", psc);
            n++;
        }
        tmp = tmp->p_next;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, ptr_cells,
                          n * sizeof(RIL_NeighboringCell *));

finally:
    at_response_free(wnci_resp);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * Not registered or unknown network (NOT UTMS or 3G)
 * return UNKNOWN_RSSI and UNKNOWN_CID 
 */
void No_NCIs(RIL_Token t)
{
    int n = 0;

    RIL_NeighboringCell *ptr_cells[MAX_NUM_NEIGHBOR_CELLS];

    ptr_cells[n] = alloca(sizeof(RIL_NeighboringCell));
    ptr_cells[n]->rssi = 99;
    ptr_cells[n]->cid = alloca(9 * sizeof(char));
    sprintf(ptr_cells[n]->cid, "%08x", -1);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, ptr_cells,
                          n * sizeof(RIL_NeighboringCell *));

    return;
}

/**
 * RIL_UNSOL_SIGNAL_STRENGTH
 *
 * Radio may report signal strength rather than have it polled.
 *
 * "data" is a const RIL_SignalStrength *
 */
void pollSignalStrength(void *arg)
{
    RIL_SignalStrength_v6 signalStrength;
    (void) arg;

    if (getSignalStrength(&signalStrength) < 0)
        ALOGE("%s() Polling the signal strength failed", __func__);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_SIGNAL_STRENGTH,
                                  &signalStrength, sizeof(RIL_SignalStrength_v6));
}

void onSignalStrengthChanged(const char *s)
{
    (void) s;
    enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollSignalStrength, NULL, NULL);
}

void onRegistrationStatusChanged(const char *s)
{
    char *line = NULL, *ptr = NULL;
    int *stat_ptr, *lac_ptr, *cid_ptr, *act_ptr = NULL;
    int commas = 0, update = 0;
    int skip, tmp, err;
    int creg_stat = 4, creg_lac = -1, creg_cid = -1, creg_act = -1;
    int cgreg_stat = 4, cgreg_lac = -1, cgreg_cid = -1, cgreg_act = -1;

    ptr = line = strdup(s);
    if (line == NULL) {
        ALOGE("%s() Failed to allocate memory", __func__);
        return;
    }

    at_tok_start(&line);

    if (strStartsWith(s, "+CREG:")) {
        stat_ptr = &creg_stat;
        lac_ptr = &creg_lac;
        cid_ptr = &creg_cid;
        act_ptr = &creg_act;
    } else {
        stat_ptr = &cgreg_stat;
        lac_ptr = &cgreg_lac;
        cid_ptr = &cgreg_cid;
        act_ptr = &cgreg_act;
    }

    /* Count number of commas */
    err = at_tok_charcounter(line, ',', &commas);
    if (err < 0) {
        ALOGE("%s() at_tok_charcounter failed", __func__);
        goto error;
    }

    switch (commas) {
    case 0:                    /* +xxREG: <stat> */
        err = at_tok_nextint(&line, stat_ptr);
        if (err < 0) goto error;
        break;

    case 1:                    /* +xxREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, stat_ptr);
        if (err < 0) goto error;
        break;

    case 2:                    /* +xxREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, stat_ptr);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, lac_ptr);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, cid_ptr);
        if (err < 0) goto error;
        break;

    case 3:                    /* +xxREG: <n>, <stat>, <lac>, <cid> */
                               /* +xxREG: <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &tmp);
        if (err < 0) goto error;

        /* We need to check if the second parameter is <lac> */
        if (*(line) == '"') {
            *stat_ptr = tmp; /* <stat> */
            err = at_tok_nexthexint(&line, lac_ptr); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, cid_ptr); /* <cid> */
            if (err < 0) goto error;
            err = at_tok_nextint(&line, act_ptr); /* <AcT> */
        } else {
            err = at_tok_nextint(&line, stat_ptr); /* <stat> */
           if (err < 0) goto error;
            err = at_tok_nexthexint(&line, lac_ptr); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, cid_ptr); /* <cid> */
            if (err < 0) goto error;
        }
        break;

    case 4:                    /* +xxREG: <n>, <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &skip); /* <n> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, stat_ptr); /* <stat> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, lac_ptr); /* <lac> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, cid_ptr); /* <cid> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, act_ptr); /* <AcT> */
        break;

    default:
        ALOGE("%s() Invalid input", __func__);
        goto error;
    }


    /* Reduce the amount of unsolicited sent to the framework
       LAC and CID will be the same in both domains */
    if (strStartsWith(s, "+CREG:")) {
        if (s_creg_stat != creg_stat) {
            update = 1;
            s_creg_stat = creg_stat;
        }
        if (s_creg_lac != creg_lac) {
            if (s_cgreg_lac != creg_lac)
                update = 1;
            s_creg_lac = creg_lac;
        }
        if (s_creg_cid != creg_cid) {
            if (s_cgreg_cid != creg_cid)
                update = 1;
            s_creg_cid = creg_cid;
        }
    } else {
        if (s_cgreg_stat != cgreg_stat) {
            update = 1;
            s_cgreg_stat = cgreg_stat;
        }
        if (s_cgreg_lac != cgreg_lac) {
            if (s_creg_lac != cgreg_lac)
                update = 1;
            s_cgreg_lac = cgreg_lac;
        }
        if (s_cgreg_cid != cgreg_cid) {
            if (s_creg_cid != cgreg_cid)
                update = 1;
            s_cgreg_cid = cgreg_cid;
        }
        if (s_cgreg_act != cgreg_act) {
            update = 1;
            s_cgreg_act = cgreg_act;
        }
    }

    if (update) {
        s_reg_change = 1;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                  NULL, 0);
    } else
        ALOGW("%s() Skipping unsolicited response since no change in state", __func__);

finally:
    free(ptr);
    return;

error:
    ALOGE("%s() Unable to parse (%s)", __func__, s);
    goto finally;
}

void onNetworkCapabilityChanged(const char *s)
{
    int err;
    int skip;
    char *line = NULL, *tok = NULL;
    static int old_gsm_rinfo = -1, old_umts_rinfo = -1;

    s_gsm_rinfo = s_umts_rinfo = 0;

    tok = line = strdup(s);
    if (tok == NULL)
        goto error;

    at_tok_start(&tok);

    err = at_tok_nextint(&tok, &skip);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &s_gsm_rinfo);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &s_umts_rinfo);
    if (err < 0)
        goto error;

    if ((old_gsm_rinfo != s_gsm_rinfo) || (old_umts_rinfo != s_umts_rinfo)) {
        old_gsm_rinfo = s_gsm_rinfo;
        old_umts_rinfo = s_umts_rinfo;
        /* No need to update when screen is off */
        if (getScreenState())
            RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                      NULL, 0);
    } else
        ALOGW("%s() Skipping unsolicited response since no change in state", __func__);

error:
    free(line);
}

void onNetworkStatusChanged(const char *s)
{
    int err;
    int skip;
    int resp;
    char *line = NULL, *tok = NULL;
    static int old_resp = -1;

    s_cs_status = s_ps_status = E2REG_UNKNOWN;
    tok = line = strdup(s);
    if (tok == NULL)
        goto error;

    at_tok_start(&tok);

    err = at_tok_nextint(&tok, &skip);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &s_cs_status);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&tok, &s_ps_status);
    if (err < 0)
        goto error;

    resp = RIL_RESTRICTED_STATE_NONE;

    switch (s_cs_status) {
        case E2REG_ACCESS_CLASS_BARRED:
        case E2REG_NO_ALLOWABLE_PLMN:
        case E2REG_PLMN_NOT_ALLOWED:
        case E2REG_LA_NOT_ALLOWED:
        case E2REG_ROAMING_NOT_ALLOWED:
        case E2REG_NO_SUITABLE_CELLS:
        case E2REG_INVALID_SIM_AUTH:
        case E2REG_INVALID_SIM_CONTENT:
        case E2REG_INVALID_SIM_LOCKED:
        case E2REG_INVALID_SIM_IMSI:
        case E2REG_INVALID_SIM_ILLEGAL_MS:
        case E2REG_INVALID_SIM_ILLEGAL_ME:
        case E2REG_INVALID_SIM_NO_GPRS:
            resp |= RIL_RESTRICTED_STATE_CS_ALL;
            break;
        default:
            break;
    }

    switch (s_ps_status) {
        case E2REG_ACCESS_CLASS_BARRED:
        case E2REG_NO_ALLOWABLE_PLMN:
        case E2REG_PLMN_NOT_ALLOWED:
        case E2REG_LA_NOT_ALLOWED:
        case E2REG_ROAMING_NOT_ALLOWED:
        case E2REG_PS_ONLY_GPRS_NOT_ALLOWED:
        case E2REG_NO_SUITABLE_CELLS:
        case E2REG_INVALID_SIM_AUTH:
        case E2REG_INVALID_SIM_CONTENT:
        case E2REG_INVALID_SIM_LOCKED:
        case E2REG_INVALID_SIM_IMSI:
        case E2REG_INVALID_SIM_ILLEGAL_MS:
        case E2REG_INVALID_SIM_ILLEGAL_ME:
        case E2REG_PS_ONLY_INVALID_SIM_GPRS:
        case E2REG_INVALID_SIM_NO_GPRS:
            resp |= RIL_RESTRICTED_STATE_PS_ALL;
            break;
        default:
            break;
    }

    if (old_resp != resp) {
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESTRICTED_STATE_CHANGED,
                                  &resp, sizeof(int *));
        old_resp = resp;
    } else
        ALOGW("%s() Skipping unsolicited response since no change in state", __func__);

    /* If registered, poll signal strength for faster update of signal bar */
    if ((s_cs_status == E2REG_REGISTERED) || (s_ps_status == E2REG_REGISTERED)) {
        enqueueRILEvent(RIL_EVENT_QUEUE_PRIO, pollSignalStrength, (void *)-1, NULL);
        /* Make sure registration state is updated when screen is off */
        if (!getScreenState())
            RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
                                      NULL, 0);
    }

error:
    free(line);
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
 *
 * Specify that the network should be selected automatically.
*/
void requestSetNetworkSelectionAutomatic(void *data, size_t datalen,
                                         RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    ATResponse *atresponse = NULL;
    int mode = 0;
    int skip;
    char *line;
    char *operator = NULL;
    struct operatorPollParams *poll_params = NULL;

    poll_params = malloc(sizeof(struct operatorPollParams));
    if (NULL == poll_params)
        goto error;

    /* First check if we are already scanning or in manual mode */
    err = at_send_command_singleline("AT+COPS=3,2;+COPS?", "+COPS:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read network selection mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    s_cops_mode = mode;

    /* If we're unregistered, we may just get
       a "+COPS: 0" response. */
    if (!at_tok_hasmore(&line)) {
        if (mode == 1) {
            ALOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else
            goto check_reg;
    }

    err = at_tok_nextint(&line, &skip);
    if (err < 0)
        goto error;

    /* A "+COPS: 0, n" response is also possible. */
    if (!at_tok_hasmore(&line)) {
        if (mode == 1) {
            ALOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else
            goto check_reg;
    }

    /* Read numeric operator */
    err = at_tok_nextstr(&line, &operator);
    if (err < 0)
        goto error;

    /* If operator is found then do a new scan,
       else let it continue the already pending scan */
    if (operator && strlen(operator) == 0) {
        if (mode == 1) {
            ALOGD("%s() Changing manual to automatic network mode", __func__);
            goto do_auto;
        } else
            goto check_reg;
    }

    /* Operator found */
    if (mode == 1) {
        ALOGD("%s() Changing manual to automatic network mode", __func__);
        goto do_auto;
    } else {
        ALOGD("%s() Already in automatic mode with known operator, trigger a new network scan",
	    __func__);
        goto do_auto;
    }

    /* Check if module is scanning,
       if not then trigger a rescan */
check_reg:
    at_response_free(atresponse);
    atresponse = NULL;

    /* Check CS domain first */
    err = at_send_command_singleline("AT+CREG?", "+CREG:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read registration unsolicited mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* Read registration status */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    s_creg_stat = mode;

    /* If scanning has stopped, then perform a new scan */
    if (mode == 0) {
        ALOGD("%s() Already in automatic mode, but not currently scanning on CS,"
	     "trigger a new network scan", __func__);
        goto do_auto;
    }

    /* Now check PS domain */
    at_response_free(atresponse);
    atresponse = NULL;
    err = at_send_command_singleline("AT+CGREG?", "+CGREG:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read registration unsolicited mode */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    /* Read registration status */
    err = at_tok_nextint(&line, &mode);
    if (err < 0)
        goto error;

    s_cgreg_stat = mode;

    /* If scanning has stopped, then perform a new scan */
    if (mode == 0) {
        ALOGD("%s() Already in automatic mode, but not currently scanning on PS,"
	     "trigger a new network scan", __func__);
        goto do_auto;
    }
    else
    {
        ALOGD("%s() Already in automatic mode and scanning", __func__);
        goto finish_scan;
    }

do_auto:
    at_response_free(atresponse);
    atresponse = NULL;

    /* This command does two things, one it sets automatic mode,
       two it starts a new network scan! */
    err = at_send_command("AT+COPS=0");
    if (err != AT_NOERROR)
        goto error;

    s_cops_mode = 0;

finish_scan:

    at_response_free(atresponse);
    atresponse = NULL;

    poll_params->loopcount = 0;
    poll_params->t = t;

    enqueueRILEvent(RIL_EVENT_QUEUE_NORMAL, pollOperatorSelected,
                    poll_params, &TIMEVAL_OPERATOR_SELECT_POLL);

    return;

error:
    free(poll_params);
    at_response_free(atresponse);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    return;
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
 *
 * Manually select a specified network.
 *
 * The radio baseband/RIL implementation is expected to fall back to
 * automatic selection mode if the manually selected network should go
 * out of range in the future.
 */
void requestSetNetworkSelectionManual(void *data, size_t datalen,
                                      RIL_Token t)
{
    /*
     * AT+COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]]
     *    <mode>   = 4 = Manual (<oper> field shall be present and AcT optionally) with fallback to automatic if manual fails.
     *    <format> = 2 = Numeric <oper>, the number has structure:
     *                   (country code digit 3)(country code digit 2)(country code digit 1)
     *                   (network code digit 2)(network code digit 1)
     */

    (void) datalen;
    int err = 0;
    const char *mccMnc = (const char *) data;

    /* Check inparameter. */
    if (mccMnc == NULL)
        goto error;

    /* Increase the AT command timeout for this operation */
    at_set_timeout_msec(1000 * 60 * 6);

    /* Build and send command. */
    err = at_send_command("AT+COPS=1,2,\"%s\"", mccMnc);

    /* Restore default AT command timeout */
    at_set_timeout_msec(1000 * 30);

    if (err != AT_NOERROR)
        goto error;

    s_cops_mode = 1;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
 *
 * Scans for available networks.
*/
void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t)
{
    #define QUERY_NW_NUM_PARAMS 4

    /*
     * AT+COPS=?
     *   +COPS: [list of supported (<stat>,long alphanumeric <oper>
     *           ,short alphanumeric <oper>,numeric <oper>[,<AcT>])s]
     *          [,,(list of supported <mode>s),(list of supported <format>s)]
     *
     *   <stat>
     *     0 = unknown
     *     1 = available
     *     2 = current
     *     3 = forbidden
     */
    (void) data; (void) datalen;
    int err = 0;
    ATResponse *atresponse = NULL;
    ATResponse *cops_response = NULL;
    const char *statusTable[] =
        { "unknown", "available", "current", "forbidden" };
    char **responseArray = NULL;
    char *p;
    char *line = NULL;
    char *current = NULL;
    int current_act = -1;
    int skip;
    int n = 0;
    int i = 0;

    /* Increase the AT command timeout for this operation */
    at_set_timeout_msec(1000 * 60 * 6);

    /* Get available operators */
    err = at_send_command_multiline("AT+COPS=?", "+COPS:", &atresponse);

    /* Restore default AT command timeout */
    at_set_timeout_msec(1000 * 30);

    if (err != AT_NOERROR)
        goto error;

    p = atresponse->p_intermediates->line;

    /* count number of '('. */
    err = at_tok_charcounter(p, '(', &n);
    if (err < 0) goto error;

    /* Allocate array of strings, blocks of 4 strings. */
    responseArray = alloca(n * QUERY_NW_NUM_PARAMS * sizeof(char *));

    /* Get current operator and technology */
    err = at_send_command_singleline("AT+COPS=3,2;+COPS?", "+COPS:", &cops_response);
    if (err != AT_NOERROR)
        goto error;

    line = cops_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /* Read and skip network selection mode */
    err = at_tok_nextint(&line, &skip);
    if (err < 0)
        goto no_current;

    /* Read and skip format */
    err = at_tok_nextint(&line, &skip);
    if (err < 0)
        goto no_current;

    /* Read current numeric operator */
    err = at_tok_nextstr(&line, &current);
    if (err < 0)
        goto no_current;

    /* Read act (Technology) */
    err = at_tok_nextint(&line, &current_act);

no_current:

    /* Loop and collect response information into the response array. */
    for (i = 0; i < n; i++) {
        int status = 0;
        line = NULL;
        char *s = NULL;
        char *longAlphaNumeric = NULL;
        char *shortAlphaNumeric = NULL;
        char *numeric = NULL;
        char *remaining = NULL;
        int act = -1;

        s = line = getFirstElementValue(p, "(", ")", &remaining);
        p = remaining;

        if (line == NULL) {
            ALOGE("%s() Null pointer while parsing COPS response."
	         "This should not happen.", __func__);
            break;
        }
        /* <stat> */
        err = at_tok_nextint(&line, &status);
        if (err < 0)
            goto error;

        /* Set home network as available network */
        if (status == 2)
            status = 1;

        /* long alphanumeric <oper> */
        err = at_tok_nextstr(&line, &longAlphaNumeric);
        if (err < 0)
            goto error;

        /* short alphanumeric <oper> */
        err = at_tok_nextstr(&line, &shortAlphaNumeric);
        if (err < 0)
            goto error;

        /* numeric <oper> */
        err = at_tok_nextstr(&line, &numeric);
        if (err < 0)
            goto error;

        /* Read act (Technology) */
        err = at_tok_nextint(&line, &act);
        if (err < 0)
            goto error;

        /* Find match for current operator in list */
        if ((strcmp(numeric, current) == 0) && (act == current_act))
            status = 2;

        responseArray[i * QUERY_NW_NUM_PARAMS + 0] = alloca(strlen(longAlphaNumeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 0], longAlphaNumeric);

        responseArray[i * QUERY_NW_NUM_PARAMS + 1] = alloca(strlen(shortAlphaNumeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 1], shortAlphaNumeric);

        responseArray[i * QUERY_NW_NUM_PARAMS + 2] = alloca(strlen(numeric) + 1);
        strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 2], numeric);

        free(s);

        /*
         * Check if modem returned an empty string, and fill it with MNC/MMC
         * if that's the case.
         */
        if (responseArray[i * QUERY_NW_NUM_PARAMS + 0] && strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 0]) == 0) {
            responseArray[i * QUERY_NW_NUM_PARAMS + 0] = alloca(strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 2]) + 1);
            strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 0], responseArray[i * QUERY_NW_NUM_PARAMS + 2]);
        }

        if (responseArray[i * QUERY_NW_NUM_PARAMS + 1] && strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 1]) == 0) {
            responseArray[i * QUERY_NW_NUM_PARAMS + 1] = alloca(strlen(responseArray[i * QUERY_NW_NUM_PARAMS + 2]) + 1);
            strcpy(responseArray[i * QUERY_NW_NUM_PARAMS + 1], responseArray[i * QUERY_NW_NUM_PARAMS + 2]);
        }

       /* Add status */
        responseArray[i * QUERY_NW_NUM_PARAMS + 3] = alloca(strlen(statusTable[status]) + 1);
        sprintf(responseArray[i * QUERY_NW_NUM_PARAMS + 3], "%s", statusTable[status]);
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseArray,
                          i * QUERY_NW_NUM_PARAMS * sizeof(char *));

finally:
    at_response_free(cops_response);
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/*
 * get the preferred network type as set by Android
 */
int getPreferredNetworkType(void)
{
    return pref_net_type;
}

/**
 * RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
 *
 * Requests to set the preferred network type for searching and registering
 * (CS/PS domain, RAT, and operation mode).
 */
void requestSetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t)
{
    (void) datalen;
    int arg = 0;
    int err = 0;
    int rat;

    RIL_Errno errno = RIL_E_GENERIC_FAILURE;

    rat = ((int *) data)[0];

    switch (rat) {
    case PREF_NET_TYPE_GSM_WCDMA_AUTO:
    case PREF_NET_TYPE_GSM_WCDMA:
        arg = PREF_NET_TYPE_3G;
        ALOGD("[%s] network type = auto", __FUNCTION__);
        break;
    case PREF_NET_TYPE_GSM_ONLY:
        arg = PREF_NET_TYPE_2G_ONLY;
        ALOGD("[%s] network type = 2g only", __FUNCTION__);
        break;
    case PREF_NET_TYPE_WCDMA:
        arg = PREF_NET_TYPE_3G_ONLY;
        ALOGD("[%s] network type = 3g only", __FUNCTION__);
        break;
    default:
        errno = RIL_E_MODE_NOT_SUPPORTED;
        goto error;
    }

    pref_net_type = arg;

    err = at_send_command("AT+CFUN=%d", arg);
    if (err == AT_NOERROR) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        return;
    }

error:
    RIL_onRequestComplete(t, errno, NULL, 0);
}

/**
 * RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
 *
 * Query the preferred network type (CS/PS domain, RAT, and operation mode)
 * for searching and registering.
 */
void requestGetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    int response = 0;
    int cfun;
    char *line;
    ATResponse *atresponse;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &atresponse);
    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &cfun);
    if (err < 0)
        goto error;

    if (cfun < 0 || cfun >= 7)
        goto error;

    switch (cfun) {
    case PREF_NET_TYPE_2G_ONLY:
        response = PREF_NET_TYPE_GSM_ONLY;
        break;
    case PREF_NET_TYPE_3G_ONLY:
        response = PREF_NET_TYPE_WCDMA;
        break;
    case PREF_NET_TYPE_3G:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    default:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
 *
 * Query current network selectin mode.
 */
void requestQueryNetworkSelectionMode(void *data, size_t datalen,
                                      RIL_Token t)
{
    (void) data; (void) datalen;
    int err;
    ATResponse *atresponse = NULL;
    int response = s_cops_mode;
    char *line;

    if (s_cops_mode != -1)
        goto no_sim;

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &atresponse);

    if (at_get_cme_error(err) == CME_SIM_NOT_INSERTED)
        goto no_sim;

    if (err != AT_NOERROR)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &response);

    if (err < 0)
        goto error;

    /*
     * Android accepts 0(automatic) and 1(manual).
     * Modem may return mode 4(Manual/automatic).
     * Convert it to 1(Manual) as android expects.
     */
    if (response == 4)
        response = 1;

no_sim:
    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

finally:
    at_response_free(atresponse);
    return;

error:
    ALOGE("%s() Must never return error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_SIGNAL_STRENGTH
 *
 * Requests current signal strength and bit error rate.
 *
 * Must succeed if radio is on.
 */
void requestSignalStrength(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    RIL_SignalStrength_v6 signalStrength;

    if (getSignalStrength(&signalStrength) < 0) {
        ALOGE("%s() Must never return an error when radio is on", __func__);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    } else
        RIL_onRequestComplete(t, RIL_E_SUCCESS, &signalStrength,
                              sizeof(RIL_SignalStrength_v6));
}

/**
 * Convert CS detailedReason from modem to what Android expects.
 * Called in requestRegistrationState().
 */
static
CsReg_Deny_DetailReason convertCsRegistrationDeniedReason(int detailedReason)
{
    CsReg_Deny_DetailReason reason;

    switch (detailedReason) {
    case E2REG_NO_RESPONSE:
        reason = NETWORK_FAILURE;
        break;
    case E2REG_PLMN_NOT_ALLOWED:
    case E2REG_NO_ALLOWABLE_PLMN:
        reason = PLMN_NOT_ALLOWED;
        break;
    case E2REG_LA_NOT_ALLOWED:
        reason = LOCATION_AREA_NOT_ALLOWED;
        break;
    case E2REG_ROAMING_NOT_ALLOWED:
        reason = ROAMING_NOT_ALLOWED;
        break;
    case E2REG_NO_SUITABLE_CELLS:
        reason = NO_SUITABLE_CELL_IN_LOCATION_AREA;
        break;
    case E2REG_INVALID_SIM_AUTH:
    case E2REG_INVALID_SIM_CONTENT:
    case E2REG_INVALID_SIM_LOCKED:
    case E2REG_INVALID_SIM_NO_GPRS:
        reason = AUTHENTICATION_FAILURE;
        break;
    case E2REG_INVALID_SIM_IMSI:
        reason = IMSI_UNKNOWN_IN_HLR;
        break;
    case E2REG_INVALID_SIM_ILLEGAL_MS:
        reason = ILLEGAL_MS;
        break;
    case E2REG_INVALID_SIM_ILLEGAL_ME:
        reason = ILLEGAL_ME;
        break;
    default:
        reason = GENERAL;
        break;
    }

    return reason;
}

/**
 * Convert PS detailedReason from modem to what Android expects.
 * Called in requestGprsRegistrationState().
 */
static
PsReg_Deny_DetailReason convertPsRegistrationDeniedReason(int detailedReason)
{
    PsReg_Deny_DetailReason reason;

    switch (detailedReason) {
    case E2REG_DETACHED:
        reason = IMPLICITLY_DETACHED;
        break;
    case E2REG_PS_ONLY_GPRS_NOT_ALLOWED:
    case E2REG_NO_SUITABLE_CELLS:
    case E2REG_NO_ALLOWABLE_PLMN:
    case E2REG_PLMN_NOT_ALLOWED:
    case E2REG_LA_NOT_ALLOWED:
    case E2REG_ROAMING_NOT_ALLOWED:
        reason = GPRS_NOT_ALLOWED_PLMN;
        break;
    case E2REG_INVALID_SIM_ILLEGAL_MS:
        reason = MS_IDENTITY_UNKNOWN;
        break;
    case E2REG_PS_ONLY_INVALID_SIM_GPRS:
        reason = GPRS_NOT_ALLOWED;
        break;
    case E2REG_INVALID_SIM_NO_GPRS:
    case E2REG_INVALID_SIM_AUTH:
    case E2REG_INVALID_SIM_CONTENT:
    case E2REG_INVALID_SIM_LOCKED:
    case E2REG_INVALID_SIM_IMSI:
    case E2REG_INVALID_SIM_ILLEGAL_ME:
        reason = GPRS_NON_GPRS_NOT_ALLOWED;
        break;
    default:
        reason = GENERAL;
        break;
    }

    return reason;
}

char *getNetworkType(int def)
{
    int network = def;
    int err, skip;
    static int ul, dl;
    int networkType;
    char *line;
    ATResponse *p_response;
    static int old_umts_rinfo = -1;

    if (s_umts_rinfo > ERINFO_UMTS_NO_UMTS_HSDPA &&
        getE2napState() == E2NAP_STATE_CONNECTED &&
        old_umts_rinfo != s_umts_rinfo) {

        old_umts_rinfo = s_umts_rinfo;
        err = at_send_command_singleline("AT+CGEQNEG=%d", "+CGEQNEG:", &p_response, RIL_CID_IP);

        if (err != AT_NOERROR)
            ALOGE("%s() Allocation for, or sending, CGEQNEG failed."
	         "Using default value specified by calling function", __func__);
        else {
            line = p_response->p_intermediates->line;
            err = at_tok_start(&line);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &skip);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &skip);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &ul);
            if (err < 0)
                goto finally;

            err = at_tok_nextint(&line, &dl);
            if (err < 0)
                goto finally;

            at_response_free(p_response);
            ALOGI("Max speed %i/%i, UL/DL", ul, dl);
        }
    }
    if (s_umts_rinfo > ERINFO_UMTS_NO_UMTS_HSDPA) {
        network = CGREG_ACT_UTRAN;
        if (dl > 384)
            network = CGREG_ACT_UTRAN_HSDPA;
        if (ul > 384) {
            if (s_umts_rinfo == ERINFO_UMTS_HSPA_EVOL)
                network = CGREG_ACT_UTRAN_HSPAP;
            else
                network = CGREG_ACT_UTRAN_HSUPA_HSDPA;
        }
    }
    else if (s_gsm_rinfo) {
        ALOGD("%s() Using 2G info: %d", __func__, s_gsm_rinfo);
        if (s_gsm_rinfo == 1)
            network = CGREG_ACT_GSM;
        else
            network = CGREG_ACT_GSM_EGPRS;
    }

    switch (network) {
    case CGREG_ACT_GSM:
        networkType = RADIO_TECH_GPRS;
        break;
    case CGREG_ACT_UTRAN:
        networkType = RADIO_TECH_UMTS;
        break;
    case CGREG_ACT_GSM_EGPRS:
        networkType = RADIO_TECH_EDGE;
        break;
    case CGREG_ACT_UTRAN_HSDPA:
        networkType = RADIO_TECH_HSDPA;
        break;
    case CGREG_ACT_UTRAN_HSUPA:
        networkType = RADIO_TECH_HSUPA;
        break;
    case CGREG_ACT_UTRAN_HSUPA_HSDPA:
        networkType = RADIO_TECH_HSPA;
        break;
    case CGREG_ACT_UTRAN_HSPAP:
        networkType = RADIO_TECH_HSPAP;
        break;
    default:
        networkType = RADIO_TECH_UNKNOWN;
        break;
    }
    char *resp;
    asprintf(&resp, "%d", networkType);
    return resp;

finally:
    at_response_free(p_response);
    return NULL;
}
/**
 * RIL_REQUEST_DATA_REGISTRATION_STATE
 *
 * Request current GPRS registration state.
 */
void requestGprsRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t)
{
    (void)request, (void)data, (void)datalen;
    int err = 0;
    const char resp_size = 6;
    int response[resp_size];
    char *responseStr[resp_size];
    ATResponse *cgreg_resp = NULL, *e2reg_resp = NULL;
    char *line, *p;
    int commas = 0;
    int skip, tmp;
    int ps_status = 0;
    int count = 3;
    int i;

    memset(responseStr, 0, sizeof(responseStr));
    memset(response, 0, sizeof(response));
    response[1] = -1;
    response[2] = -1;

    /* We only allow polling if screenstate is off, in such case
       CREG and CGREG unsolicited are disabled */
    getScreenStateLock();
    if (!getScreenState())
        (void)at_send_command("AT+CGREG=2"); /* Response not vital */
    else {
        response[0] = s_cgreg_stat;
        response[1] = s_cgreg_lac;
        response[2] = s_cgreg_cid;
        response[3] = s_cgreg_act;
        if (response[0] == CGREG_STAT_REG_DENIED)
            response[4] = convertPsRegistrationDeniedReason(s_ps_status);
        goto cached;
    }


    err = at_send_command_singleline("AT+CGREG?", "+CGREG: ", &cgreg_resp);

    if (at_get_cme_error(err) == CME_SIM_NOT_INSERTED)
        goto no_sim;

    if (err != AT_NOERROR)
        goto error;

    line = cgreg_resp->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;
    /*
     * The solicited version of the +CGREG response is
     * +CGREG: n, stat, [lac, cid [,<AcT>]]
     * and the unsolicited version is
     * +CGREG: stat, [lac, cid [,<AcT>]]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be.
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both.
     *
     * Also since the LAC, CID and AcT are only reported when registered,
     * we can have 1, 2, 3, 4 or 5 arguments here.
     */
    /* Count number of commas */
    p = line;
    err = at_tok_charcounter(line, ',', &commas);
    if (err < 0) {
        ALOGE("%s() at_tok_charcounter failed", __func__);
        goto error;
    }

    switch (commas) {
    case 0:                    /* +CGREG: <stat> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        break;

    case 1:                    /* +CGREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        break;

    case 2:                    /* +CGREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0) goto error;
        break;

    case 3:                    /* +CGREG: <n>, <stat>, <lac>, <cid> */
                               /* +CGREG: <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &tmp);
        if (err < 0) goto error;

        /* We need to check if the second parameter is <lac> */
        if (*(line) == '"') {
            response[0] = tmp; /* <stat> */
            err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[3]); /* <AcT> */
            if (err < 0) goto error;
            count = 4;
        } else {
            err = at_tok_nextint(&line, &response[0]); /* <stat> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
            if (err < 0) goto error;
        }
        break;

    case 4:                    /* +CGREG: <n>, <stat>, <lac>, <cid>, <AcT> */
        err = at_tok_nextint(&line, &skip); /* <n> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[0]); /* <stat> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[1]); /* <lac> */
        if (err < 0) goto error;
        err = at_tok_nexthexint(&line, &response[2]); /* <cid> */
        if (err < 0) goto error;
        err = at_tok_nextint(&line, &response[3]); /* <AcT> */
        if (err < 0) goto error;
        count = 4;
        break;

    default:
        ALOGE("%s() Invalid input", __func__);
        goto error;
    }

    if (response[0] == CGREG_STAT_REG_DENIED) {
        err = at_send_command_singleline("AT*E2REG?", "*E2REG:", &e2reg_resp);

        if (err != AT_NOERROR)
            goto error;

        line = e2reg_resp->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &ps_status);
        if (err < 0)
            goto error;

        response[4] = convertPsRegistrationDeniedReason(ps_status);
    }

    if (s_cgreg_stat != response[0] ||
        s_cgreg_lac != response[1] ||
        s_cgreg_cid != response[2] ||
        s_cgreg_act != response[3]) {

        s_cgreg_stat = response[0];
        s_cgreg_lac = response[1];
        s_cgreg_cid = response[2];
        s_cgreg_act = response[3];
        s_reg_change = 1;
    }

cached:
    if (response[0] == CGREG_STAT_REG_HOME_NET ||
        response[0] == CGREG_STAT_ROAMING)
        responseStr[3] = getNetworkType(response[3]);

no_sim:
    /* Converting to stringlist for Android */
    asprintf(&responseStr[0], "%d", response[0]); /* state */

    if (response[1] >= 0)
        asprintf(&responseStr[1], "%04x", response[1]); /* LAC */
    else
        responseStr[1] = NULL;

    if (response[2] >= 0)
        asprintf(&responseStr[2], "%08x", response[2]); /* CID */
    else
        responseStr[2] = NULL;

    if (response[4] >= 0)
        err = asprintf(&responseStr[4], "%d", response[4]);
    else
        responseStr[4] = NULL;

    asprintf(&responseStr[5], "%d", 1);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, resp_size * sizeof(char *));

finally:
    if (!getScreenState())
        (void)at_send_command("AT+CGREG=0");

    releaseScreenStateLock(); /* Important! */

    for (i = 0; i < resp_size; i++)
        free(responseStr[i]);

    at_response_free(cgreg_resp);
    at_response_free(e2reg_resp);
    return;

error:
    ALOGE("%s Must never return an error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_VOICE_REGISTRATION_STATE
 *
 * Request current registration state.
 */
void requestRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen, (void)request;
    int err = 0;
    const char resp_size = 15;
    int response[resp_size];
    char *responseStr[resp_size];
    ATResponse *creg_resp = NULL, *e2reg_resp = NULL;
    char *line;
    int commas = 0;
    int skip, cs_status = 0;
    int i;

    /* Setting default values in case values are not returned by AT command */
    for (i = 0; i < resp_size; i++)
        responseStr[i] = NULL;

    memset(response, 0, sizeof(response));

    /* IMPORTANT: Will take screen state lock here. Make sure to always call
                  releaseScreenStateLock BEFORE returning! */
    getScreenStateLock();
    if (!getScreenState())
        (void)at_send_command("AT+CREG=2"); /* Ignore the response, not VITAL. */
    else {
        response[0] = s_creg_stat;
        response[1] = s_creg_lac;
        response[2] = s_creg_cid;
        if (response[0] == CGREG_STAT_REG_DENIED)
            response[13] = convertCsRegistrationDeniedReason(s_cs_status);
        goto cached;
    }

    err = at_send_command_singleline("AT+CREG?", "+CREG:", &creg_resp);

    if (at_get_cme_error(err) == CME_SIM_NOT_INSERTED)
        goto no_sim;

    if (err != AT_NOERROR)
        goto error;

    line = creg_resp->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    /*
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be.
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both.
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here.
     *
     * finally, a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     */

    /* Count number of commas */
    err = at_tok_charcounter(line, ',', &commas);

    if (err < 0)
        goto error;

    switch (commas) {
    case 0:                    /* +CREG: <stat> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        response[1] = -1;
        response[2] = -1;
        break;

    case 1:                    /* +CREG: <n>, <stat> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        response[1] = -1;
        response[2] = -1;
        if (err < 0)
            goto error;
        break;
    case 2:                    /* +CREG: <stat>, <lac>, <cid> */
        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0)
            goto error;
        break;
    case 3:                    /* +CREG: <n>, <stat>, <lac>, <cid> */
    case 4:                    /* +CREG: <n>, <stat>, <lac>, <cid>, <?> */
        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response[0]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[1]);
        if (err < 0)
            goto error;

        err = at_tok_nexthexint(&line, &response[2]);
        if (err < 0)
            goto error;
        break;
    default:
        goto error;
    }

    s_registrationDeniedReason = DEFAULT_VALUE;

    if (response[0] == CGREG_STAT_REG_DENIED) {
        err = at_send_command_singleline("AT*E2REG?", "*E2REG:", &e2reg_resp);

        if (err != AT_NOERROR)
            goto error;

        line = e2reg_resp->p_intermediates->line;
        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cs_status);
        if (err < 0)
            goto error;

        response[13] = convertCsRegistrationDeniedReason(cs_status);
        s_registrationDeniedReason = response[13];
    }

    if (s_creg_stat != response[0] ||
        s_creg_lac != response[1] ||
        s_creg_cid != response[2]) {

        s_creg_stat = response[0];
        s_creg_lac = response[1];
        s_creg_cid = response[2];
        s_reg_change = 1;
    }

cached:
    if (response[0] == CGREG_STAT_REG_HOME_NET ||
        response[0] == CGREG_STAT_ROAMING)
        responseStr[3] = getNetworkType(0);

no_sim:
    err = asprintf(&responseStr[0], "%d", response[0]);
    if (err < 0)
            goto error;

    if (response[1] > 0)
        err = asprintf(&responseStr[1], "%04x", response[1]);
    if (err < 0)
        goto error;

    if (response[2] > 0)
        err = asprintf(&responseStr[2], "%08x", response[2]);
    if (err < 0)
        goto error;

    if (response[13] > 0)
        err = asprintf(&responseStr[13], "%d", response[13]);
    if (err < 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr,
                          resp_size * sizeof(char *));

finally:
    if (!getScreenState())
        (void)at_send_command("AT+CREG=0");

    releaseScreenStateLock(); /* Important! */

    for (i = 0; i < resp_size; i++)
        free(responseStr[i]);

    at_response_free(creg_resp);
    at_response_free(e2reg_resp);
    return;

error:
    ALOGE("%s() Must never return an error when radio is on", __func__);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

/**
 * RIL_REQUEST_OPERATOR
 *
 * Request current operator ONS or EONS.
 */
void requestOperator(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    int err;
    int i;
    int skip;
    ATLine *cursor;
    static const int num_resp_lines = 3;
    char *response[num_resp_lines];
    static char *old_response[3] = {NULL, NULL, NULL};
    ATResponse *atresponse = NULL;

    memset(response, 0, sizeof(response));

    if (!(s_creg_stat == CGREG_STAT_REG_HOME_NET ||
        s_creg_stat == CGREG_STAT_ROAMING ||
        s_cgreg_stat == CGREG_STAT_REG_HOME_NET ||
        s_cgreg_stat == CGREG_STAT_ROAMING))
        goto no_sim;

    if (!(s_reg_change)) {
        if (old_response[0] != NULL) {
            memcpy(response, old_response, sizeof(old_response));
            ALOGW("%s() Using buffered info since no change in state", __func__);
            goto no_sim;
        }
    }

    s_reg_change = 0;

    err = at_send_command_multiline
        ("AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?", "+COPS:",
         &atresponse);

    if (at_get_cme_error(err) == CME_SIM_NOT_INSERTED)
        goto no_sim;

    if (err != AT_NOERROR)
        goto error;

    /* We expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     */
    for (i = 0, cursor = atresponse->p_intermediates;
         cursor != NULL && i < num_resp_lines;
         cursor = cursor->p_next, i++) {
        char *line = cursor->line;

        err = at_tok_start(&line);

        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &skip);

        if (err < 0)
            goto error;

        /* If we're unregistered, we may just get
           a "+COPS: 0" response. */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);

        if (err < 0)
            goto error;

        /* A "+COPS: 0, n" response is also possible. */
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));

        if (err < 0)
            goto error;
    }

    if (i != num_resp_lines)
        goto error;

    /*
     * Check if modem returned an empty string, and fill it with MNC/MMC
     * if that's the case.
     */
    if (response[2] && response[0] && strlen(response[0]) == 0) {
        response[0] = alloca(strlen(response[2]) + 1);
        strcpy(response[0], response[2]);
    }

    if (response[2] && response[1] && strlen(response[1]) == 0) {
        response[1] = alloca(strlen(response[2]) + 1);
        strcpy(response[1], response[2]);
    }
    for (i = 0; i < num_resp_lines; i++) {
        if (old_response[i] != NULL) {
            free(old_response[i]);
            old_response[i] = NULL;
        }
        if (response[i] != NULL) {
            old_response[i] = strdup(response[i]);
        }
    }

no_sim:
    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}
