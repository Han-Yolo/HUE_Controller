#ifndef INPUT_H
#define INPUT_H


#include <stdint.h>


class Input
{
public:

    static void init(void);

private:

    struct event_s
    {
        uint8_t source;
        uint16_t value;
    };

    static void interrupt(void* pParam);
    static void adcTask(void* pParam);
    static void eventTask(void* pParam);
};


#endif /* INPUT_H */