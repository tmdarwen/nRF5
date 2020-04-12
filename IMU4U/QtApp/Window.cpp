#include "Window.h"

#include <QFontDatabase>

void Window::TimerHandler()
{
    static int counter = 0;

    QString str;
    str.sprintf("Status: %s\n\n"
                "Accel\nX:% 1.2fg\nY:% 1.2fg\nZ:% 1.2fg\nx:% 5d\ny:% 5d\nz:% 5d\n\n"
                "Gyro\nX:% *.2f°/s\nY:% *.2f°/s\nZ:% *.2f°/s\nx:% *d\ny:% *d\nz:% *d\n\n"
                "Mag\nX:% 4.0fuT\nY:% 4.0fμT\nZ:% 4.0fμT\nx:% 4d\ny:% 4d\nz:% 4d\n\n"
                "Button:%s\nError:%d\nTimer:%d",
                "Connected",
                0, 0, 0,
                0, 0, 0,
                6, 0, 6, 0, 6, 0,
                6, 0, 6, 0, 6, 0,
                0, 0, 0,
                0, 0, 0, "Up", 0, counter);

    m_positionLabels.setText(str);
    ++counter;
}

Window::Window() : m_GLWidget(this)
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
    connect(&m_Timer, &QTimer::timeout, &m_GLWidget, &GLWidget::animate);
    m_Timer.start(100);
}
