#include "App.h"

#include "RequestGenerator.h"
#include "JsonObject.h"

#include <esp_log.h>
#include <driver/gpio.h>

#include "task.h"


#define LOG_TAG     "App"

#define ERROR(...)  { ESP_LOGE(LOG_TAG, __VA_ARGS__); return; }

#define GPIO_SHUTDOWN   GPIO_NUM_5


static const char* numStr[] = 
        {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

static const char* stateStr     = "state";
static const char* onStr        = "on";
static const char* colormodeStr = "colormode";
static const char* briStr       = "bri";
static const char* hueStr       = "hue";
static const char* satStr       = "sat";
static const char* ctStr        = "ct";


App::App()
{
    m_FirstSend = true;
    m_NumLamps = 0;
    m_ControlMode = CONTROLMODE_BRIGHTNESS;
    m_LampComboMode = LAMPCOMBOMODE_ALL_ON;
    m_ColorMode = colorMode_e::CT;
    m_Brightness = 0x00;
    m_HUE = 0x5555;
    m_Saturation = 0xFF;
    m_CT = 300;
    m_ShutdownTimer = nullptr;
}


void App::init(void)
{
    /* Init the shutdown gpio */
    gpio_config_t gpioConf;
    gpioConf.pin_bit_mask = BIT(GPIO_SHUTDOWN);
    gpioConf.mode = GPIO_MODE_OUTPUT;
    gpioConf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpioConf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpioConf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_set_level(GPIO_SHUTDOWN, 0));
    ESP_ERROR_CHECK(gpio_config(&gpioConf));

    /* Create the automatic shutdown timer */
    m_ShutdownTimer = xTimerCreate("Shutdown Timer", 
        pdMS_TO_TICKS(m_ShutdownTimeout), false, 
        (void*)0, shutdown);

    xTimerStart(m_ShutdownTimer, 0);

    LedStrip::init(20);

    wifi_init();

    /* Get the state of the HUE lamps */
    int32_t sendLen, recLen;
    sendLen = RequestGenerator::get(m_WifiSendBuffer,
        sizeof(m_WifiSendBuffer)/sizeof(m_WifiSendBuffer[0]));

    if(sendLen <= 0) ERROR("Get request generation failed!");

    recLen = wifi_send(m_WifiSendBuffer, sendLen, m_WifiRecBuffer, 
        sizeof(m_WifiRecBuffer), 10);

    if(recLen <= 0) ERROR("Get request failed!");

    //printf("reclen: %d\n", recLen);
    //printf(m_WifiRecBuffer);

    char* jsonStart = strchr(m_WifiRecBuffer, '{');
    if(jsonStart == nullptr) ERROR("JSON string not found!");

    JsonObject json(jsonStart);
    //json.print();

    /* Determine which lamp to get the status from */
    if(json.getNumObjects(nullptr, 0, &m_NumLamps) == false)
        ERROR("Number of lamps not found!");

    uint32_t lampToEvaluate = 0;
    for(uint32_t i = 1; i <= m_NumLamps; i++)
    {
        bool lampOn = false;
        const char* lampOnPath[] = {numStr[i], stateStr, onStr};
        if(json.getBool(lampOnPath, 
            sizeof(lampOnPath)/sizeof(lampOnPath[0]), 
            &lampOn) == false) ERROR("Lamp on state not found!");

        if(lampOn)
        {
            lampToEvaluate = i;
            break;
        }
    }

    if(lampToEvaluate >= sizeof(numStr)/sizeof(numStr[0]))
        ERROR("Index of lamp to evaluate to high: %d!", lampToEvaluate);
    
    /* Get the color of the selected lamp and set the LED strip 
     * accordingly if an enabled lamp was found */
    if(lampToEvaluate > 0)
    {
        int64_t bri = 0;
        const char* briPath[] = 
            {numStr[lampToEvaluate], stateStr, briStr};

        if(json.getInt(briPath, 
            sizeof(briPath)/sizeof(briPath[0]), 
            &bri) == false) ERROR("Brightness value not found!");

        m_Brightness = (uint8_t)bri;

        char* colorMode;
        const char* colorModePath[] = 
            {numStr[lampToEvaluate], stateStr, colormodeStr};

        if(json.getString(colorModePath, 
            sizeof(colorModePath)/sizeof(colorModePath[0]), 
            &colorMode) == false) ERROR("Color mode not found!");

        if(strcmp(colorMode, "hs") == 0)
        {
            ESP_LOGI(LOG_TAG, "Color mode HS");

            int64_t hue = 0;
            const char* huePath[] = 
                {numStr[lampToEvaluate], stateStr, hueStr};

            if(json.getInt(huePath, 
                sizeof(huePath)/sizeof(huePath[0]), 
                &hue) == false) ERROR("HUE value not found!");

            int64_t sat = 0;
            const char* satPath[] = 
                {numStr[lampToEvaluate], stateStr, satStr};

            if(json.getInt(satPath, 
                sizeof(satPath)/sizeof(satPath[0]), 
                &sat) == false) ERROR("Saturation value not found!");

            m_ColorMode = colorMode_e::HS;
            m_HUE = (uint16_t)hue;
            m_Saturation = (uint16_t)sat;
        }
        else if(strcmp(colorMode, "ct") == 0)
        {
            ESP_LOGI(LOG_TAG, "Color mode CT");

            int64_t ct = 0;
            const char* ctPath[] = 
                {numStr[lampToEvaluate], stateStr, ctStr};

            if(json.getInt(ctPath, 
                sizeof(ctPath)/sizeof(ctPath[0]), 
                &ct) == false) ERROR("CT value not found!");

            m_ColorMode = colorMode_e::CT;
            m_CT = (uint16_t)ct;
        }
        else if(strcmp(colorMode, "xy") == 0)
        {
            ESP_LOGI(LOG_TAG, "Color mode XY");

            /* Use default values */
        }
        else ERROR("Color mode unknown: %s!", colorMode);
    }

    setMode();

    Input::init();
}


