#include "GLWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

    background = QBrush(QColor(0, 0, 0));
    circleBrush = QBrush(gradient);
    circlePen = QPen(Qt::black);
    circlePen.setWidth(1);
    textPen = QPen(Qt::white);
    textFont.setPixelSize(50);

    m_Elapsed = 0;
    setFixedSize(460, 400);
    setAutoFillBackground(false);
}

void GLWidget::Animate()
{
    m_Elapsed = (m_Elapsed + qobject_cast<QTimer*>(sender())->interval()) % 1000;
    update();
}

void GLWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(event->rect(), background);
    painter.translate(100, 100);
    painter.setPen(textPen);
    painter.setFont(textFont);
    painter.drawText(QRect(45, 25, 200, 75), Qt::AlignCenter, QStringLiteral("OpenGL"));
    painter.drawText(QRect(45, 75, 200, 80), Qt::AlignCenter, QStringLiteral("Display"));
    painter.end();
}
