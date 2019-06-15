#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "Adafruit_SSD1306.h"
#include "splash.h"

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT        64
#define TEXT_SIZE             2

#define PAUSE_AFTER_DELAY  2000
#define PAUSE_AFTER_CHAR    100

#define STARTING_CHAR        48  // Start with char '0'
#define CHARS_TO_DISPLAY     40

void DisplayAdafruitLogo(Adafruit_SSD1306& Display);

int main(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Starting...");
    NRF_LOG_FLUSH();

    Adafruit_SSD1306 Display(SCREEN_WIDTH, SCREEN_HEIGHT, TEXT_SIZE);
    uint16_t count = 0;

    // Display the logo at startup
    DisplayAdafruitLogo(Display);
    nrf_delay_ms(PAUSE_AFTER_DELAY);
    Display.Clear();
    Display.SetCursor(0, 0);

    while (1)
    {
        // Display the new character
        Display.Write(STARTING_CHAR + count);
        Display.Display();
        nrf_delay_ms(PAUSE_AFTER_CHAR);

        if(count++ == CHARS_TO_DISPLAY) 
        {
            // Pause while displaying all cursors
            nrf_delay_ms(PAUSE_AFTER_DELAY);

            // Display the logo again
            DisplayAdafruitLogo(Display);
            nrf_delay_ms(PAUSE_AFTER_DELAY);

            NRF_LOG_INFO("Looping...");
            NRF_LOG_FLUSH();

            // Clear the logo and reset cursor position
            Display.Clear();
            Display.SetCursor(0, 0);

            count = 0;
        }        
    }
}

void DisplayAdafruitLogo(Adafruit_SSD1306& Display)
{
    Display.Clear();
    uint16_t upperLeftX = (SCREEN_WIDTH - splash_width) / 2;
    uint16_t upperLeftY = (SCREEN_HEIGHT - splash_height) / 2;
    Display.DrawBitmap(upperLeftX, upperLeftY, splash_data, splash_width, splash_height);
    Display.Display();
}
