/* Ericsson MBM RIL
 *
 * Copyright (C) Ericsson AB 2011
 * Copyright 2006, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 */

#include "u300-ril-error.h"

const char *errorCauseToString(int cause)
{
    const char *string;

    switch (cause) {
        case E2NAP_CAUSE_SUCCESS:
            string = "Success";
            break;
        case E2NAP_CAUSE_GPRS_ATTACH_NOT_POSSIBLE:
            string = "GPRS attach not possible";
            break;
        case E2NAP_CAUSE_NO_SIGNAL_CONN:
            string = "No signaling connection";
            break;
        case E2NAP_CAUSE_REACTIVATION_POSSIBLE:
            string = "Reactivation possible";
            break;
        case E2NAP_CAUSE_ACCESS_CLASS_BARRED:
            string = "Access class barred";
            break;
        case GPRS_OP_DETERMINED_BARRING:
            string = "Operator Determined Barring";
            break;
        case GPRS_MBMS_CAPA_INFUFFICIENT:
            string = "MBMS bearer capabilities insufficient for the service";
            break;
        case GPRS_LLC_SNDCP_FAILURE:
            string = "LLC or SNDCP failure";
            break;
        case GPRS_INSUFFICIENT_RESOURCES:
            string = "Insufficient resources";
            break;
        case GPRS_UNKNOWN_APN:
            string = "Unknown or missing access point name";
            break;
        case GPRS_UNKNOWN_PDP_TYPE:
            string = "Unknown PDP address or PDP type";
            break;
        case GPRS_USER_AUTH_FAILURE:
            string = "User authentication failed";
            break;
        case GPRS_ACT_REJECTED_GGSN:
            string = "Activation rejected by GGSN";
            break;
        case GPRS_ACT_REJECTED_UNSPEC:
            string = "Activation rejected, unspecified";
            break;
        case GPRS_SERVICE_OPTION_NOT_SUPP:
            string = "Service option not supported";
            break;
        case GPRS_REQ_SER_OPTION_NOT_SUBS:
            string = "Requested service option not subscribed";
            break;
        case GPRS_SERVICE_OUT_OF_ORDER:
            string = "Service option temporarily out of order";
            break;
        case GPRS_NSAPI_ALREADY_USED:
            string = "NSAPI already used";
            break;
        case GPRS_REGULAR_DEACTIVATION:
            string = "Regular deactivation";
            break;
        case GPRS_QOS_NOT_ACCEPTED:
            string = "QoS not accepted";
            break;
        case GPRS_NETWORK_FAILURE:
            string = "Network failure";
            break;
        case GPRS_REACTIVATION_REQUESTED:
            string = "Reactivation requested";
            break;
        case GPRS_FEATURE_NOT_SUPPORTED:
            string = "Feature not supported";
            break;
        case GRPS_SEMANTIC_ERROR_TFT:
            string = "semantic error in the TFT operation.";
            break;
        case GPRS_SYNTACT_ERROR_TFT:
            string = "syntactical error in the TFT operation.";
            break;
        case GRPS_UNKNOWN_PDP_CONTEXT:
            string = "unknown PDP context";
            break;
        case GPRS_SEMANTIC_ERROR_PF:
            string = "semantic errors in packet filter(s)";
            break;
        case GPRS_SYNTACT_ERROR_PF:
            string = "syntactical error in packet filter(s)";
            break;
        case GPRS_PDP_WO_TFT_ALREADY_ACT:
            string = "PDP context without TFT already activated";
            break;
        case GPRS_MULTICAST_GMEM_TIMEOUT:
            string = "Multicast group membership time-out";
            break;
        case GPRS_ACT_REJECTED_BCM_VIOLATION:
            string = "Activation rejected, Bearer ControlMode violation";
            break;
        case GPRS_INVALID_TRANS_IDENT:
            string = "Invalid transaction identifier value.";
            break;
        case GRPS_SEM_INCORRECT_MSG:
            string = "Semantically incorrect message.";
            break;
        case GPRS_INVALID_MAN_INFO:
            string = "Invalid mandatory information.";
            break;
        case GPRS_MSG_TYPE_NOT_IMPL:
            string = "Message type non-existent or not implemented.";
            break;
        case GPRS_MSG_NOT_COMP_PROTOCOL:
            string = "Message not compatible with protocol state.";
            break;
        case GPRS_IE_NOT_IMPL:
            string = "Information element non-existent or not implemented.";
            break;
        case GPRS_COND_IE_ERROR:
            string = "Conditional IE error.";
            break;
        case GPRS_MSG_NOT_COMP_PROTO_STATE:
            string = "Message not compatible with protocol state.";
            break;
        case GPRS_PROTO_ERROR_UNSPECIFIED:
            string = "Protocol error, unspecified.";
            break;
        case GPRS_APN_RESTRICT_VALUE_INCOMP:
            string = "APN restriction value incompatible with active PDP context.";
            break;
        default:
            string = "E2NAP_CAUSE_<> Unknown!";
            break;
    }

    return string;
}

const char *e2napStateToString(int state)
{
    const char *string;

    switch (state) {
        case E2NAP_STATE_DISCONNECTED:
            string = "Disconnected";
            break;
        case E2NAP_STATE_CONNECTED:
            string = "Connected";
            break;
        case E2NAP_STATE_CONNECTING:
            string = "Connecting";
            break;
        default:
            string = "E2NAP_STATE_<> Unknown!";
            break;
    }

    return string;
}
