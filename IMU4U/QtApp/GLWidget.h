#pragma once

#include <QBrush>
#include <QFont>
#include <QPen>
#include <QWidget>
#include <QOpenGLWidget>

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

    public:
        GLWidget(QWidget *parent);

    public slots:
        void Animate();

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        int m_Elapsed;
        QBrush background;
        QBrush circleBrush;
        QFont textFont;
        QPen circlePen;
        QPen textPen;
};
