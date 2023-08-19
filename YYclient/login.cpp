#include "login.h"
#include "ui_login.h"
#include <QPainter>
#include <QDebug>
#include <QMessageBox>
#include <QRegExp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include "common/des.h"
#include "common/logininfoinstance.h"


login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    m_manager=Common::getNetManager();
    m_mainWin= new MainWindow;
    //设置窗口图标
    this->setWindowIcon(QIcon("://images/go.png"));
    m_mainWin->setWindowIcon(QIcon("://images/go.png"));

    //隐藏密码输入
    ui->log_pwd->setEchoMode(QLineEdit::Password);
    ui->reg_pwd->setEchoMode(QLineEdit::Password);
    ui->reg_ensure_pwd->setEchoMode(QLineEdit::Password);
    //当前显示窗口

    ui->stackedWidget->setCurrentWidget(ui->log_2);
    ui->log_user->setFocus();

    // 数据的格式提示
    ui->log_user->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->reg_user->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->reg_flower->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->log_pwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");
    ui->reg_pwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");
    ui->reg_ensure_pwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");


    // 读取配置文件信息，并初始化
    readCfg();
    //加载图片信息--显示文件列表时用
    m_cm.getFileTypeList();





    //去掉login边框
    this->setWindowFlags(Qt::FramelessWindowHint|windowFlags());


    //title_wg信号处理,关闭按钮
    connect(ui->title_wg,&TitleWg::closeWindow,[=]()
    {
        //判断当前页面
        if(ui->stackedWidget->currentWidget()==ui->log_2)
        {
            this->close();
        }
        else if (ui->stackedWidget->currentWidget()==ui->set_2)
        {
           ui->stackedWidget->setCurrentWidget(ui->log_2);
           //清空空间数据
          ui->set_ip->clear();
          ui->set_port->clear();
        }
        else
        {
            ui->stackedWidget->setCurrentWidget(ui->log_2);

            ui->reg_user->clear();
            ui->reg_pwd->clear();
            ui->reg_ensure_pwd->clear();
            ui->reg_tel->clear();
            ui->reg_flower->clear();
            ui->reg_email->clear();
        }

    });
    connect(ui->title_wg,&TitleWg::showSetWg,[=]()//set按钮
    {
         ui->stackedWidget->setCurrentWidget(ui->set_2);
         ui->set_ip->setFocus();
    });

    connect(ui->button_reg_log,&QToolButton::clicked,[=](){//点击注册页面
        ui->stackedWidget->setCurrentWidget(ui->reg_2);
        ui->reg_user->setFocus();//光标移动到user
    });
    // 切换用户 - 重新登录
    connect(m_mainWin, &MainWindow::changeUser, [=]()
    {
        m_mainWin->hide();
        this->show();
    });


}

login::~login()
{
    delete ui;
}

//设置登录用户需要发送的json包
QByteArray login::setLoginJson(QString user, QString pwd)
{
    QMap<QString,QVariant> login;
    login.insert("user",(QVariant) user);
    login.insert("pwd",(QVariant)pwd);
    /*
    {
        "user":xxx,
        "pwd":xxxx
    }
*/
    QVariant login_vc(login);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(login_vc);
    if(jsonDocument.isNull())
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();


}

//注册用户需要用到的json
QByteArray login::setRegisterJson(QString userName, QString nickName, QString firstPwd, QString phone, QString email)
{
    QMap<QString,QVariant> reg;
    reg.insert("userName",(QVariant)userName);
    reg.insert("nickName",(QVariant)nickName);
    reg.insert("firstPwd",(QVariant)firstPwd);
    reg.insert("phone",(QVariant)phone);
    reg.insert("email",(QVariant)email);


    /*json数据如下
        {
            userName:xxxx,
            nickName:xxx,
            firstPwd:xxx,
            phone:xxx,
            email:xxx
        }
    */
    QVariant reg_vc(reg);
    QJsonDocument jsonDocument= QJsonDocument::fromVariant(reg_vc);
    if(jsonDocument.isNull())
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();
}

// 得到服务器回复的登陆状态， 状态码返回值为 "000", 或 "001"，还有登陆section
QStringList login::getLoginStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;
    QJsonDocument doc=QJsonDocument::fromJson(json,&error);
    if(error.error==QJsonParseError::NoError)
    {
        if(doc.isNull()||doc.isEmpty())
        {
            cout<<"doc.isNull()||doc.isEmpty()";
            return list;
        }
        if(doc.isObject())
        {
            QJsonObject obj=doc.object();
            cout << "服务器返回的数据" << obj;
            //状态码
            list.append( obj.value( "code" ).toString());
            //登陆token
            list.append( obj.value( "token" ).toString());
        }
    }
    else
    {
        cout << "err = " << error.errorString();

    }
    return list;
}


