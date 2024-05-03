#ifndef UARTSTREAMER_HPP
#define UARTSTREAMER_HPP

#include "FreeRTOS.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define RX_MAX_BUFFER_PER_FRAME 30
#define QUEUE_SIZE              2
#define UART_PORT               uart0

namespace SERIAL {

typedef struct {
    char data[RX_MAX_BUFFER_PER_FRAME];
    uint16_t length;
} uart_buffer_t;

class UART_RTOS_Driver {
public:
    UART_RTOS_Driver(uint32_t baudRate, uint8_t txPin, uint8_t rxPin) :
        baud_rate(baudRate), tx_pin(txPin), rx_pin(rxPin) {
        initialize();
    };

    ~UART_RTOS_Driver() {
        vSemaphoreDelete(uart_tx_done_sem);
        deinitialize();
    }

    void initialize() {
        if (uart_tx_done_sem == nullptr) {
            uart_tx_done_sem = xSemaphoreCreateBinary();
        }

        if (uart_rx_queue == nullptr) {
            uart_rx_queue = xQueueCreate(QUEUE_SIZE, sizeof(uart_buffer_t)); 
        }

        if (! is_connected) {
            is_connected = true; 
            uart_init(uart_port, baud_rate);
            gpio_set_function(rx_pin, GPIO_FUNC_UART);
            gpio_set_function(tx_pin, GPIO_FUNC_UART);

            uart_set_irq_enables(uart_port, true, false); // Enable Reading.
            
            const uint8_t uartIRQ = (uart_port == uart0) ? UART0_IRQ : UART1_IRQ; 
            irq_set_exclusive_handler(uartIRQ, uart_interrupt_handler);
            irq_set_enabled(uartIRQ, true);
        }
    }

    void deinitialize() {
        uart_deinit(uart_port);
        is_connected = false;
        if (uart_tx_done_sem) {
            vSemaphoreDelete(uart_tx_done_sem);
            uart_tx_done_sem = nullptr;
        }
    }

    void uart_send_non_blocking(const char *data) {
        for (const char *ptr = data; *ptr != '\0'; ++ptr) {
            while (!uart_is_writable(uart_port)) {
                // Yield is preferred so that other tasks with higher prio can proceed
                taskYIELD();
                // Wait till writing is possible.
            }
            uart_putc_raw(uart_port, *data++);
        }
        
        uart_set_irq_enables(uart_port, true, true);
        taskYIELD();
        xSemaphoreTake(uart_tx_done_sem, portMAX_DELAY); // Wait for completion.
    }

    bool uart_check_rx_non_blocking(uart_buffer_t* outBuffer, uint16_t size, TickType_t waitTime) {
        if (outBuffer == nullptr || size == 0) {
            // Important to not proceed when outbuf is nullptr to prevent undefined behavior.
            return false;
        }

        uart_buffer_t buf;

        if (xQueueReceive(uart_rx_queue, &buf, waitTime) == pdPASS) {
            const uint16_t copySize = (size < sizeof(buf.data)) ? size : sizeof(buf.data);
            memcpy(outBuffer->data, buf.data, copySize);
            outBuffer->length = buf.length > copySize ? copySize : buf.length;
            return true;
        }

        return false;
    }

private:
    bool is_connected = false;
    uint32_t baud_rate; 
    static inline uart_inst_t* uart_port = UART_PORT;
    const uint8_t tx_pin;
    const uint8_t rx_pin;

    static inline SemaphoreHandle_t uart_tx_done_sem = nullptr;
    static inline QueueHandle_t uart_rx_queue = nullptr;

    static void uart_interrupt_handler() {
        if (uart_get_hw(uart_port)->fr & UART_UARTFR_TXFE_BITS) { // Check of: TX FIFO empty
            uart_set_irq_enables(uart_port, true, false); 
            xSemaphoreGiveFromISR(uart_tx_done_sem, NULL); 
        }

        if (uart_is_readable(uart_port)) {
            static uart_buffer_t buf = {.length = 0};
            while (uart_is_readable(uart_port)) {
                char rxChar = uart_getc(uart_port);
                if (rxChar != '\n' && buf.length < sizeof(buf.data) - 1) {
                    buf.data[buf.length++] = rxChar;
                } else {
                    buf.data[buf.length] = '\0'; // Null termination
                    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                    xQueueSendToBackFromISR(uart_rx_queue, &buf, &xHigherPriorityTaskWoken);
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                    buf.length = 0; // Buf reset
                    break;
                }
            }
        }
    }
};
}   // namespace UART_STREAMER

#endif // UARTSTREAMER_HPP