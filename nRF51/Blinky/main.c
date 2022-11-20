#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define LED_PIN          21
#define LED_PIN_MASK     1 << LED_PIN

int main()
{
    nrf_gpio_cfg_output(LED_PIN);

    while(true)
    {
        uint32_t gpio_state = NRF_GPIO->OUT;
        if(gpio_state & LED_PIN_MASK)
            NRF_GPIO->OUTCLR = (LED_PIN_MASK & gpio_state); 
        else
            NRF_GPIO->OUTSET = (LED_PIN_MASK & ~gpio_state);

        nrf_delay_ms(100);
    }
}
