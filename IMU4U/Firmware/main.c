#include <stdint.h>

#include "app_button.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh_ble.h"
#include "sdk_common.h"

#include "IMU.h"

#define CONNECTED_LED                   BSP_BOARD_LED_0                         // Is on when device has connected.
#define LEDBUTTON_LED                   BSP_BOARD_LED_1                         // LED to be toggled with the help of the LED Button Service.
#define LEDBUTTON_BUTTON                BSP_BUTTON_0                            // Button that will trigger the notification event with the LED Button Service

#define DEVICE_NAME                     "TMD_IMU4U"                             // Name of device. Will be included in the advertising data.

#define APP_BLE_OBSERVER_PRIO           3                                       // Application's BLE observer priority. Nordic says you shouldn't need to modify this value.
#define APP_BLE_CONN_CFG_TAG            1                                       // A tag identifying the SoftDevice BLE configuration.

#define APP_ADV_INTERVAL                64                                      // The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms).
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   // The advertising time-out (in units of seconds). When set to 0, we will never time out.

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        // Minimum acceptable connection interval (0.5 seconds).
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        // Maximum acceptable connection interval (1 second).
#define SLAVE_LATENCY                   0                                       // Slave latency.
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         // Connection supervisory time-out (4 seconds).

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  // Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds).
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   // Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds).
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       // Number of attempts before giving up the connection parameter negotiation.

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     // Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks).

#define DEAD_BEEF                       0xDEADBEEF                              // Value used as error code on stack dump, can be used to identify stack location on stack unwind.

#define SEND_IMU_DATA_FREQUENCY        5                                        // The timer checking for display update occurs every 100 milliseconds
#define SEND_IMU_DATA_TIME_MS          1000/SEND_IMU_DATA_FREQUENCY             // The timer checking for display update occurs every 100 milliseconds

static uint16_t gConnHandle = BLE_CONN_HANDLE_INVALID;                          // Handle of the current connection.

