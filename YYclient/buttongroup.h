#ifndef BUTTONGROUP_H
#define BUTTONGROUP_H

#include <QWidget>
#include <QSignalMapper>
#include <QMap>

namespace Ui {
class ButtonGroup;
}
class QToolButton;
enum Page{MYFILE, SHARE, TRANKING, TRANSFER, SWITCHUSR};
class ButtonGroup : public QWidget
{
    Q_OBJECT

public:
    explicit ButtonGroup(QWidget *parent = nullptr);
    ~ButtonGroup();
    void changeCurrUser(QString user);
public slots:
    //按钮处理函数
   void slotButtonClick(Page cur);
   void slotButtonClick(QString text);
   void setParent(QWidget *parent);

protected:
   //设置背景
    void paintEvent(QPaintEvent *event);
    //设置拖拽
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent* event);

signals:
    void sigMyFile();
    void sigShareList();
    void sigDownload();
    void sigTransform();
    void sigSwitchUser();
    void closeWindow();
    void minWindow();
    void maxWindow();
private:
    Ui::ButtonGroup *ui;
    //鼠标位置
    QPoint m_pos;
    //主页面指针
    QWidget* m_parent;
    //信号map
    QSignalMapper* m_mapper;
    QToolButton* m_curBtn;
    //map---按钮显示内容：按钮指针
    QMap<QString, QToolButton*> m_btns;
    //map---按钮显示内容：Qstring
    QMap<Page, QString> m_pages;
};

#endif // BUTTONGROUP_H
