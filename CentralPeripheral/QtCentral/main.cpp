#include <QCoreApplication>
#include <QLowEnergyController>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>
#include <iostream>
#include <memory>
#include <string>

namespace
{
    const char* DEVICE_NAME = "Nordic_Blinky";                      // Name of the BLE device we want to connect to
    constexpr char LED_ON = 1;
    constexpr char LED_OFF = 0;
    constexpr int BLE_SCAN_TIMEOUT_MS = 5000;                       // Scan timeout in milliseconds
    constexpr int TIMER_MS = 1000;                                  // TimerEvent() called every TIMER_MS milliseconds
    constexpr unsigned int LED_TOGGLE_TIME_SEC  = 2;                // Toggle the LED this frequently
    constexpr unsigned int NORDIC_BLINKY_SERVICE_UUID = 0x1523;     // Blinky service UUID
    constexpr unsigned int NORDIC_BLINKY_BUTTON_CHAR_UUID = 0x1524; // Button characteristic UUID
    constexpr unsigned int NORDIC_BLINKY_LED_CHAR_UUID = 0x1525;    // LED characteristic UUID
}

class NordicCentral : public QObject
{
    public:
        NordicCentral() = default;
        ~NordicCentral() = default;

        void Start()
        {
            std::cout << "Starting..." << std::endl;
            StartTimer();
            StartDicoveryAgent();
        }

    private:
        void StartTimer()
        {
            connect(&m_timer, &QTimer::timeout, this, &NordicCentral::TimerEvent);
            m_timer.start(TIMER_MS);
        }

        void StartDicoveryAgent()
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

         void CreateController()
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
                std::cout << "Device connected" << std::endl;
                m_controller->discoverServices();
            });

            connect(m_controller, &QLowEnergyController::disconnected, this, []()
            {
                std::cout << "Device disconnected" << std::endl;
            });

            connect(m_controller, &QLowEnergyController::stateChanged, this, []()
            {
                // For debugging
            });

            // Connect
            m_controller->connectToDevice();
            //! [Connect-Signals-2]
        }

    private slots:
        void DeviceDiscovered(const QBluetoothDeviceInfo& Device)
        {
            std::string DeviceName = Device.name().toLocal8Bit().data();
            if(DeviceName == std::string(DEVICE_NAME))
            {
                m_device = Device;
                //m_DeviceLastDiscoveredCount = m_TimerCounter;
                m_deviceDiscoveryAgent->stop();
                CreateController();
            }
        }

        void ServiceDiscovered(const QBluetoothUuid &gatt)
        {
            if(gatt.data1 == NORDIC_BLINKY_SERVICE_UUID)
            {
                m_gatt = gatt;
                m_bGattFound = true;
            }
        }

        void ServiceScanDone()
        {
            if(m_bGattFound)
            {
                m_service = m_controller->createServiceObject(m_gatt, this);
                if(m_service)
                {
                    connect(m_service, &QLowEnergyService::stateChanged, this, &NordicCentral::ServiceStateChanged);
                    connect(m_service, &QLowEnergyService::characteristicChanged, this, &NordicCentral::NordicBlinkyButtonPress);
                    connect(m_service, &QLowEnergyService::descriptorWritten, this, &NordicCentral::ConfirmedDescriptorWrite);
                    m_service->discoverDetails();
                }
            }
        }

        void ConnectionUpdated()
        {
            // For debugging
        }

        void SetError(const QString&)
        {
            // For debugging
        }

        void SetInfo(const QString&)
        {
            // For debugging
        }

        void TimerEvent()
        {
            if(m_bGotDescriptors && m_TimerCounter % LED_TOGGLE_TIME_SEC == 0)
            {
                bool toggleOn = (m_TimerCounter % (LED_TOGGLE_TIME_SEC * 2) != 0);
                std::cout << "Toggling LED " << (toggleOn ? "on" : "off") << std::endl;
                m_service->writeCharacteristic(m_LEDChar, QByteArray(1, toggleOn ? LED_ON : LED_OFF));
            }

            ++m_TimerCounter;
        }

        void ScanError(QBluetoothDeviceDiscoveryAgent::Error)
        {
            // For debugging
        }

        void ScanFinished()
        {
            // For debugging
        }

        void ScanCancelled()
        {
            // For debugging
        }

        void ServiceStateChanged(QLowEnergyService::ServiceState s)
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

        void NordicBlinkyButtonPress(const QLowEnergyCharacteristic &c, const QByteArray &value)
        {
            if(c.uuid().data1 == NORDIC_BLINKY_BUTTON_CHAR_UUID)
            {
                std::cout << "Button press " << (value.at(0) == 1 ? "down" : "up") << std::endl;
            }
        }

        void ConfirmedDescriptorWrite(const QLowEnergyDescriptor&, const QByteArray&)
        {
            // For debugging
        }

    private:
        QBluetoothDeviceInfo                            m_device;
        std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_deviceDiscoveryAgent = nullptr;
        QLowEnergyController*                           m_controller = nullptr;
        QBluetoothUuid                                  m_gatt;
        QLowEnergyCharacteristic                        m_LEDChar;
        QLowEnergyService*                              m_service = nullptr;
        QTimer                                          m_timer;
        uint32_t                                        m_TimerCounter = 0;
        bool                                            m_bGattFound = false;
        bool                                            m_bGotDescriptors = false;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    NordicCentral central;
    central.Start();

    return a.exec();
}