static uint8_t gAdvHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                     // Advertising handle used to identify an advertising set.
static uint8_t gEncAdvData[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                      // Buffer for storing an encoded advertising set.
static uint8_t gEncScanData[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                     // Buffer for storing an encoded scan data.

// Struct that contains pointers to the encoded advertising data.
static ble_gap_adv_data_t gAdvData =
{
    .adv_data =
    {
        .p_data = gEncAdvData,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = gEncScanData,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    }
};

// Our UUID's
#define IMU4U_UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define IMU4U_UUID_SERVICE     0x1523
#define IMU4U_UUID_BUTTON_CHAR 0x1524
#define IMU4U_UUID_LED_CHAR    0x1525
#define IMU4U_UUID_IMU_CHAR    0x1526

// Contains the most current IMU data
static IMUData gCurrIMUData;

// Various forward declarations
void InitLog();
void InitLED();
void InitTimers();
void InitButtons();
void InitPowerMgmt();
void IdleStateHandler();
void InitBLEStack();
void InitConnectionParams();
void InitGATT();
void InitGAPParams();
void InitServices();
void InitAdvertising();
void TimerHandler(void* pContext);
void SendIMUState();
void CheckButtonState();
void StartAdvertising();
void StartIMUTimer();
void IMUCallback(const IMUData* pIMUData);


typedef struct IMU4UServiceStruct IMU4UServiceStruct;
typedef void (*IMU4UWriteHandler) (uint16_t connHandle, IMU4UServiceStruct* pIMU4U, uint8_t newState);
typedef struct IMU4UInitStruct
{
    IMU4UWriteHandler LEDWriteHandler; // Event handler to be called when the LED Characteristic is written.
} IMU4UInitStruct;

struct IMU4UServiceStruct  // Service structure. This structure contains various status information for the service.
{
    uint16_t                  ServiceHandle;    // Handle of LED Button Service (as provided by the BLE stack).
    ble_gatts_char_handles_t  LEDCharHandle;    // Handles related to the LED Characteristic.
    ble_gatts_char_handles_t  ButtonCharHandle; // Handles related to the Button Characteristic.
    ble_gatts_char_handles_t  IMUCharHandle;    // Handles related to the IMU Characteristic.
    uint8_t                   UUIDType;         // UUID type for the LED Button Service.
    IMU4UWriteHandler         LEDWriteHandler;  // Event handler to be called when the LED Characteristic is written.
};

int main(void)
{
    InitLog();
    InitLED();
    InitIMU(IMUCallback);
    InitTimers();
    InitButtons();
    InitPowerMgmt();
    InitBLEStack();
    InitGAPParams();
    InitGATT();
    InitServices();
    InitAdvertising();
    InitConnectionParams();
    
    StartAdvertising();
    StartIMU();
    StartIMUTimer();

    while(1)
    {
        IdleStateHandler();
    }
}

void BLEEventHandler(ble_evt_t const* pEvent, void* pContext)
{
    IMU4UServiceStruct* pIMU = (IMU4UServiceStruct*)pContext;

    switch (pEvent->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            {
                ble_gatts_evt_write_t const* pWriteEvent = &pEvent->evt.gatts_evt.params.write;

                if((pWriteEvent->handle == pIMU->LEDCharHandle.value_handle) &&
                    (pWriteEvent->len == 1) &&
                    (pIMU->LEDWriteHandler != NULL))
                {
                    pIMU->LEDWriteHandler(pEvent->evt.gap_evt.conn_handle, pIMU, pWriteEvent->data[0]);
                }
            }
            break;
        default:
            break;
    }
}

#define BLE_IMU4U_SERVICE(_name)                                                                    \
static IMU4UServiceStruct _name;                                                                    \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_LBS_BLE_OBSERVER_PRIO,                                                     \
                     BLEEventHandler, &_name)


void InitIMUService(IMU4UServiceStruct* pService, const IMU4UInitStruct* pInit)
{
    ret_code_t errCode;    

    pService->LEDWriteHandler = pInit->LEDWriteHandler;

    // Add service.
    ble_uuid128_t baseUUID = {IMU4U_UUID_BASE};
    errCode = sd_ble_uuid_vs_add(&baseUUID, &pService->UUIDType);
    VERIFY_SUCCESS(errCode);

    ble_uuid_t bleUUID;
    bleUUID.type = pService->UUIDType;
    bleUUID.uuid = IMU4U_UUID_SERVICE;

    errCode = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &bleUUID, &pService->ServiceHandle);
    VERIFY_SUCCESS(errCode);

    // Add Button characteristic.
    ble_add_char_params_t newChar;
    memset(&newChar, 0, sizeof(newChar));
    newChar.uuid              = IMU4U_UUID_BUTTON_CHAR;
    newChar.uuid_type         = pService->UUIDType;
    newChar.init_len          = sizeof(uint8_t);
    newChar.max_len           = sizeof(uint8_t);
    newChar.char_props.read   = 1;
    newChar.char_props.notify = 1;
    newChar.read_access       = SEC_OPEN;
    newChar.cccd_write_access = SEC_OPEN;

    errCode = characteristic_add(pService->ServiceHandle, &newChar, &pService->ButtonCharHandle);
    VERIFY_SUCCESS(errCode);

    // Add LED characteristic.
    memset(&newChar, 0, sizeof(newChar));
    newChar.uuid             = IMU4U_UUID_LED_CHAR;
    newChar.uuid_type        = pService->UUIDType;
    newChar.init_len         = sizeof(uint8_t);
    newChar.max_len          = sizeof(uint8_t);
    newChar.char_props.read  = 1;
    newChar.char_props.write = 1;
    newChar.read_access      = SEC_OPEN;
    newChar.write_access     = SEC_OPEN;

    errCode = characteristic_add(pService->ServiceHandle, &newChar, &pService->LEDCharHandle);
    VERIFY_SUCCESS(errCode);
     
    // Add IMU characteristic.
    memset(&newChar, 0, sizeof(newChar));
    newChar.uuid              = IMU4U_UUID_IMU_CHAR;
    newChar.uuid_type         = pService->UUIDType;
    newChar.init_len          = sizeof(IMUData);
    newChar.max_len           = sizeof(IMUData);
    newChar.char_props.read   = 1;
    newChar.char_props.notify = 1;
    newChar.read_access       = SEC_OPEN;
    newChar.cccd_write_access = SEC_OPEN;

    errCode = characteristic_add(pService->ServiceHandle, &newChar, &pService->IMUCharHandle);
    VERIFY_SUCCESS(errCode);
}

void InitLED()
{
    bsp_board_init(BSP_INIT_LEDS);
}

