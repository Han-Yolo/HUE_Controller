#include "Input.h"

#include "App.h"

#include <driver/adc.h>
#include <driver/gpio.h>

#include <esp_log.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


#define GPIO_BUTTON_TOP     GPIO_NUM_14
#define GPIO_BUTTON_MIDDLE  GPIO_NUM_12
#define GPIO_BUTTON_BOTTOM  GPIO_NUM_13

#define GPIO_SWITCH_LEFT    GPIO_NUM_2
#define GPIO_SWITCH_RIGHT   GPIO_NUM_0

#define SOURCE_ADC          0xFF

#define LOG_TAG             "Input"


static xQueueHandle eventQueue = NULL;


void Input::init(void)
{
    gpio_config_t gpioConf;

    /* Init buttons */
    gpioConf.pin_bit_mask = BIT(GPIO_BUTTON_TOP) | 
                            BIT(GPIO_BUTTON_MIDDLE) | 
                            BIT(GPIO_BUTTON_BOTTOM);
    gpioConf.mode = GPIO_MODE_INPUT;
    gpioConf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpioConf.intr_type = GPIO_INTR_NEGEDGE;
    ESP_ERROR_CHECK(gpio_config(&gpioConf));

    /* Init switch */
    gpioConf.pin_bit_mask = BIT(GPIO_SWITCH_LEFT) | 
                            BIT(GPIO_SWITCH_RIGHT);
    gpioConf.mode = GPIO_MODE_INPUT;
    gpioConf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpioConf.intr_type = GPIO_INTR_NEGEDGE;
    ESP_ERROR_CHECK(gpio_config(&gpioConf));

    /* Init slider */
    adc_config_t adcConf;
    adcConf.mode = ADC_READ_TOUT_MODE;
    adcConf.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    ESP_ERROR_CHECK(adc_init(&adcConf));

    /* Init interrupt queue */
    eventQueue = xQueueCreate(20, sizeof(event_s));

    /* Init GPIO interrupts */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON_TOP, interrupt, (void*)GPIO_BUTTON_TOP));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON_MIDDLE, interrupt, (void*)GPIO_BUTTON_MIDDLE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON_BOTTOM, interrupt, (void*)GPIO_BUTTON_BOTTOM));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_SWITCH_LEFT, interrupt, (void*)GPIO_SWITCH_LEFT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_SWITCH_RIGHT, interrupt, (void*)GPIO_SWITCH_RIGHT));

    /* Init task */
    xTaskCreate(adcTask, "ADC task", 1024, nullptr, 8, nullptr);
    xTaskCreate(eventTask, "Input task", 16384, nullptr, 8, nullptr);
}


void Input::interrupt(void* pParam)
{
    const TickType_t blockTime = 10;
    static TickType_t lastIsrTme = 0;
    TickType_t isrTime = xTaskGetTickCount();

    if((isrTime - lastIsrTme) > blockTime)
    {
        event_s event;
        event.source = (uint8_t)(uint32_t)pParam;
        event.value = 0;
        xQueueSendFromISR(eventQueue, &event, nullptr);
    }

    lastIsrTme = isrTime;
}


void Input::adcTask(void* pParam)
{
    uint16_t newVal = 0, oldVal = 0;
	uint16_t threshold = 10;

    while(true)
    {
        ESP_ERROR_CHECK(adc_read(&newVal));

        //ESP_LOGI(LOG_TAG, "Diff: %d\n", (int32_t)oldVal - newVal);
		if(abs((int32_t)oldVal - newVal) > threshold)
		{
            event_s event;
            event.source = SOURCE_ADC;
            event.value = newVal;
            xQueueSend(eventQueue, &event, 10);

			oldVal = newVal;
		}

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}


void Input::eventTask(void* pParam)
{
    event_s event;

    while(true)
    {
        if(xQueueReceive(eventQueue, &event, portMAX_DELAY) == pdFALSE)
        {
            continue;
        }

        if(event.source == SOURCE_ADC)
        {
            ESP_LOGI(LOG_TAG, "Adc %d", event.value);
            App::instance().newAdVal(event.value);
        }
        else
        {
            vTaskDelay(20 / portTICK_PERIOD_MS);

            if(gpio_get_level((gpio_num_t)event.source) != 0)
            {
                continue;
            }

            switch(event.source)
            {
                case GPIO_BUTTON_TOP:
                {
                    ESP_LOGI(LOG_TAG, "Btn top");
                    App::instance().buttonPress(button_e::TOP);
                    break;
                }

                case GPIO_BUTTON_MIDDLE:
                {
                    ESP_LOGI(LOG_TAG, "Btn middle");
                    App::instance().buttonPress(button_e::MIDDLE);
                    break;
                }

                case GPIO_BUTTON_BOTTOM:
                {
                    ESP_LOGI(LOG_TAG, "Btn bottom");
                    App::instance().buttonPress(button_e::BOTTOM);
                    break;
                }

                case GPIO_SWITCH_LEFT:
                {
                    ESP_LOGI(LOG_TAG, "Switch left");
                    App::instance().switchAction(switch_e::LEFT);
                    break;
                }

                case GPIO_SWITCH_RIGHT:
                {
                    ESP_LOGI(LOG_TAG, "Switch right");
                    App::instance().switchAction(switch_e::RIGHT);
                    break;
                }

                default:
                {
                    ESP_LOGI(LOG_TAG, "Unknown source");
                    break;
                }
            }
        }
    }
}