#include "RequestGenerator.h"

#include "main.h"

#include <string.h>
#include <stdio.h>


#define HUE_URL "http://" HUE_IP "/api/"
#define HUE_USERNAME "hVC1QjzakMA58CjXBPy2wRB3KX3GZvZccI66o9dx"
#define HUE_LIGHTS HUE_URL HUE_USERNAME "/lights"
#define HUE_LAMP HUE_URL HUE_USERNAME "/lights/%d/state"
#define HUE_GROUP HUE_URL HUE_USERNAME "/groups/%d/action"

#define GET_REQUEST "GET " HUE_LIGHTS " HTTP/1.0\r\n" \
    "Host: " HUE_IP "\r\n" \
    "\r\n" 

#define PUT_REQUEST "PUT " HUE_LAMP " HTTP/1.1\r\n" \
    "Host: " HUE_IP "\r\n" \
    "Content-Length: %d\r\n" \
    "\r\n"


int32_t RequestGenerator::get(char* outputBuffer, uint32_t bufferSize)
{
    const int32_t contentLen = strlen(GET_REQUEST) + 1;

    if(bufferSize < contentLen) return -1;

    strncpy(outputBuffer, GET_REQUEST, bufferSize);

    return contentLen;
}


int32_t RequestGenerator::put(char* outputBuffer, uint32_t bufferSize, 
        int8_t on, int16_t bri, int32_t hue, int16_t sat, 
        int32_t ct, int32_t transitiontime)
{
	int32_t contentLen = 0;

	outputBuffer[contentLen++] = '{';

	if(on >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
            bufferSize - contentLen, "\"on\": %s,", 
            (on ? "true" : "false"))) < 0) return -1;
	}

	if(bri >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
        bufferSize - contentLen, "\"bri\": %d,", bri)) < 0) return -1;
	}

	if(hue >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
            bufferSize - contentLen, "\"hue\": %d,", hue)) < 0) return -1;
	}

	if(sat >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
            bufferSize - contentLen, "\"sat\": %d,", sat)) < 0) return -1;
	}

    if(ct >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
            bufferSize - contentLen, "\"ct\": %d,", ct)) < 0) return -1;
	}

	if(transitiontime >= 0)
	{
		if((contentLen += snprintf(outputBuffer + contentLen, 
            bufferSize - contentLen, "\"transitiontime\": %d,", 
            transitiontime)) < 0) return -1;
	}

    outputBuffer[contentLen - 1] = '}';

    /* Check size */
	if((contentLen + 1) > bufferSize) return -1;
	
    outputBuffer[contentLen++] = '\0';

	return strlen(outputBuffer);
}


int32_t RequestGenerator::addPutHeader(char* outputBuffer, uint32_t bufferSize, 
        char* content, uint32_t contentLen, uint8_t lampId)
{
    int32_t headerLen = 0;

    /* Generate header */
	if((headerLen = snprintf(outputBuffer, bufferSize, PUT_REQUEST, lampId, 
        contentLen)) < 0) return -1;

    /* Check size */
	if((headerLen + contentLen + 1) > bufferSize) return -1;

    /* Add content to header */
	memcpy(outputBuffer + headerLen, content, contentLen + 1);

	return strlen(outputBuffer);
}
    