APP_TIMER_DEF(gTimerID);
void InitTimers()
{
    // Initialize timer module, making it use the scheduler
    ret_code_t errCode = app_timer_init();
    APP_ERROR_CHECK(errCode);

    // Create the timer for collecting and sending IMU Data
    errCode = app_timer_create(&gTimerID, APP_TIMER_MODE_REPEATED, TimerHandler);
    APP_ERROR_CHECK(errCode);    
}


void StartIMUTimer()
{
    // Start the timer for collecting and sending IMU Data
    ret_code_t errCode = app_timer_start(gTimerID, APP_TIMER_TICKS(SEND_IMU_DATA_TIME_MS), NULL);    
    APP_ERROR_CHECK(errCode);   
}

void InitGAPParams()
{
    ret_code_t              errCode;
    ble_gap_conn_params_t   gapConnParams;
    ble_gap_conn_sec_mode_t secMode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);

    errCode = sd_ble_gap_device_name_set(&secMode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(errCode);

    memset(&gapConnParams, 0, sizeof(gapConnParams));

    gapConnParams.min_conn_interval = MIN_CONN_INTERVAL;
    gapConnParams.max_conn_interval = MAX_CONN_INTERVAL;
    gapConnParams.slave_latency     = SLAVE_LATENCY;
    gapConnParams.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    errCode = sd_ble_gap_ppcp_set(&gapConnParams);
    APP_ERROR_CHECK(errCode);
}

BLE_IMU4U_SERVICE(gIMU4UService);  // IMU4U Service instance
NRF_BLE_GATT_DEF(gGATT);           // GATT module instance
NRF_BLE_QWR_DEF(gQWR);             // Context for the Queued Write module

void InitGATT()
{
    ret_code_t errCode = nrf_ble_gatt_init(&gGATT, NULL);
    APP_ERROR_CHECK(errCode);
}

void InitAdvertising()
{
    ret_code_t    errCode;
    ble_advdata_t advdata;
    ble_advdata_t srdata;

    ble_uuid_t advUUIDs[] = {{IMU4U_UUID_SERVICE, gIMU4UService.UUIDType}};

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(advUUIDs) / sizeof(advUUIDs[0]);
    srdata.uuids_complete.p_uuids  = advUUIDs;

    errCode = ble_advdata_encode(&advdata, gAdvData.adv_data.p_data, &gAdvData.adv_data.len);
    APP_ERROR_CHECK(errCode);

    errCode = ble_advdata_encode(&srdata, gAdvData.scan_rsp_data.p_data, &gAdvData.scan_rsp_data.len);
    APP_ERROR_CHECK(errCode);

    ble_gap_adv_params_t advParams;

    // Set advertising parameters.
    memset(&advParams, 0, sizeof(advParams));

    advParams.primary_phy     = BLE_GAP_PHY_1MBPS;
    advParams.duration        = APP_ADV_DURATION;
    advParams.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    advParams.p_peer_addr     = NULL;
    advParams.filter_policy   = BLE_GAP_ADV_FP_ANY;
    advParams.interval        = APP_ADV_INTERVAL;

    errCode = sd_ble_gap_adv_set_configure(&gAdvHandle, &gAdvData, &advParams);
    APP_ERROR_CHECK(errCode);
}

static void QWRErrorHandler(uint32_t nrfError)
{
    APP_ERROR_HANDLER(nrfError);
}

static void LEDWriteHandler(uint16_t connHandle, IMU4UServiceStruct* pService, uint8_t ledState)
{
    if (ledState)
    {
        bsp_board_led_on(LEDBUTTON_LED);
        NRF_LOG_INFO("LED on received");
    }
    else
    {
        bsp_board_led_off(LEDBUTTON_LED);
        NRF_LOG_INFO("LED off received");
    }
}

void InitServices()
{
    ret_code_t         errCode;
    IMU4UInitStruct    ConnErrorHandler = {0};
    nrf_ble_qwr_init_t qwrInit  = {0};

    // Initialize Queued Write Module.
    qwrInit.error_handler = QWRErrorHandler;

    errCode = nrf_ble_qwr_init(&gQWR, &qwrInit);
    APP_ERROR_CHECK(errCode);

    // Initialize the service
    ConnErrorHandler.LEDWriteHandler = LEDWriteHandler;
    InitIMUService(&gIMU4UService, &ConnErrorHandler);    
}

