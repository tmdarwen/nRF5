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
#include "nrf_drv_twi.h"

#define UART_BUFFER_SIZE           256 // Buffer size for UART data
#define TWI_INSTANCE_ID              0 // Two wire interface instance ID

#define FXOS8700CQ_ADDR           0x1F
#define FXOS8700_WHO_AM_I_VAL     0xC7
#define FXOS8700_REG_WHO_AM_I     0x0D
#define FXOS8700_REG_CTRL_REG1    0x2A
#define FXOS8700_REG_CTRL_REG2    0x2B
#define FXOS8700_REG_XYZ_DATA_CFG 0x0E
#define FXOS8700_REG_MCTRL_REG1   0x5B
#define FXOS8700_REG_MCTRL_REG2   0x5C
#define FXOS8700CQ_READ_LEN         13 // Bytes to read when getting data

#define FXAS21002C_ADDR           0x21
#define FXAS21002C_REG_WHO_AM_I   0x0C
#define FXAS21002C_REG_CTRL_REG0  0x0D
#define FXAS21002C_REG_CTRL_REG1  0x13
#define FXAS21002C_WHO_AM_I_VAL   0xD7
#define FXAS21002C_READ_LEN          7 // Bytes to read when getting data

#define ONE_G_IN_LSB          16384.0f
#define MICRO_TESLA_PER_LSB       0.1f
#define MILLI_DEGREES_PER_LSB  7.8125f

// Stores the accelerometer/magnometer data
typedef struct AccelMagData 
{
   uint8_t status;
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

    nrf_delay_ms(1);

    uint8_t result;
    err_code = nrf_drv_twi_rx(&m_twi, slaveAddress, &result, 1);
    if(err_code != NRF_SUCCESS)
    {
        printf("I2CReadByte receive FAILED.  ErrorCode:%d SlaveAddress:0x%x RegAddress:0x%x\r\n",
                err_code, slaveAddress, regAddress);
        while(1);
    }

    nrf_delay_ms(2);

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

    nrf_delay_ms(2);
}

void I2CReadMultiByte(uint8_t slaveAddress, void* data, uint8_t bytes)
{
    ret_code_t err_code = nrf_drv_twi_rx(&m_twi, slaveAddress, data, bytes);
    if(err_code != NRF_SUCCESS)
    {
        printf("I2CReadMultiByte transfer FAILED.  ErrorCode:%d SlaveAddress:0x%x Bytes:%d\r\n",
                err_code, slaveAddress, bytes);
        while(1);
    }

    nrf_delay_ms(2 * bytes);
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
    twi_lm75b_config.frequency = NRF_DRV_TWI_FREQ_100K;
    twi_lm75b_config.hold_bus_uninit = NRF_DRV_TWI_FREQ_100K;
    twi_lm75b_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;
    twi_lm75b_config.scl = ARDUINO_SCL_PIN;
    twi_lm75b_config.sda = ARDUINO_SDA_PIN;
    
    uint32_t err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, TWIHandler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
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
    // Bit 0:     0 (Set to standby mode)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG1, 0x00);
    
    // XYZ_DATA_CFG (0x0E)
    // No need for the following line of code.  By default it's set to all 
    // zeroes at reset which means +/-2G accelerometer range and no high 
    // pass filter.
    //I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REGISTER_XYZ_DATA_CFG, 0x00);

    // CTRL_REG2 (0x2B) - Accelerometer control register
    // Bit 7:    0 (Self test disabled)
    // Bit 6:    0 (Device reset disabled)
    // Bit 5:    - (Unused)
    // Bit 4-3: 00 (Normal power mode)
    // Bit 2:    0 (Sleep mode disabled)
    // Bit 1-0: 10 (High resolution)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG2, 0x02);

    // CTRL_REG1 (0x2A) - Accelerometer control register
    // Bit 7-6:  00 (50Hz sleep mode output data rate)
    // Bit 5-3: 010 (200Hz output data rate)
    // Bit 2:     1 (Low noise mode)
    // Bit 1:     0 (Normal I2C read mode 100kbit/s)
    // Bit 0:     1 (Change from standby to active)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_CTRL_REG1, 0x15);

    // MCTRL_REG1 (0x5B) - Magnetometer control register
    // Bit 7:     0 (Auto-calibration feature is disabled)
    // Bit 6:     1 (One-shot magnetic reset is enabled)
    // Bit 5:     0 (No action taken when one-shot trigger)
    // Bit 4-2: 111 (Oversample ratio)
    // Bit 1-0:  11 (Hybrid mode, both accelerometer and magnetometer sensors are active)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_MCTRL_REG1, 0x1F);

    // MCTRL_REG2 (0x5C) - Magnetometer control register
    // Bit 7-6: -- (Unused)
    // Bit   5:  1 (Hybrid auto increment feature enabled)
    // Bit   4:  0 (Magnetic min/max detection function is enabled)
    // Bit   3:  0 (No impact to magnetic min/max detection function on a magnetic threshold event)
    // Bit   2:  0 (No reset sequence is active) 
    // Bit 1-0: 00 (Automatic magnetic reset at the beginning of each ODR cycle)
    I2CWriteByte(FXOS8700CQ_ADDR, FXOS8700_REG_MCTRL_REG2, 0x20);

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
  
    // CTRL_REG1 (0x13) - Gyroscope control register
    // Bit 7:     - (Unused)
    // Bit 6:     1 (Reset the device)
    // Bit 5:     0 (Self test disabled)
    // Bit 4-2: 000 (800 Hz output data rate)
    // Bit 0-1:  00 (Still in standby mode while we program it)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG1, 0x40);

    // CTRL_REG0 (0x0D) - Gyroscope control register
    // Bit 7-6: 00 (Don't think we really care about the lowpass cutoff frequency)
    // Bit   5:  0 (Shouldn't matter, not using SPI)
    // Bit 4-3: 00 (High pass filter cutoff doesn't matter, it's disabled)
    // Bit   2:  0 (High pass filter disabled)
    // Bit 1-0: 11 (Range is set to +/- 250 degrees per second) 
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG0, 0x03);

    // CTRL_REG1 (0x13) - Gyroscope control register
    // Bit 7:     - (Unused)
    // Bit 6:     0 (Don't reset the device)
    // Bit 5:     0 (Self test disabled)
    // Bit 4-2: 011 (100 Hz output data rate)
    // Bit 0-1:  10 (Move from stanby mode to active)
    I2CWriteByte(FXAS21002C_ADDR, FXAS21002C_REG_CTRL_REG1, 0x0E);

    // Wait a moment after configuring
    nrf_delay_ms(100);

    printf("Gyro(FXAS21002C) Initialization Successful\r\n");
}