void App::newAdVal(uint16_t adVal)
{
    int32_t contentLen = 0;

    xTimerReset(m_ShutdownTimer, 0);

    switch(m_ControlMode)
    {
        case CONTROLMODE_BRIGHTNESS:
        {
            int8_t on = -1;
            if(m_FirstSend)
            {
                m_FirstSend = false;
                on = 1;
            }

            m_Brightness = ((uint32_t)adVal*254)/1023;
            ESP_LOGI(LOG_TAG, "New brightness %d", m_Brightness);

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                on, m_Brightness, -1, -1, -1, 2);

            break;
        }

        case CONTROLMODE_HUE:
        {
            m_ColorMode = colorMode_e::HS;
            m_HUE = ((uint32_t)adVal*65535)/1023;
            ESP_LOGI(LOG_TAG, "New HUE %d", m_HUE);

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                -1, -1, m_HUE, m_Saturation, -1, 2);

            break;
        }

        case CONTROLMODE_SATURATION:
        {
            m_ColorMode = colorMode_e::HS;
            m_Saturation = ((uint32_t)adVal*254)/1023;
            ESP_LOGI(LOG_TAG, "New saturation %d", m_Saturation);

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                -1, -1, m_HUE, m_Saturation, -1, 2);
            
            break;
        }

        case CONTROLMODE_COLOR_TEMPERATURE:
        {
            m_ColorMode = colorMode_e::CT;
            m_CT = ((uint32_t)adVal*347)/1023 + 153;
            ESP_LOGI(LOG_TAG, "New color temperature %d", m_CT);

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                -1, -1, -1, -1, m_CT, 2);

            break;
        }

        default:
        {
            ESP_LOGE(LOG_TAG, "Unknown mode %d!", m_ControlMode);
            break;
        }
    }

    sendRequest(1, m_NumLamps, contentLen);
}


void App::buttonPress(button_e button)
{
    xTimerReset(m_ShutdownTimer, 0);

    switch(button)
    {
        case button_e::TOP:
        {
            m_LampComboMode++;
            if(m_LampComboMode >= NUM_LAMPCOMBOMODES)
                m_LampComboMode = 0;

            setLampComboMode();
            break;
        }

        case button_e::MIDDLE:
        {
            break;
        }

        case button_e::BOTTOM:
        {
            ESP_LOGI(LOG_TAG, "Bye Bye");

            int32_t contentLen = 0;
            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                0, -1, -1, -1, -1, 2);

            sendRequest(1, m_NumLamps, contentLen);

            shutdown(nullptr);
            break;
        }
    }
}


