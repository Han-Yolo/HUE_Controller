#ifndef WIFI_H
#define WIFI_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


void wifi_init(void);

int32_t wifi_send(const char* sendData, const uint32_t sendDataLen,
        char* recDataBuffer, uint32_t recDataBufferLen, uint32_t recDelay);


#ifdef __cplusplus
}
#endif


#endif /* WIFI_H */