//用户登录
void login::on_button_log_clicked()
{
    //获取信息
    QString user = ui->log_user->text();
    QString pwd = ui->log_pwd->text();
    QString address = ui->set_ip->text();
    QString port = ui->set_port->text();

    //检查格式
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(user))
    {
        QMessageBox::warning(this, "警告", "用户名格式不正确");
        ui->log_user->clear();
        ui->log_user->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(pwd))
    {
        QMessageBox::warning(this, "警告", "密码格式不正确");
        ui->log_pwd->clear();
        ui->log_pwd->setFocus();
        return;
    }


    // 登录信息写入配置文件cfg.json
    // 登陆信息加密
    m_cm.writeLoginInfo(user,pwd,ui->check_rem->isChecked());

    //设置登录信息json，密码经过加密
    QByteArray array=setLoginJson(user,m_cm.getStrMd5(pwd));

    //设置http头信息
    QNetworkRequest request;
    QString url=QString("http://%1:%2/login").arg(address).arg(port);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(array.size()));

    QNetworkReply *reply=m_manager->post(request,array);
    cout << "post url:" << url << "post data: " << array;


    //接收服务器响应
    connect(reply,&QNetworkReply::finished,[=](){
        //出错
        if(reply->error() !=QNetworkReply::NoError)
        {
            cout<<reply->errorString();
            reply->deleteLater();
            return;
        }

        //读出数据
        QByteArray json=reply->readAll();
        /*
            登陆 - 服务器回写的json数据包格式：
                成功：{"code":"000"}
                失败：{"code":"001"}
        */
        cout << "server return value: " << json;
        QStringList tmpList=getLoginStatus(json);
        if( tmpList.at(0) == "000" )
        {
            LoginInfoInstance *p = LoginInfoInstance::getInstance(); //获取单例
            p->setLoginInfo(user, address, port, tmpList.at(1));
            cout << p->getUser().toUtf8().data() << ", " << p->getIp() << ", " << p->getPort() << tmpList.at(1);

            //隐藏当前窗口
            this->hide();
            //显示主窗口
            m_mainWin->showMainWindow();
        }
        else
        {
            QMessageBox::warning(this, "登录失败", "用户名或密码不正确！！！");
        }

         reply->deleteLater();
    });



}







//用户设置
void login::on_button_set_clicked()
{
    QString ip = ui->set_ip->text();
    QString port = ui->set_port->text();
    QRegExp regexp(IP_REG);
    if(!regexp.exactMatch(ip))
    {
        QMessageBox::warning(this, "警告", "您输入的IP格式不正确, 请重新输入!");
        return;
    }
    // 端口号
    regexp.setPattern(PORT_REG);
    if(!regexp.exactMatch(port))
    {
        QMessageBox::warning(this, "警告", "您输入的端口格式不正确, 请重新输入!");
        return;
    }
    // 跳转到登陆界面
    ui->stackedWidget->setCurrentWidget(ui->log_2);
    // 将配置信息写入配置文件中
    m_cm.writeWebInfo(ip, port);
}






