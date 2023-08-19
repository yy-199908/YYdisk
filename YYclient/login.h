#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <mainwindow.h>
#include "common/common.h"

namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();
    // 设置登陆用户信息的json包
    QByteArray setLoginJson(QString user, QString pwd);

    // 设置注册用户信息的json包
    QByteArray setRegisterJson(QString userName, QString nickName, QString firstPwd, QString phone, QString email);

    // 得到服务器回复的登陆状态， 状态码返回值为 "000", 或 "001"，还有登陆section
    QStringList getLoginStatus(QByteArray json);


private slots:
    void on_button_log_clicked();//登录
    void on_button_set_clicked();//设置
    void on_button_reg_clicked();//注册



protected:
    //绘图事件函数
    void paintEvent(QPaintEvent *event);


private:
    void  readCfg();//读取json文件
    Ui::login *ui;
    QNetworkAccessManager *m_manager;
    //主窗口指针
    MainWindow *m_mainWin;
    Common m_cm;


};

#endif // LOGIN_H
