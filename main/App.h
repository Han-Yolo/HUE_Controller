#ifndef APP_H
#define APP_H


#include "main.h"
#include "Wifi.h"
#include "LedStrip.h"
#include "Input.h"

#include "FreeRTOS.h"
#include "timers.h"


enum class button_e : uint32_t
{
    TOP = 0,
    MIDDLE,
    BOTTOM
};

enum class switch_e : uint32_t
{
    LEFT = 0,
    RIGHT
};


class App
{
public:

    static App& instance(void)
    {
        static App instance;
        return instance;
    }

    void init(void);

    void newAdVal(uint16_t adVal);
    void buttonPress(button_e button);
    void switchAction(switch_e switchDir);

private:

    enum controlMode_e : int32_t
    {
        CONTROLMODE_BRIGHTNESS = 0,
        CONTROLMODE_HUE,
        CONTROLMODE_SATURATION,
        CONTROLMODE_COLOR_TEMPERATURE,
        NUM_CONTROLMODES
    };

    enum lampComboMode : int32_t
    {
        LAMPCOMBOMODE_ALL_ON = 0,
        LAMPCOMBOMODE_NO_CEILING,
        NUM_LAMPCOMBOMODES
    };

    enum class colorMode_e : uint8_t
    {
        HS = 0,
        CT,
        XY
    };

    App();

    void setMode(void);
    void setLampComboMode(void);
    bool sendRequest(uint8_t firstLamp, uint8_t lastLamp, int32_t requestLen);

    static void shutdown(TimerHandle_t timer);

    bool m_FirstSend;

    uint32_t m_NumLamps;
    int32_t m_ControlMode;
    int32_t m_LampComboMode;
    colorMode_e m_ColorMode;
    uint8_t m_Brightness;
    uint16_t m_HUE;
    uint8_t m_Saturation;
    uint16_t m_CT;

    char m_ContentBuffer[512];
    char m_WifiSendBuffer[512];
    char m_WifiRecBuffer[10000];

    TimerHandle_t m_ShutdownTimer;
    static const uint32_t m_ShutdownTimeout = 20000;
};


#endif /* APP_H */