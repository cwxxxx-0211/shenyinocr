//#include "waitting.h"
//#include "ui_waitting.h"
//#include <QSettings>
//#include <QIcon>
//#pragma execution_character_set("utf-8")
//Waitting::Waitting(QWidget *parent) :
//    QWidget(parent),
//    ui(new Ui::Waitting)
//{
//    // 定义版本号 每次更新请变更版本号
//    // v2.1.6 将二维码画圈注释
//    QString version = "Version 4.0.1";


//    QSettings *configIniRead = new QSettings("D:\\SystemInifiles\\ConfigName.ini", QSettings::IniFormat);
//    configIniRead->setIniCodec("GBK");
//    QString Company_Name = configIniRead->value("Company_Name/first").toString();

//    setWindowIcon(QIcon(":/2.png"));

//    setWindowFlags(Qt::FramelessWindowHint);//无边框
//    setAttribute(Qt::WA_TranslucentBackground);//背景透明
//    ui->setupUi(this);
//    ui->label->setText(Company_Name+"欢迎您使用冻存管二\n维码自动采集识别程序");
//    QFont ft;
//    ft.setPointSize(14);
//    ui->label->setFont(ft);
//    ui->label_2->setText("Loading...");
//    ui->label_2->setFont(ft);
//    ui->label_3->setText(version);
//    ui->label_2->setFont(ft);

////    QSettings *configIniWrite = new QSettings("E:\Name.ini", QSettings::IniFormat);
////    configIniWrite->setIniCodec("GBK");
////    configIniWrite->setValue("Company_Name/first", "硕华生命");
////    configIniWrite->setValue("Hospital_Name/second", "人民医院");

//}

//Waitting::~Waitting()
//{
//    delete ui;
//}