//用户注册
void login::on_button_reg_clicked()
{

    // 从控件中取出用户输入的数据
    QString userName = ui->reg_user->text();
    QString nickName = ui->reg_flower->text();
    QString firstPwd = ui->reg_pwd->text();
    QString surePwd = ui->reg_ensure_pwd->text();
    QString phone = ui->reg_tel->text();
    QString email = ui->reg_email->text();


    //格式校验
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(userName))
    {
        QMessageBox::warning(this, "警告", "用户名格式不正确");
        ui->reg_user->clear();
        ui->reg_user->setFocus();
        return;
    }
    if(!regexp.exactMatch(nickName))
    {
        QMessageBox::warning(this, "警告", "昵称格式不正确");
        ui->reg_flower->clear();
        ui->reg_flower->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(firstPwd))
    {
        QMessageBox::warning(this, "警告", "密码格式不正确");
        ui->reg_pwd->clear();
        ui->reg_pwd->setFocus();
        return;
    }
    if(surePwd != firstPwd)
    {
        QMessageBox::warning(this, "警告", "两次输入的密码不匹配, 请重新输入");
        ui->reg_ensure_pwd->clear();
        ui->reg_ensure_pwd->setFocus();
        return;
    }
    regexp.setPattern(PHONE_REG);
    if(!regexp.exactMatch(phone))
    {
        QMessageBox::warning(this, "警告", "手机号码格式不正确");
        ui->reg_tel->clear();
        ui->reg_tel->setFocus();
        return;
    }
    regexp.setPattern(EMAIL_REG);
    if(!regexp.exactMatch(email))
    {
        QMessageBox::warning(this, "警告", "邮箱码格式不正确");
        ui->reg_email->clear();
        ui->reg_email->setFocus();
        return;
    }

    // 将注册信息打包为json格式
    QByteArray array=setRegisterJson(userName,nickName,firstPwd,phone,email);
    qDebug() << "register json data" << array;
     // 设置连接服务器要发送的url
    QNetworkRequest request;
    QString url=QString("http://%1:%2/reg").arg(ui->set_ip->text()).arg(ui->set_port->text());
    request.setUrl(QUrl(url));

    //设置http头
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(array.size()));

    //发送数据
    QNetworkReply* reply = m_manager->post(request, array);

    //接受数据
    connect(reply,&QNetworkReply::readyRead,[=]()
    {
        QByteArray json=reply->readAll();
        /*
        注册 - server端返回的json格式数据：
            成功:         {"code":"002"}
            该用户已存在：  {"code":"003"}
            失败:         {"code":"004"}
        */
        // 判断状态码
        if("002" == m_cm.getCode(json))
        {
            QMessageBox::information(this, "注册成功", "注册成功，请登录");
            ui->reg_user->clear();
            ui->reg_flower->clear();
            ui->reg_pwd->clear();
            ui->reg_ensure_pwd->clear();
            ui->reg_tel->clear();
            ui->reg_email->clear();

            //设置登录窗口信息并转到登录界面
            ui->log_user->setText(userName);
            ui->log_pwd->setText(firstPwd);
            ui->stackedWidget->setCurrentWidget(ui->log_2);

        }
        else if("003" == m_cm.getCode(json))
        {
            QMessageBox::warning(this,"注册失败","该用户已存在");
        }
        else
        {
            QMessageBox::warning(this,"注册失败","注册失败!!");
        }
        delete reply;


    });
}

//读取配置信息。设置默认登录状态
void login::readCfg()
{
    QString user=m_cm.getCfgValue("login","user");
    QString pwd=m_cm.getCfgValue("login","pwd");
    QString remember=m_cm.getCfgValue("login","remember");
    int ret=0;
    if(remember=="yes")//记住密码
    {
        unsigned char encPwd[512]={0};
        int encPwdLen=0;
        QByteArray tmp=QByteArray::fromBase64(pwd.toLocal8Bit());
        ret=DesDec((unsigned char*)tmp.data(),tmp.size(),encPwd,&encPwdLen);
        if(ret!=0)//解码失败
        {
            cout << "DesDec";
            return;
        }
    #ifdef _WIN32 //如果是windows平台
    //fromLocal8Bit(), 本地字符集转换为utf-8
    ui->log_pwd->setText( QString::fromLocal8Bit( (const char *)encPwd, encPwdLen ) );
    #else //其它平台
        ui->log_pwd->setText( (const char *)encPwd );
    #endif
        ui->check_rem->setChecked(true);

    }
    else
    {
        ui->log_pwd->setText("");
        ui->check_rem->setChecked(false);
    }

    //解密用户名
    unsigned char encUsr[512]={0};
    int encUsrLen=0;
    QByteArray tmp = QByteArray::fromBase64( user.toLocal8Bit());
    ret = DesDec( (unsigned char *)tmp.data(), tmp.size(), encUsr, &encUsrLen);
    if(ret != 0)
    {
        cout << "DesDec";
        return;
    }

    #ifdef _WIN32 //如果是windows平台
        //fromLocal8Bit(), 本地字符集转换为utf-8
        ui->log_user->setText( QString::fromLocal8Bit( (const char *)encUsr, encUsrLen ) );
    #else //其它平台
        ui->log_usr->setText( (const char *)encUsr );
    #endif

    QString ip = m_cm.getCfgValue("web_server", "ip");
    QString port = m_cm.getCfgValue("web_server", "port");
    ui->set_ip->setText(ip);
    ui->set_port->setText(port);


}



void login::paintEvent(QPaintEvent *event)
{
    //给窗口画背景图
    QPainter p(this);
    QPixmap pixmap("://images/login_bk.png");
    p.drawPixmap(0,0,this->width(),this->height(),pixmap);
}
