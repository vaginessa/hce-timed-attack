#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <winscard.h>

const char file_name[] = "output";

void timespec_diff(struct timespec *result, struct timespec *stop, struct timespec *start)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

int main(void)
{
    FILE *fp = NULL;
    char str[100];
    int ret = 0;

    LONG rv;

    SCARDCONTEXT hContext;
    LPTSTR mszReaders;
    SCARDHANDLE hCard;
    DWORD dwReaders, dwActiveProtocol, dwRecvLength;

    SCARD_IO_REQUEST pioSendPci;
    BYTE pbRecvBuffer[258];

    #define MSG 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, \
                0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, \
                0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04

    BYTE RequestAID[] = { 0x00, 0xA4, 0x04, 0x00, 0x0A,
                    0xF0, 0xA0, 0x00, 0x00, 0x62, 0x03, 0x01, 0x0C, 0x06, 0x01 };

    BYTE RequestPIN[] = { 0x00, 0x20, 0x00, 0x01, 0x08, 0x31, 0x32,
                          0x33, 0x34, 0xFF, 0xFF, 0xFF, 0xFF };

    BYTE WriteBinary[] = { 0x80, 0xD0, 0x00, 0x00, 0x24, MSG, 0x00 };

    printf("sizeof byte is %lu\n", (sizeof(WriteBinary) / sizeof(WriteBinary[0])));

    unsigned int i;

    fp = fopen(file_name, "w+");
    if (fp == NULL) {
        printf("fopen() failed. errno %d\n", errno);
        return -1;
    }

    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardEstablishContext: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }

#ifdef SCARD_AUTOALLOCATE
    dwReaders = SCARD_AUTOALLOCATE;

    rv = SCardListReaders(hContext, NULL, (LPTSTR)&mszReaders, &dwReaders);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardListReaders: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }
#else
    rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardListReaders: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }

    mszReaders = calloc(dwReaders, sizeof(char));
    rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardListReaders: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }
#endif
    printf("reader name: %s\n", mszReaders);

    rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED,
                      SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardConnect: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }
    switch(dwActiveProtocol) {
        case SCARD_PROTOCOL_T0:
            pioSendPci = *SCARD_PCI_T0;
            break;

        case SCARD_PROTOCOL_T1:
            pioSendPci = *SCARD_PCI_T1;
            break;
    }

//    for (i = 0; i < 500; i++) {
        struct timespec ts_before;
        struct timespec ts_after;
        struct timespec ts_res;
        dwRecvLength = sizeof(pbRecvBuffer);
//        clock_gettime(CLOCK_REALTIME, &ts_before);
        rv = SCardTransmit(hCard, &pioSendPci, RequestAID, sizeof(RequestAID),
                           NULL, pbRecvBuffer, &dwRecvLength);
        // clock_gettime(CLOCK_REALTIME, &ts_after);
        // timespec_diff(&ts_res, &ts_after, &ts_before);
        // sprintf(str, "%lld.%.9ld\n", (long long)ts_res.tv_sec, ts_res.tv_nsec);
        // fprintf(fp, str, sizeof(str));
        if (SCARD_S_SUCCESS != rv) {
            printf("SCardTransmit: %s\n", pcsc_stringify_error(rv));
            ret = -1;
            goto out;
        }
        printf("%d: response: ", i);
        for(int j = 0; j < dwRecvLength; j++) {
            printf("%c", pbRecvBuffer[j]);
        }
        printf("\n");
//    }

    // dwRecvLength = sizeof(pbRecvBuffer);
    // rv = SCardTransmit(hCard, &pioSendPci, RequestPIN, sizeof(RequestPIN),
    //                    NULL, pbRecvBuffer, &dwRecvLength);
    // if (SCARD_S_SUCCESS != rv) {
    //     printf("SCardTransmit: %s\n", pcsc_stringify_error(rv));
    //     ret = -1;
    //     goto out;
    // }

    // printf("response: ");
    // for(i = 0; i < dwRecvLength; i++) {
    //     printf("%c", pbRecvBuffer[i]);
    // }
    // printf("\n");

    dwRecvLength = sizeof(pbRecvBuffer);
    rv = SCardTransmit(hCard, &pioSendPci, WriteBinary, sizeof(WriteBinary),
                       NULL, pbRecvBuffer, &dwRecvLength);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardTransmit: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }

    printf("response: ");
    for(i = 0; i < dwRecvLength; i++) {
        printf("%c", pbRecvBuffer[i]);
    }
    printf("\n");

    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardDisconnect: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }

#ifdef SCARD_AUTOALLOCATE
    rv = SCardFreeMemory(hContext, mszReaders);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardFreeMemory: %s\n", pcsc_stringify_error(rv));
        ret = -1;
        goto out;
    }
#else
    free(mszReaders);
#endif

    rv = SCardReleaseContext(hContext);
    if (SCARD_S_SUCCESS != rv) {
        printf("SCardReleaseContext: %s\n", pcsc_stringify_error(rv));
        ret = -1;
    }

out:

    return 0;
}