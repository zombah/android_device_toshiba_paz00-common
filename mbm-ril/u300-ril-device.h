#ifndef U300_RIL_INFORMATION_H
#define U300_RIL_INFORMATION_H 1

#include <telephony/ril.h>

void requestGetIMSI(void *data, size_t datalen, RIL_Token t);
void requestDeviceIdentity(void *data, size_t datalen, RIL_Token t);
void requestGetIMEI(void *data, size_t datalen, RIL_Token t);
void requestGetIMEISV(void *data, size_t datalen, RIL_Token t);
void requestBasebandVersion(void *data, size_t datalen, RIL_Token t);

int retryRadioPower(void);
int isRadioOn(void);
void setRadioState(RIL_RadioState newState);
RIL_RadioState getRadioState(void);
void onSIMReady(void *p);
void sendTime(void *p);
char *getTime(void);

void clearDeviceInfo(void);

/* readDeviceInfo needs to be called while unsolicited responses are not yet
 * enabled.
 */
void readDeviceInfo(void);

/* getdeviceInfo returns a pointer to allocated memory of a character string.
 * Note: Caller need to take care of freeing the allocated memory by calling
 * free( ) when the alllocated string is not longer used.
 */
char *getDeviceInfo(const char *info);

#endif