AccelMagData GetAccelMagData()
{
    AccelMagData retData;
    uint8_t AMBuffer[FXOS8700CQ_READ_LEN];

    I2CReadMultiByte(FXOS8700CQ_ADDR, &AMBuffer, FXOS8700CQ_READ_LEN);

    retData.status = AMBuffer[0];

    // Note that the accelerometer data is only 14 bits of precision.  The low 6 bits 
    // are in bits 2-to-7.  The value is signed, so we just put into a signed 16 bit
    // so the sign conversion to signed data is easy.
    int16_t ax = (int16_t)((AMBuffer[1] << 8) | AMBuffer[2]);
    int16_t ay = (int16_t)((AMBuffer[3] << 8) | AMBuffer[4]);
    int16_t az = (int16_t)((AMBuffer[5] << 8) | AMBuffer[6]);

    // Convert raw data to gravitional units
    retData.ax = ax / ONE_G_IN_LSB;
    retData.ay = ay / ONE_G_IN_LSB;
    retData.az = az / ONE_G_IN_LSB;

    int16_t mx = (int16_t)((AMBuffer[7] << 8) | AMBuffer[8]);
    int16_t my = (int16_t)((AMBuffer[9] << 8) | AMBuffer[10]);
    int16_t mz = (int16_t)((AMBuffer[11] << 8) | AMBuffer[12]);

    // Convert raw data to micro Tesla's
    retData.mx = mx * MICRO_TESLA_PER_LSB;
    retData.my = my * MICRO_TESLA_PER_LSB;
    retData.mz = mz * MICRO_TESLA_PER_LSB;

    return retData;
}

GyroData GetGyroData()
{
    GyroData retData;

    uint8_t GyroBuffer[FXAS21002C_READ_LEN];        
    I2CReadMultiByte(FXAS21002C_ADDR, &GyroBuffer, FXAS21002C_READ_LEN);

    retData.status = GyroBuffer[0];

    int16_t gx = (int16_t)((GyroBuffer[1] << 8) | GyroBuffer[2]);
    int16_t gy = (int16_t)((GyroBuffer[3] << 8) | GyroBuffer[4]);
    int16_t gz = (int16_t)((GyroBuffer[5] << 8) | GyroBuffer[6]);

    // Convert raw data to milli degrees of rotation around an axis
    retData.x = gx * MILLI_DEGREES_PER_LSB;
    retData.y = gy * MILLI_DEGREES_PER_LSB;
    retData.z = gz * MILLI_DEGREES_PER_LSB;

    return retData;
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
    InitFXAS21002C();     // Setup the gyroscope  
    InitFXOS8700CQ();     // Setup the accelerometer/magnometer

    uint32_t counter = 0;
    while(1)
    {
        // Get accelerometer, magnometer and gyro data
        AccelMagData amData = GetAccelMagData();
        GyroData gData = GetGyroData();
         
        // Display the data
        printf("Iter:%3d Stat:0x%02x/0x%02x ", counter, amData.status, gData.status);
        printf("A:% 1.2f % 1.2f % 1.2f ", amData.ax, amData.ay, amData.az);
        printf("M:% 5.1f % 5.1f % 5.1f ", amData.mx, amData.my, amData.mz);
        printf("G:% 3.2f % 3.2f % 3.2f\r\n", gData.x/1000.0f, gData.y/1000.0f, gData.z/1000.0f);

        nrf_gpio_pin_toggle(BSP_LED_0);  // Blink LED to let user know we're still "alive"

        nrf_delay_ms(500);  // Pause before getting/displaying values again
        ++counter;
    }
}