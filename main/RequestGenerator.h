#ifndef REQUESTGENERATOR_H
#define REQUESTGENERATOR_H


#include <stdint.h>


class RequestGenerator
{
public:

    static int32_t get(char* outputBuffer, uint32_t bufferSize);

    static int32_t put(char* outputBuffer, uint32_t bufferSize, 
        int8_t on, int16_t bri, int32_t hue, int16_t sat, 
        int32_t ct, int32_t transitiontime);

    static int32_t addPutHeader(char* outputBuffer, uint32_t bufferSize, 
        char* content, uint32_t contentLen, uint8_t lampId);
};


#endif /* REQUESTGENERATOR_H */