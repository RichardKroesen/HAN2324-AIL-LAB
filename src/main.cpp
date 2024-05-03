#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "tusb.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include <task.h>

#include <UART_STREAMER.hpp>

static SERIAL::UART_RTOS_Driver driver(115200, 16, 17);

void error_handler()
{
    cyw43_arch_disable_sta_mode();
    cyw43_arch_deinit();
    while (1)
    {
        ;
    }
}

void mainTask(void *params) {
    SERIAL::uart_buffer_t rxBuffer;

    while (1) {
        if (driver.uart_check_rx_non_blocking(&rxBuffer, sizeof(rxBuffer), pdMS_TO_TICKS(100))) {
            // Process received UART data
            printf("Received UART data: %s\n", rxBuffer.data);
        } else {
            // Yield to other tasks
            taskYIELD();
        }
    }
}

void uart_send_task(void *params) {
    const char* message = "Hello UART!\n";
    driver.uart_send_non_blocking(message);

    while (1) {
        driver.uart_send_non_blocking(message); // Non-blocking send
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

int main()
{
    stdio_init_all();

    xTaskCreate(mainTask, "UART_RX_Daemon", 200, NULL, 1, NULL); 
    xTaskCreate(uart_send_task, "UART_TX_Task", 200, NULL, 2, NULL);
    
    vTaskStartScheduler();
    
    for (;;)
    {
        ;
    }

    return 0;
}

