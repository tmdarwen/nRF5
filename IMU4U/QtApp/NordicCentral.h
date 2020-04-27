#pragma once

#include <memory>

#include <QLowEnergyController>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>


typedef struct ThreeDimData
{
    int16_t X;
    int16_t Y;
    int16_t Z;
} ThreeDimData;

typedef struct IMUData
{
    ThreeDimData Mag;
    ThreeDimData Accel;
    ThreeDimData Gyro;
    uint8_t MagStatus;
    uint8_t AccelStatus;
    uint8_t GyroStatus;
    uint8_t ErrorStatus;
} IMUData;


class NordicCentral : public QObject
{
    public:
        enum class LED_STATE
        {
            OFF = 0,
            ON  = 1
        };

        NordicCentral() = default;
        ~NordicCentral() = default;

        void Start();
        bool Connected();
        bool ButtonPressed();
        const IMUData& IMUData();
        LED_STATE LEDState();

    private:
        void StartTimer();
        void StartDicoveryAgent();
        void CreateController();
        void DeviceDiscovered(const QBluetoothDeviceInfo& Device);
        void ServiceDiscovered(const QBluetoothUuid &gatt);
        void ServiceScanDone();
        void ConnectionUpdated();
        void SetError(const QString&);
        void SetInfo(const QString&);
        void TimerEvent();
        void ScanError(QBluetoothDeviceDiscoveryAgent::Error);
        void ScanFinished();
        void ScanCancelled();
        void ServiceStateChanged(QLowEnergyService::ServiceState s);
        void NordicBlinkyCharChange(const QLowEnergyCharacteristic &c, const QByteArray &value);
        void ConfirmedDescriptorWrite(const QLowEnergyDescriptor&, const QByteArray&);

        QBluetoothDeviceInfo                            m_device;
        std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_deviceDiscoveryAgent = nullptr;
        QLowEnergyController*                           m_controller = nullptr;
        QBluetoothUuid                                  m_gatt;
        QLowEnergyCharacteristic                        m_LEDChar;
        QLowEnergyCharacteristic                        m_IMUChar;
        QLowEnergyService*                              m_service = nullptr;
        QTimer                                          m_timer;
        uint32_t                                        m_timerCounter = 0;
        bool                                            m_bConnected = false;
        bool                                            m_bGattFound = false;
        bool                                            m_bGotDescriptors = false;
        bool                                            m_bButtonPressed = false;
        LED_STATE                                       m_LEDState = LED_STATE::OFF;
        struct IMUData                                  m_IMUData;
};
