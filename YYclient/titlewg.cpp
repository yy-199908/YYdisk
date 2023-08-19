#include "titlewg.h"
#include "ui_titlewg.h"
#include <QMouseEvent>

TitleWg::TitleWg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleWg)
{
    ui->setupUi(this);
    ui->logo->setPixmap(QPixmap("://images/logo_on.png").scaled(40,40));
    m_parent=parent;


    //实现按钮功能
    //设置
    connect(ui->set,&QToolButton::clicked,[=](){
        emit showSetWg();
    });
    //最小化
    connect(ui->min,&QToolButton::clicked,[=](){
        m_parent->showMinimized();
    });
    //关闭
    connect(ui->close,&QToolButton::clicked,[=](){
        emit closeWindow();
    });
}

TitleWg::~TitleWg()
{
    delete ui;
}

void TitleWg::mouseMoveEvent(QMouseEvent *event)
{
    //只允许左键操作
    if(event->buttons()& Qt::LeftButton)
    {
        //窗口跟随鼠标移动
        m_parent->move(event->globalPos()-m_pt);
    }
}

void TitleWg::mousePressEvent(QMouseEvent *event)
{
    //只允许左键操作
    if(event->button()== Qt::LeftButton)
    {
        //求差值
        m_pt=event->globalPos()-m_parent->geometry().topLeft();
    }
}
