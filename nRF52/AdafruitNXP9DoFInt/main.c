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
#include "nrf_drv_gpiote.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_uart.h"
#include "nrf_drv_twi.h"

#define UART_BUFFER_SIZE           256 // Buffer size for UART data
#define TWI_INSTANCE_ID              0 // Two wire interface instance ID

#define FXOS8700CQ_ADDR           0x1F
#define FXOS8700_WHO_AM_I_VAL     0xC7
#define FXOS8700_REG_STATUS       0x00
#define FXOS8700_REG_OUT_X_MSB    0x01
#define FXOS8700_REG_OUT_X_LSB    0x02
#define FXOS8700_REG_OUT_Y_MSB    0x03
#define FXOS8700_REG_OUT_Y_LSB    0x04
#define FXOS8700_REG_OUT_Z_MSB    0x05
#define FXOS8700_REG_OUT_Z_LSB    0x06
#define FXOS8700_REG_WHO_AM_I     0x0D
#define FXOS8700_REG_XYZ_DATA_CFG 0x0E
#define FXOS8700_REG_CTRL_REG1    0x2A
#define FXOS8700_REG_CTRL_REG2    0x2B
#define FXOS8700_REG_CTRL_REG3    0x2C
#define FXOS8700_REG_CTRL_REG4    0x2D
#define FXOS8700_REG_CTRL_REG5    0x2E
#define FXOS8700_REG_M_DR_STATUS  0x32
#define FXOS8700_REG_M_OUT_X_MSB  0x33
#define FXOS8700_REG_M_OUT_X_LSB  0x34
#define FXOS8700_REG_M_OUT_Y_MSB  0x35
#define FXOS8700_REG_M_OUT_Y_LSB  0x36
#define FXOS8700_REG_M_OUT_Z_MSB  0x37
#define FXOS8700_REG_M_OUT_Z_LSB  0x38
#define FXOS8700_REG_M_CTRL_REG1  0x5B
#define FXOS8700_REG_M_CTRL_REG2  0x5C

#define FXAS21002C_ADDR           0x21
#define FXAS21002C_WHO_AM_I_VAL   0xD7
#define FXAS21002C_REG_STATUS     0x00
#define FXAS21002C_REG_OUT_X_MSB  0x01
#define FXAS21002C_REG_OUT_X_LSB  0x02
#define FXAS21002C_REG_OUT_Y_MSB  0x03
#define FXAS21002C_REG_OUT_Y_LSB  0x04
#define FXAS21002C_REG_OUT_Z_MSB  0x05
#define FXAS21002C_REG_OUT_Z_LSB  0x06
#define FXAS21002C_REG_DR_STATUS  0x07
#define FXAS21002C_REG_INT_SRC    0x0B
#define FXAS21002C_REG_WHO_AM_I   0x0C
#define FXAS21002C_REG_CTRL_REG0  0x0D
#define FXAS21002C_REG_CTRL_REG1  0x13
#define FXAS21002C_REG_CTRL_REG2  0x14

#define ONE_G_IN_LSB          16384.0f
#define MICRO_TESLA_PER_LSB       0.1f
#define MILLI_DEGREES_PER_LSB  7.8125f

#define GYRO_INTERRUPT_PIN           2
#define ACCEL_MAG_INTERRUPT_PIN      3

#define GYRO_LED             BSP_LED_0
#define ACCEL_MAG_LED        BSP_LED_1

// Stores the accelerometer/magnometer data
typedef struct AccelMagData 
{
   uint8_t astatus;
   uint8_t mstatus;
   float ax;
   float ay;
   float az;
   float mx;
   float my;
   float mz;
} AccelMagData;

// Stores the gyroscope data
typedef struct GyroData
{
   uint8_t status;
   float x;
   float y;
   float z;
} GyroData;

volatile static AccelMagData g_AccelMagData;
volatile static GyroData g_GyroData;

volatile static uint32_t g_AccelMagIntCount = 0;
volatile static uint32_t g_GyroIntCount = 0;

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

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

void TWIHandler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    // Empty TWI handler, nrf_drv_twi_init() seems to want one even if it does nothing
}

uint8_t I2CReadByte(uint8_t slaveAddress, uint8_t regAddress)
{
    ret_code_t err_code;

    err_code = nrf_drv_twi_tx(&m_twi, slaveAddress, &regAddress, 1, true);
    if(err_code != NRF_SUCCESS)
    {
        printf("I2CReadByte transfer FAILED.  ErrorCode:%d SlaveAddress:0x%x RegAddress:0x%x\r\n",
                err_code, slaveAddress, regAddress);
        while(1);
    }

    nrf_delay_us(250);

    uint8_t result;
    err_code = nrf_drv_twi_rx(&m_twi, slaveAddress, &result, 1);
    if(err_code != NRF_SUCCESS)
    {
        printf("I2CReadByte receive FAILED.  ErrorCode:%d SlaveAddress:0x%x RegAddress:0x%x\r\n",
                err_code, slaveAddress, regAddress);
        while(1);
    }

    nrf_delay_us(250);

    return result;
}

