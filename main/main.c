#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#define SAMPLE_COUNT 800

//uint16_t adc_buffer[SAMPLE_COUNT];

static const char *TAG = "ADC";


static adc_oneshot_unit_handle_t adc_handle;

/*-------------------------------------------------------------
 * ADC Initialization
 *------------------------------------------------------------*/
void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config =
    {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config =
    {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
                        adc_handle,
                        ADC_CHANNEL_0,
                        &config));
}

/*-------------------------------------------------------------
 * Read ADC
 *------------------------------------------------------------*/
int adc_read(void)
{
    int raw = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(
                        adc_handle,
                        ADC_CHANNEL_0,
                        &raw));

    return raw;
}

/*-------------------------------------------------------------
 * Main
 *------------------------------------------------------------*/
void app_main(void)
{
    adc_init();

    ESP_LOGI(TAG, "ADC Started");

    uint16_t adc_buffer[SAMPLE_COUNT];

    while (1)
    {
        /* Collect 200 samples */
        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            adc_buffer[i] = adc_read();

            /* Sampling interval = 1 ms */
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        /* Print all samples after collection */
        ESP_LOGI(TAG, "---------------- Samples ----------------");

        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            float voltage = ((float)adc_buffer[i] * 3.3f) / 4095.0f;

            printf("%.2f\n", voltage);
        }

        ESP_LOGI(TAG, "-----------------------------------------");

        /* Wait before collecting the next set */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
