#include "Window.h"
#include <QFontDatabase>

namespace
{
    constexpr double ONE_G_IN_LSB = 16384.0;      // Conversion from accelerometer int value to gravitational unit (See datasheet)
    constexpr double MICRO_TESLA_PER_LSB = 0.001; // Conversion from magmometer int value to tesla unit (See datasheet)
    constexpr double DEGREES_PER_LSB = 0.0078125; // Conversion from gyro int value to degrees per second (See datasheet)
    constexpr int    TIMER_MS = 100;              // TimerHandler() called every TIMER_MS milliseconds
}

Window::Window(NordicCentral& nordicCentral) : m_GLWidget(this), m_NordicCentral(nordicCentral)
{
    setFixedSize(600,420);
    setWindowFlags(Qt::Window);

    m_positionLabels.setAlignment(Qt::AlignTop);
    dataLayout.addWidget(&m_positionLabels, 0, 0);

    QFont myFont;
    myFont.setFamily("Consolas");
    myFont.setPointSize(8);

    m_positionLabels.setFont(myFont);

    m_MainLayout.addWidget(&m_GLWidget, 0, 0);
    m_MainLayout.addLayout(&dataLayout, 0, 1);
    setLayout(&m_MainLayout);

    connect(&m_Timer, &QTimer::timeout, this, &Window::TimerHandler);
    connect(&m_Timer, &QTimer::timeout, &m_GLWidget, &GLWidget::Animate);
    m_Timer.start(TIMER_MS);

    m_NordicCentral.Start();
}

void Window::TimerHandler()
{
    static int counter = 0;

    auto IMUData = m_NordicCentral.IMUData();

    QString str;
    str.sprintf("Connected: %s\n\n"
                "Accel\nX:% 2.2fg\nY:% 2.2fg\nZ:% 2.2fg\nx:% 6d\ny:% 6d\nz:% 6d\n\n"
                "Gyro\nX:% *.2f°/s\nY:% *.2f°/s\nZ:% *.2f°/s\nx:% *d\ny:% *d\nz:% *d\n\n"
                "Mag\nX:% 2.2fmT\nY:% 2.2fmT\nZ:% 2.2fmT\nx:% 6d\ny:% 6d\nz:% 6d\n\n"
                "LED:%s\nButton:%s\nError:%d\nTimer:%d",
                m_NordicCentral.Connected() ? "Yes" : "No",
                IMUData.Accel.X / ONE_G_IN_LSB,
                IMUData.Accel.Y / ONE_G_IN_LSB,
                IMUData.Accel.Z / ONE_G_IN_LSB,
                IMUData.Accel.X,
                IMUData.Accel.Y,
                IMUData.Accel.Z,
                6, IMUData.Gyro.X * DEGREES_PER_LSB,
                6, IMUData.Gyro.Y * DEGREES_PER_LSB,
                6, IMUData.Gyro.Z * DEGREES_PER_LSB,
                6, IMUData.Gyro.X,
                6, IMUData.Gyro.Y,
                6, IMUData.Gyro.Z,
                IMUData.Accel.X * MICRO_TESLA_PER_LSB,
                IMUData.Accel.Y * MICRO_TESLA_PER_LSB,
                IMUData.Accel.Z * MICRO_TESLA_PER_LSB,
                IMUData.Accel.X,
                IMUData.Accel.Y,
                IMUData.Accel.Z,
                m_NordicCentral.LEDState() == NordicCentral::LED_STATE::ON ? "On" : "Off",
                m_NordicCentral.ButtonPressed() ? "Down" : "Up", 0, counter);

    m_positionLabels.setText(str);
    ++counter;
}