void I2CWriteByte(uint8_t slaveAddress, uint8_t regAddress, uint8_t data)
{
    uint8_t dataBuffer[2];
    dataBuffer[0] = regAddress;
    dataBuffer[1] = data;
    
    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, slaveAddress, dataBuffer, 2, true);
    if(err_code != NRF_SUCCESS)
    {
        printf("I2CWriteByte FAILED.  ErrorCode:%d SlaveAddress:0x%x RegAddress:0x%x  Data:0x%x\r\n",
                err_code, slaveAddress, regAddress, data);
        while(1);
    }

    nrf_delay_us(500);
}

void InitUART()
{
    uint32_t err_code;

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
}

void InitTWI()
{
    nrf_drv_twi_config_t twi_lm75b_config;
    twi_lm75b_config.clear_bus_init = false;
    twi_lm75b_config.frequency = NRF_DRV_TWI_FREQ_400K;
    twi_lm75b_config.hold_bus_uninit = NRF_DRV_TWI_FREQ_400K;
    twi_lm75b_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;
    twi_lm75b_config.scl = ARDUINO_SCL_PIN;
    twi_lm75b_config.sda = ARDUINO_SDA_PIN;
    
    uint32_t err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, TWIHandler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

void GetGryoData()
{
    // Read the Gyroscope status and x,y,z values
    g_GyroData.status = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_STATUS);
    uint8_t xMSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_X_MSB);
    uint8_t xLSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_X_LSB);
    uint8_t yMSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_Y_MSB);
    uint8_t yLSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_Y_LSB);
    uint8_t zMSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_Z_MSB);
    uint8_t zLSB = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_OUT_Z_LSB);

    // Convert MSB/LSB values into meaningful data
    g_GyroData.x = (int16_t)((xMSB << 8) | xLSB) * MILLI_DEGREES_PER_LSB;
    g_GyroData.y = (int16_t)((yMSB << 8) | yLSB) * MILLI_DEGREES_PER_LSB;
    g_GyroData.z = (int16_t)((zMSB << 8) | zLSB) * MILLI_DEGREES_PER_LSB;
}

void GetAccelMagData()
{
    // Read the accelerometer and magnometer status and x,y,z values
    g_AccelMagData.astatus = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_STATUS);
    uint8_t axMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_X_MSB);
    uint8_t axLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_X_LSB);
    uint8_t ayMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_Y_MSB);
    uint8_t ayLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_Y_LSB);
    uint8_t azMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_Z_MSB);
    uint8_t azLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_OUT_Z_LSB);            
    g_AccelMagData.mstatus = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_DR_STATUS);
    uint8_t mxMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_X_MSB);
    uint8_t mxLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_X_LSB);
    uint8_t myMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_Y_MSB);
    uint8_t myLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_Y_LSB);
    uint8_t mzMSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_Z_MSB);
    uint8_t mzLSB = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_OUT_Z_LSB);

    // Convert accelerometer MSB/LSB values into meaningful data.
    // Note that the accelerometer data is only 14 bits of precision.  The low 6 bits 
    // are in bits 2-to-7.  The value is signed, so we just put into a signed 16 bit
    // so the sign conversion to signed data is easy.
    g_AccelMagData.ax = (int16_t)((axMSB << 8) | axLSB) / ONE_G_IN_LSB;
    g_AccelMagData.ay = (int16_t)((ayMSB << 8) | ayLSB) / ONE_G_IN_LSB;
    g_AccelMagData.az = (int16_t)((azMSB << 8) | azLSB) / ONE_G_IN_LSB;

    // Convert magnometer MSB/LSB values into meaningful data.
    g_AccelMagData.mx  = (int16_t)((mxMSB << 8) | mxLSB) * MICRO_TESLA_PER_LSB;
    g_AccelMagData.my  = (int16_t)((mxMSB << 8) | myLSB) * MICRO_TESLA_PER_LSB;
    g_AccelMagData.mz  = (int16_t)((mzMSB << 8) | mzLSB) * MICRO_TESLA_PER_LSB;
}

void DataReadyInterruptHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{    
    if(pin == GYRO_INTERRUPT_PIN)
    {     
        nrf_gpio_pin_toggle(GYRO_LED); // Toggle LED to show interrupt still being called
        GetGryoData();
        ++g_GyroIntCount;
    }

    if(pin == ACCEL_MAG_INTERRUPT_PIN)
    {     
        nrf_gpio_pin_toggle(ACCEL_MAG_LED); // Toggle LED to show interrupt still being called
        GetAccelMagData();
        ++g_AccelMagIntCount;
    }
}

