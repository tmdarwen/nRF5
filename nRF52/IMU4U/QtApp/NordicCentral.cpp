#include "NordicCentral.h"
#include <string>

namespace
{
    const char*            DEVICE_NAME = "TMD_IMU4U";               // Name of the BLE device we want to connect to
    constexpr int          BLE_SCAN_TIMEOUT_MS = 5000;              // Scan timeout in milliseconds
    constexpr int          TIMER_MS = 1000;                         // TimerEvent() called every TIMER_MS milliseconds
    constexpr unsigned int LED_TOGGLE_TIME_SEC  = 2;                // Toggle the LED this frequently
    constexpr unsigned int NORDIC_BLINKY_SERVICE_UUID = 0x1523;     // Blinky service UUID
    constexpr unsigned int NORDIC_BLINKY_BUTTON_CHAR_UUID = 0x1524; // Button characteristic UUID
    constexpr unsigned int NORDIC_BLINKY_LED_CHAR_UUID = 0x1525;    // LED characteristic UUID
    constexpr unsigned int NORDIC_BLINKY_IMU_CHAR_UUID = 0x1526;    // IMU characteristic UUID
}

void NordicCentral::Start()
{
    StartTimer();
    StartDicoveryAgent();
}

bool NordicCentral::Connected()
{
    return m_bConnected;
}

bool NordicCentral::ButtonPressed()
{
    return m_bButtonPressed;
}

NordicCentral::LED_STATE NordicCentral::LEDState()
{
    return m_LEDState;
}

const IMUData& NordicCentral::IMUData()
{
    return m_IMUData;
}

void NordicCentral::StartTimer()
{
    connect(&m_timer, &QTimer::timeout, this, &NordicCentral::TimerEvent);
    m_timer.start(TIMER_MS);
}

void NordicCentral::StartDicoveryAgent()
{
    m_deviceDiscoveryAgent = std::make_unique<QBluetoothDeviceDiscoveryAgent>(this);
    m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(BLE_SCAN_TIMEOUT_MS);
    connect(m_deviceDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &NordicCentral::DeviceDiscovered);
    connect(m_deviceDiscoveryAgent.get(), static_cast<void (QBluetoothDeviceDiscoveryAgent::*)
            (QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error), this, &NordicCentral::ScanError);
    connect(m_deviceDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::finished, this, &NordicCentral::ScanFinished);
    connect(m_deviceDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::canceled, this, &NordicCentral::ScanCancelled);
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::DiscoveryMethod::LowEnergyMethod);
}

void NordicCentral::CreateController()
{
    // Make connections
    //! [Connect-Signals-1]
    m_controller = QLowEnergyController::createCentral(m_device, this);
    //! [Connect-Signals-1]
    m_controller->setRemoteAddressType(QLowEnergyController::PublicAddress);

    //! [Connect-Signals-2]
    connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &NordicCentral::ServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished, this, &NordicCentral::ServiceScanDone);
    connect(m_controller, &QLowEnergyController::connectionUpdated, this, &NordicCentral::ConnectionUpdated);

    connect(m_controller, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
            this, [](QLowEnergyController::Error)
    {
        // For debugging
    });

    connect(m_controller, &QLowEnergyController::connected, this, [this]()
    {
        m_bConnected = true;
        m_controller->discoverServices();
    });

    connect(m_controller, &QLowEnergyController::disconnected, this, [this]()
    {
        m_bConnected = false;
    });

    connect(m_controller, &QLowEnergyController::stateChanged, this, []()
    {
        // For debugging
    });

    // Connect
    m_controller->connectToDevice();
    //! [Connect-Signals-2]
}

void NordicCentral::DeviceDiscovered(const QBluetoothDeviceInfo& Device)
{
    std::string DeviceName = Device.name().toLocal8Bit().data();
    if(DeviceName == std::string(DEVICE_NAME))
    {
        m_device = Device;
        m_deviceDiscoveryAgent->stop();
        CreateController();
    }
}

void NordicCentral::ServiceDiscovered(const QBluetoothUuid &gatt)
{
    if(gatt.data1 == NORDIC_BLINKY_SERVICE_UUID)
    {
        m_gatt = gatt;
        m_bGattFound = true;
    }
}

void NordicCentral::ServiceScanDone()
{
    if(m_bGattFound)
    {
        m_service = m_controller->createServiceObject(m_gatt, this);
        if(m_service)
        {
            connect(m_service, &QLowEnergyService::stateChanged, this, &NordicCentral::ServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged, this, &NordicCentral::NordicBlinkyCharChange);
            connect(m_service, &QLowEnergyService::descriptorWritten, this, &NordicCentral::ConfirmedDescriptorWrite);
            m_service->discoverDetails();
        }
    }
}

void NordicCentral::ConnectionUpdated()
{
    // For debugging
}

void NordicCentral::SetError(const QString&)
{
    // For debugging
}

void NordicCentral::SetInfo(const QString&)
{
    // For debugging
}

void NordicCentral::TimerEvent()
{
    if(m_bGotDescriptors && m_timerCounter % LED_TOGGLE_TIME_SEC == 0)
    {
        m_LEDState = (m_LEDState == LED_STATE::OFF ? LED_STATE::ON : LED_STATE::OFF);
        m_service->writeCharacteristic(m_LEDChar, QByteArray(1, static_cast<int8_t>(m_LEDState)));
    }

    ++m_timerCounter;
}

void NordicCentral::ScanError(QBluetoothDeviceDiscoveryAgent::Error)
{
    // For debugging
}

void NordicCentral::ScanFinished()
{
    // For debugging
}

void NordicCentral::ScanCancelled()
{
    // For debugging
}

void NordicCentral::ServiceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s)
    {
        case QLowEnergyService::DiscoveringServices:
            // For debugging
            break;
        case QLowEnergyService::ServiceDiscovered:
        {
            auto chars = m_service->characteristics();
            for(int i = 0; i < chars.size(); ++i)
            {
                if(chars[i].isValid())
                {
                    if(chars[i].uuid().data1 == NORDIC_BLINKY_BUTTON_CHAR_UUID)
                    {
                        auto NotificationDesc = chars[i].descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                        m_service->writeDescriptor(NotificationDesc, QByteArray::fromHex("0100"));
                    }
                    else if(chars[i].uuid().data1 == NORDIC_BLINKY_LED_CHAR_UUID)
                    {
                        m_LEDChar = chars[i];
                    }
                    else if(chars[i].uuid().data1 == NORDIC_BLINKY_IMU_CHAR_UUID)
                    {
                        auto NotificationDesc = chars[i].descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                        m_service->writeDescriptor(NotificationDesc, QByteArray::fromHex("0100"));
                    }
                }
            }
            m_bGotDescriptors = true;
            break;
        }
        case QLowEnergyService::InvalidService:
            // For debugging
            break;
        case QLowEnergyService::LocalService:
            // For debugging
            break;
        case QLowEnergyService::DiscoveryRequired:
            // For debugging
            break;
    }
}

void NordicCentral::NordicBlinkyCharChange(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if(c.uuid().data1 == NORDIC_BLINKY_BUTTON_CHAR_UUID)
    {
        m_bButtonPressed = value.at(0) == 1;
    }
    else if(c.uuid().data1 == NORDIC_BLINKY_IMU_CHAR_UUID)
    {
        m_IMUData = *(reinterpret_cast<const struct IMUData*>(value.constData()));
    }
}

void NordicCentral::ConfirmedDescriptorWrite(const QLowEnergyDescriptor&, const QByteArray&)
{
    // For debugging
}
