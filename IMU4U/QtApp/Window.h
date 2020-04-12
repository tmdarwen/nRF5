#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include "GLWidget.h"

class Window : public QWidget
{
    Q_OBJECT

    public:
        Window();

    private:
        QLabel m_positionLabels;
        QGridLayout dataLayout;
        GLWidget m_GLWidget;
        QGridLayout m_MainLayout;
        QTimer m_Timer;

        void TimerHandler();

};