void SetupInterruptPin(nrfx_gpiote_pin_t pin)
{
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;    
    
    ret_code_t err_code = nrf_drv_gpiote_in_init(pin, &in_config, DataReadyInterruptHandler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(pin, true);
}

void InitGPIOInterrupts()
{
    ret_code_t err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    SetupInterruptPin(GYRO_INTERRUPT_PIN);
    SetupInterruptPin(ACCEL_MAG_INTERRUPT_PIN);
}

void InitLED()
{
    bsp_board_init(BSP_INIT_LEDS);
}

void InitFXOS8700CQ()
{
    // Make sure the accelerometer/magnometer is what we think it is
    uint8_t whoAmI = I2CReadByte(FXOS8700CQ_ADDR, FXOS8700_REG_WHO_AM_I);
    if(whoAmI != FXOS8700_WHO_AM_I_VAL)
    {        
        printf("Initialization of Accel/Mag(FXOS8700CQ) FAILED.  Unexpected 'who am i' value: 0x%x\r\n", whoAmI);
        while(1);
    }

    // CTRL_REG1 (0x2A) - Accelerometer control register
    // Bit 0:     0 (Set to standby mode while we program it)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG1, 0x00);
    
    // XYZ_DATA_CFG (0x0E)
    // No need for the following line of code.  By default it's set to all 
    // zeroes at reset which means +/-2G accelerometer range and no high 
    // pass filter.
    //I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REGISTER_XYZ_DATA_CFG, 0x00);

    // CTRL_REG2 (0x2B) - Accelerometer control register
    // Bit 7:    0 (Self test disabled)
    // Bit 6:    1 (Device reset disabled)  // First call
    // Bit 5:    - (Unused)
    // Bit 4-3: 00 (Normal power mode)
    // Bit 2:    0 (Sleep mode disabled)
    // Bit 1-0: 10 (High resolution)       // Second call (after reset)
    // Reset the device and wait a moment.  The datasheet says to wait one millisecond 
    // between issuing a reset and performing more communication.  I wait 5 just to 
    // play it safe.
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG2, 0x40);
    nrf_delay_ms(5);
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG2, 0x02);

    // CTRL_REG3 (0x2C) - Accelerometer control register
    // Bit 0: 1 (INT1/INT2 set to open-drain output mode)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG3, 0x01);

    // CTRL_REG4 (0x2D) - Accelerometer control register
    // Bit 0: 1 (Data ready interrupt enabled)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG4, 0x01);

    // CTRL_REG5 (0x2E) - Accelerometer control register
    // Bit 0: 1 (Interrupt is routed to INT1 pin)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG5, 0x01);

    // CTRL_REG1 (0x2A) - Accelerometer control register
    // Bit 7-6:  00 (50Hz sleep mode output data rate)
    // Bit 5-3: 101 (6.25Hz output data rate (note below we're in hybrid mode))
    // Bit 2:     1 (Reduced noise mode)
    // Bit 1:     0 (Normal I2C read mode 100kbit/s)
    // Bit 0:     1 (Change from standby to active)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG1, 0x2D);

    // MCTRL_REG1 (0x5B) - Magnetometer control register
    // Bit 7:     0 (Auto-calibration feature is disabled)
    // Bit 6:     0 (No one-shot magnetic reset)
    // Bit 5:     0 (No action taken when one-shot trigger)
    // Bit 4-2: 111 (Oversample ratio for magnetometer data is set to 256)
    // Bit 1-0:  11 (Hybrid mode, both accelerometer and magnetometer sensors are active)    
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_CTRL_REG1, 0x1F);

    // MCTRL_REG2 (0x5C) - Magnetometer control register
    // Bit 7-6: -- (Unused)
    // Bit   5:  0 (Not using hybrid auto increment feature)
    // Bit   4:  0 (Magnetic min/max detection function is enabled)
    // Bit   3:  0 (No impact to magnetic min/max detection function on a magnetic threshold event)
    // Bit   2:  0 (No reset sequence is active) 
    // Bit 1-0: 00 (Automatic magnetic reset at the beginning of each ODR cycle)
    // No need to send the 0x00 since it's set this way by default
    //I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_M_CTRL_REG2, 0x00);    

    // Wait briefly after configuration
    nrf_delay_ms(100);

    printf("Accel/Mag(FXOS8700CQ) Initialization Successful\r\n");
}