void App::switchAction(switch_e switchDir)
{
    xTimerReset(m_ShutdownTimer, 0);

    switch(switchDir)
    {
        case switch_e::LEFT:
        {
            m_ControlMode++;
            if(m_ControlMode >= NUM_CONTROLMODES) m_ControlMode = 0;
            break;
        }

        case switch_e::RIGHT:
        {
            m_ControlMode--;
            if(m_ControlMode < 0) m_ControlMode = NUM_CONTROLMODES - 1;
            break;
        }

        default:
        {
            ESP_LOGE(LOG_TAG, "Unknown switch direction!");
            break;
        }
    }

    setMode();
}


bool App::sendRequest(uint8_t firstLamp, uint8_t lastLamp, int32_t requestLen)
{
    if(firstLamp > lastLamp) return false;
    if(requestLen <= 0) return false;

    for(uint8_t lampId = firstLamp; lampId <= lastLamp; lampId++)
    {
        int32_t putLen = RequestGenerator::addPutHeader(m_WifiSendBuffer,
            sizeof(m_WifiSendBuffer)/sizeof(m_WifiSendBuffer[0]),
            m_ContentBuffer, requestLen, lampId);

        if(putLen > 0)
        {
            wifi_send(m_WifiSendBuffer, putLen, m_WifiRecBuffer, 
                sizeof(m_WifiRecBuffer)/sizeof(m_WifiRecBuffer[0]), 0);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Put request generation failed!");
            return false;
        }
    }
    return true;
}


void App::setMode(void)
{
    switch(m_ControlMode)
    {
        case CONTROLMODE_BRIGHTNESS:
        {
            ESP_LOGI(LOG_TAG, "Mode brightness");
            
            switch(m_ColorMode)
            {
                case colorMode_e::HS:
                    LedStrip::showBrightness(m_HUE, m_Saturation);
                    break;

                case colorMode_e::CT:
                    LedStrip::showBrightness(m_CT);
                    break;

                default:
                    break;
            }
            
            break;
        }

        case CONTROLMODE_HUE:
        {
            ESP_LOGI(LOG_TAG, "Mode HUE");
            LedStrip::showHUE(m_Saturation);
            break;
        }

        case CONTROLMODE_SATURATION:
        {
            ESP_LOGI(LOG_TAG, "Mode saturation");
            LedStrip::showSaturation(m_HUE);

            break;
        }

        case CONTROLMODE_COLOR_TEMPERATURE:
        {
            ESP_LOGI(LOG_TAG, "Mode white\n");
            LedStrip::showColorTemperature();
            break;
        }

        default:
        {
            ESP_LOGE(LOG_TAG, "Unknown mode %d!", m_ControlMode);
            break;
        }
    }
}


void App::setLampComboMode(void)
{
    int32_t contentLen = 0;

    switch(m_LampComboMode)
    {
        case LAMPCOMBOMODE_ALL_ON:
        {
            ESP_LOGI(LOG_TAG, "All lamps on");

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                1, m_Brightness, -1, -1, -1, 2);

            sendRequest(1, m_NumLamps, contentLen);
            break;
        }

        case LAMPCOMBOMODE_NO_CEILING:
        {
            ESP_LOGI(LOG_TAG, "All but ceiling lamps on");

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                0, -1, -1, -1, -1, 2);

            sendRequest(1, 3, contentLen);

            contentLen = RequestGenerator::put(m_ContentBuffer, 
                sizeof(m_ContentBuffer)/sizeof(m_ContentBuffer[0]),
                1, m_Brightness, -1, -1, -1, 2);

            sendRequest(4, m_NumLamps, contentLen);
            break;
        }

        default:
        {
            ESP_LOGE(LOG_TAG, "Unknown lamp combo mode!");
            break;
        }
    }
}


void App::shutdown(TimerHandle_t timer)
{
    ESP_ERROR_CHECK(gpio_set_level(GPIO_SHUTDOWN, 1));
}


extern "C" void runApp(void)
{
    App::instance().init();
}