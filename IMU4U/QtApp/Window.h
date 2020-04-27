#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include "GLWidget.h"
#include "NordicCentral.h"

class Window : public QWidget
{
    Q_OBJECT

    public:
        Window(NordicCentral&);

    private:
        void TimerHandler();

        QLabel m_positionLabels;
        QGridLayout dataLayout;
        GLWidget m_GLWidget;
        QGridLayout m_MainLayout;
        QTimer m_Timer;

        NordicCentral& m_NordicCentral;
};
