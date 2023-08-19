#ifndef TITLEWG_H
#define TITLEWG_H

#include <QWidget>

namespace Ui {
class TitleWg;
}

class TitleWg : public QWidget
{
    Q_OBJECT

public:
    explicit TitleWg(QWidget *parent = nullptr);
    ~TitleWg();
protected:
   void mouseMoveEvent(QMouseEvent *event);
   void mousePressEvent(QMouseEvent *event);
signals:
   void showSetWg();
    void closeWindow();
private:
    Ui::TitleWg *ui;
    QPoint m_pt;//鼠标和窗口左上角的位置差值
    QWidget* m_parent;
};

#endif // TITLEWG_H
