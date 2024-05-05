#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "tusb.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include <task.h>
#include <hal_i2c_host.h>
#include <i2c_helper.hpp>
#include <sensor_fingerposition.hpp>

inline constexpr uint8_t I2C_SDA_PIN=0;
inline constexpr uint8_t I2C_SCL_PIN=1;
inline constexpr long I2C_SPEED=100e3;

inline constexpr uint16_t SAMPLE_RATE_MS=100;

void error_handler()
{
    cyw43_arch_disable_sta_mode();
    cyw43_arch_deinit();
    while (1)
    {
        ;
    }
}

void uart_send_task(void *params)
{
    uint32_t ms1, ms2 = 0;
    TickType_t xLastWakeTime;
    FingerPositionSensor fpos;

    /* Set I2C SDA and SCL pin functions to I2C peripheral (mux settings)*/
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    i2c_host_init(I2C_PERIPHERAL_0, I2C_CLK_SOURCE_USE_DEFAULT, configCPU_CLOCK_HZ, I2C_SPEED, I2C_EXTRA_OPT_NONE);
    I2CDriver Driver(I2C_PERIPHERAL_0, kI2cSpeed_100KHz);

    fpos.Initialize(&Driver);
    const TickType_t xFrequency = SAMPLE_RATE_MS / portTICK_PERIOD_MS; // Convert 100ms to ticks

    // Initialize the last wake time
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        ms1 = xTaskGetTickCount();
        SensorData_t data = fpos.GetSensorData();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        ms2 = xTaskGetTickCount();
        printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n", ms1,
               ms2,
               data.buffer[0],
               data.buffer[1],
               data.buffer[2],
               data.buffer[3],
               data.buffer[4],
               data.buffer[5],
               data.buffer[6],
               data.buffer[7]);
    }
}

int main()
{
    stdio_init_all();
    while (!tud_cdc_available())
    {
    }
    xTaskCreate(uart_send_task, "UART_TX_Task", 250, NULL, 2, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }

    return 0;
}