void InitFXAS21002C()
{
    // Make sure the gyroscope is what we think it is
    uint8_t whoAmI = I2CReadByte(FXAS21002C_ADDR, FXAS21002C_REG_WHO_AM_I);
    if(whoAmI != FXAS21002C_WHO_AM_I_VAL)
    {        
        printf("Initialization of Gyro(FXA21002C) FAILED.  Unexpected 'who am i' value: 0x%x\r\n", whoAmI);
        while(1);
    }

    // CTRL_REG1 (0x13) - Gyroscope control register
    // Bit 0-1: 00 (Place in standby mode while we program it)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG1, 0x00);
  
    // CTRL_REG1 (0x13) - Gyroscope control register 1
    // Bit 7:     - (Unused)
    // Bit 6:     1 (Reset the device)
    // Bit 5:     0 (Self test disabled)
    // Bit 4-2: 000 (800 Hz output data rate)
    // Bit 0-1:  00 (Still in standby mode while we program it)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG1, 0x40);

    // CTRL_REG0 (0x0D) - Gyroscope control register 0
    // Bit 7-6: 00 (Don't think we really care about the lowpass cutoff frequency)
    // Bit   5:  0 (Shouldn't matter, not using SPI)
    // Bit 4-3: 00 (High pass filter cutoff doesn't matter, it's disabled)
    // Bit   2:  0 (High pass filter disabled)
    // Bit 1-0: 11 (Range is set to +/- 250 degrees per second) 
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG0, 0x03);

    // INT_SOURCE_FLAG (0x0B) - Gyroscope interrupt source flag
    // Bit 7-4: - (Unused)
    // Bit 3:   0 (Don't interrupt on boot sequence)
    // Bit 2:   0 (Don't interrupt on FIFO event)    
    // Bit 1:   0 (Don't interrupt on rate threshold)
    // Bit 0:   1 (Interrupt on data-ready event)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_INT_SRC, 0x01);

    // CTRL_REG2 (0x14) - Gyroscope control register 2    
    // Bit 7: 0 (Don't care, not using this interrupt)
    // Bit 6: 0 (FIFO inerrupt disabled)
    // Bit 5: 0 (Don't care, not using this interrupt)
    // Bit 4: 0 (Rate threshold interrupt disabled)
    // Bit 3: 1 (Interrupt is routed to INT1 pin)
    // Bit 2: 1 (Data-ready interrupt enable)    
    // Bit 1: 0 (Interrupt logic polarity active low)
    // Bit 0: 1 (Push/pull output driver)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG2, 0x0D);

    // CTRL_REG1 (0x13) - Gyroscope control register 1
    // Bit 7:     - (Unused)
    // Bit 6:     0 (Don't reset the device)
    // Bit 5:     0 (Self test disabled)
    // Bit 4-2: 111 (12.5 Hz output data rate)
    // Bit 0-1:  10 (Move from stanby mode to active)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG1, 0x1E);

    // Wait a moment after configuring
    nrf_delay_ms(100);

    printf("Gyro(FXAS21002C) Initialization Successful\r\n");
}

void PrintStartupReset()
{
    printf("=================================================================\r\n");
    printf("                        STARTUP/RESET\r\n");
    printf("=================================================================\r\n");
}

void main()
{
    InitLED();            // Setup LED
    InitUART();           // Setup UART
    PrintStartupReset();  // Display to UART terminal that startup/reset occurred
    InitTWI();            // Setup the two wire interface
    InitGPIOInterrupts(); // Setup interrupt pins for the Gyroscope and Accelerometer/Magnometer
    InitFXAS21002C();     // Setup the gyroscope  
    InitFXOS8700CQ();     // Setup the accelerometer/magnometer
   
    uint32_t PrevAccelMagIntCount = 0;
    uint32_t PrevGyroIntCount = 0;

    while(1)
    {
        if(PrevAccelMagIntCount != g_AccelMagIntCount || PrevGyroIntCount != g_GyroIntCount)        
        {
            printf("AMCnt:%3d GCnt:%d ", g_AccelMagIntCount, g_GyroIntCount);
            printf("Stat:0x%02x/0x%02x/0x%02x ", g_AccelMagData.astatus, g_AccelMagData.mstatus, g_GyroData.status);
            printf("A:% 1.2f % 1.2f % 1.2f ", g_AccelMagData.ax, g_AccelMagData.ay, g_AccelMagData.az);
            printf("M:% 5.1f % 5.1f % 5.1f ", g_AccelMagData.mx, g_AccelMagData.my, g_AccelMagData.mz);
            printf("G:% 3.2f % 3.2f % 3.2f\r\n", g_GyroData.x/1000.0f, g_GyroData.y/1000.0f, g_GyroData.z/1000.0f);
        }

        PrevAccelMagIntCount = g_AccelMagIntCount;
        PrevGyroIntCount = g_GyroIntCount;
    }
}