static void ConnEventHandler(ble_conn_params_evt_t* pEvent)
{
    ret_code_t errCode;

    if (pEvent->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        errCode = sd_ble_gap_disconnect(gConnHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(errCode);
    }
}

static void ConnErrorHandler(uint32_t nrfError)
{
    APP_ERROR_HANDLER(nrfError);
}

void InitConnectionParams()
{
    ret_code_t             errCode;
    ble_conn_params_init_t connInit;

    memset(&connInit, 0, sizeof(connInit));

    connInit.p_conn_params                  = NULL;
    connInit.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    connInit.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    connInit.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    connInit.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    connInit.disconnect_on_fail             = false;
    connInit.evt_handler                    = ConnEventHandler;
    connInit.error_handler                  = ConnErrorHandler;

    errCode = ble_conn_params_init(&connInit);
    APP_ERROR_CHECK(errCode);
}


void StartAdvertising()
{
    ret_code_t errCode;

    errCode = sd_ble_gap_adv_start(gAdvHandle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(errCode);
}

static void ble_evt_handler(ble_evt_t const* pEvent, void* pContect)
{
    ret_code_t errCode;

    switch (pEvent->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            bsp_board_led_on(CONNECTED_LED);            
            gConnHandle = pEvent->evt.gap_evt.conn_handle;
            errCode = nrf_ble_qwr_conn_handle_assign(&gQWR, gConnHandle);
            APP_ERROR_CHECK(errCode);
            errCode = app_button_enable();
            APP_ERROR_CHECK(errCode);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            bsp_board_led_off(CONNECTED_LED);
            gConnHandle = BLE_CONN_HANDLE_INVALID;
            errCode = app_button_disable();
            APP_ERROR_CHECK(errCode);
            StartAdvertising();
            break;
        default:            
            break;
    }
}

void InitBLEStack()
{
    ret_code_t errCode;

    errCode = nrf_sdh_enable_request();
    APP_ERROR_CHECK(errCode);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ramStart = 0;
    errCode = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ramStart);
    APP_ERROR_CHECK(errCode);

    // Enable BLE stack.
    errCode = nrf_sdh_ble_enable(&ramStart);
    APP_ERROR_CHECK(errCode);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

void InitButtons()
{
    ret_code_t errCode;

    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] = {{LEDBUTTON_BUTTON, false, BUTTON_PULL}};

    errCode = app_button_init(buttons, ARRAY_SIZE(buttons), BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(errCode);
}

void InitLog()
{
    ret_code_t errCode = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(errCode);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void InitPowerMgmt()
{
    ret_code_t errCode;
    errCode = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(errCode);
}

void IdleStateHandler()
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

void IMUCallback(const IMUData* pIMUData)
{
    gCurrIMUData = *pIMUData;
}

void TimerHandler(void* pContext)
{   
    SendIMUState();
    CheckButtonState();
}

void SendIMUState()
{
    ble_gatts_hvx_params_t params;
    uint16_t length = sizeof(IMUData);

    memset(&params, 0, sizeof(params));
    params.type   = BLE_GATT_HVX_NOTIFICATION;
    params.handle = gIMU4UService.IMUCharHandle.value_handle;
    params.p_data = (uint8_t*)(&gCurrIMUData);
    params.p_len  = &length;

    sd_ble_gatts_hvx(gConnHandle, &params); 
}

void CheckButtonState()
{
    ble_gatts_hvx_params_t params;
    uint8_t buttonState= 1;
    uint16_t length = sizeof(buttonState);

    static bool PrevButtonDown = false;
    bool ButtonDown = !nrf_gpio_pin_read(LEDBUTTON_BUTTON);
    if(ButtonDown != PrevButtonDown)
    {
        if(ButtonDown)
        { 
            NRF_LOG_INFO("Button down.");
        }
        else
        {
            NRF_LOG_INFO("Button up.");       
        }

        buttonState = ButtonDown ? 1 : 0;
        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = gIMU4UService.ButtonCharHandle.value_handle;
        params.p_data = &buttonState;
        params.p_len  = &length;

        sd_ble_gatts_hvx(gConnHandle, &params);
    }

    PrevButtonDown = ButtonDown;
}
