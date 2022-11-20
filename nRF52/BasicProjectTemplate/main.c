/*
* Copyright (c) 2020 Terence M. Darwen - tmdarwen.com
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy 
* of this software and associated documentation files (the "Software"), to deal 
* in the Software without restriction, including without limitation the rights 
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
* copies of the Software, and to permit persons to whom the Software is furnished 
* to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
* IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_uart.h"

#define UART_BUFFER_SIZE 256

void UARTErrorHandler(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

int main(void)
{
    uint32_t err_code;

    bsp_board_init(BSP_INIT_LEDS);
    
    app_uart_comm_params_t comm_params;
    comm_params.baud_rate = NRF_UART_BAUDRATE_115200;
    comm_params.cts_pin_no = CTS_PIN_NUMBER;
    comm_params.flow_control = APP_UART_FLOW_CONTROL_DISABLED;
    comm_params.rts_pin_no = RTS_PIN_NUMBER;
    comm_params.rx_pin_no = RX_PIN_NUMBER;
    comm_params.tx_pin_no = TX_PIN_NUMBER;
    comm_params.use_parity = false;

    APP_UART_FIFO_INIT(&comm_params, 
        UART_BUFFER_SIZE, 
        UART_BUFFER_SIZE, 
        UARTErrorHandler, 
        APP_IRQ_PRIORITY_LOWEST, 
        err_code);
    APP_ERROR_CHECK(err_code);    

    uint32_t counter = 0;
    printf("\r\n");
    while (true)
    {
    
        printf("Hello World (%d)\r\n", counter);
        uint8_t cr;
        while (app_uart_get(&cr) != NRF_SUCCESS);        
        ++counter;      
        nrf_gpio_pin_toggle(BSP_LED_0);
    }
}