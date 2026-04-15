//#include "databasesetting.h"
//#include "ui_databasesetting.h"
//#include <QString>
//#include <QMessageBox>
//#include <QDebug>
//#include <icrsint.h>
//#include <comutil.h>
//#include <string>
//#include <QSettings>
//#include <QTime>
//#include <odbcinst.h>
////#include "afxdb.h"

////#import "C:\Program Files\Common Files\system\ado\msado15.dll" rename("EOF","adoEOF"), rename("BOF", "adoBOF")
//#pragma execution_character_set("utf-8")
//using namespace std;
//using namespace ADODB;
//DatabaseSetting::DatabaseSetting(QWidget *parent) :
//    QWidget(parent),
//    ui(new Ui::DatabaseSetting)
//{
//    setWindowIcon(QIcon(":/2.png"));
//    ui->setupUi(this);


//}

//DatabaseSetting::~DatabaseSetting()
//{
//    delete ui;
//}


//void DatabaseSetting::on_pushButton_connect_clicked()
//{


//    IP = ui->lineEdit_IP->text();
//    Str_Insheet = ui->lineEdit_Insheet->text();
//    Str_Outsheet = ui->lineEdit_outsheet->text();
//    Str_db = ui->lineEdit_dataname->text();
//    QString Str_user = ui->lineEdit_user->text();
//    QString Str_pswd = ui->lineEdit_pswd->text();
//    QString Str_port = ui->lineEdit_port->text();

//    //数据库信息写入本地

//    QSettings *configIniWrite = new QSettings("C:\\ProgramData\\hAcquDir\\hAcquDir.ini", QSettings::IniFormat);
//    configIniWrite->setValue("/DataBase/IP", IP);
//    configIniWrite->setValue("/DataBase/user", Str_user);
//    configIniWrite->setValue("/DataBase/pswd", Str_pswd);
//    configIniWrite->setValue("/DataBase/db", Str_db);
//    configIniWrite->setValue("/DataBase/port", Str_port);
//    configIniWrite->setValue("/DataBase/insheet", Str_Insheet);
//    configIniWrite->setValue("/DataBase/outsheet", Str_Outsheet);
//    delete configIniWrite;


//    //sql函数类型要是string类型
//    string host = ui->lineEdit_IP->text().toStdString();
//    string user = ui->lineEdit_user->text().toStdString();
//    string pswd = ui->lineEdit_pswd->text().toStdString();
//    string db = ui->lineEdit_dataname->text().toStdString();
//    unsigned int port = ui->lineEdit_port->text().toInt();

//    //判断框的选择
//    bool Mysql = ui->radioButton_MySQL->isChecked();
//    bool sqlserver = ui->radioButton_SQLServer->isChecked();
//    bool clear = ui->radioButton_clear->isChecked();
//    bool add = ui->radioButton_add->isChecked();
//    if(Mysql == false && sqlserver==false)
//    {
//        QMessageBox::information(this,tr("提示"),tr("请先选择数据库类型!"));
//        return;
//    }

//    if(clear == false && add == false)
//    {
//        QMessageBox::information(this,tr("提示"),tr("请选择数据库表格是否删除!"));
//        return;
//    }

//    if(ui->radioButton_add->isChecked())
//    {
//        QSettings *configIniWrite = new QSettings("C:\\ProgramData\\hAcquDir\\hAcquDir.ini", QSettings::IniFormat);
//        configIniWrite->setValue("/DataBase/AddOrClear", "Add");
//        delete configIniWrite;
//    }
//    else
//    {
//        QSettings *configIniWrite = new QSettings("C:\\ProgramData\\hAcquDir\\hAcquDir.ini", QSettings::IniFormat);
//        configIniWrite->setValue("/DataBase/AddOrClear", "Clear");
//        delete configIniWrite;
//    }

//    //使用Mysql数据库，连接数据库，写数据库等操作
//    if(ui->radioButton_MySQL->isChecked())
//    {
//        //数据库类型记录写本地
//        QSettings *configIniWrite = new QSettings("C:\\ProgramData\\hAcquDir\\hAcquDir.ini", QSettings::IniFormat);
//        configIniWrite->setValue("/DataBase/SQLType", "MYSQL");
//        delete configIniWrite;

//        //连接mysql，连接成功ok=true
//        mydb = QSqlDatabase::addDatabase("QMYSQL");
//        mydb.setHostName(host.c_str());//主机
//        mydb.setPort(port);//端口号（可以写默认0或者3306等）
//        mydb.setDatabaseName( db.c_str());//数据库名
//        mydb.setUserName(user.c_str());//用户名
//        mydb.setPassword(pswd.c_str());//密码
//        bool ok = mydb.open();

//        if(!ok)

//        {
//            QMessageBox::information(this,tr("提示"),tr("MYSQL数据库连接失败，请重试!\n1.请检查输入数据库参数是否填写正确!"
//                                                      "\n2.请检查所选数据库是否正确!\n具体错如下:") + "\n"+mydb.lastError().text() );
//            return;
//        }

//        else
//        {
//            QMessageBox::information(this,tr("提示"),tr("MYSQL数据库连接成功!"));
//            isConnect = true;
//            this->close();
//        }
//    }

//    //使用SQLServer数据库，连接数据库，写数据库等操作
//    if(ui->radioButton_SQLServer->isChecked())
//    {
//        //数据库类型记录写本地
//        QSettings *configIniWrite = new QSettings("C:\\ProgramData\\hAcquDir\\hAcquDir.ini", QSettings::IniFormat);
//        configIniWrite->setValue("/DataBase/SQLType", "SQLServer");
//        delete configIniWrite;

//        CoInitialize(NULL);
//        HRESULT hr = pconnect.CreateInstance(_uuidof(Connection)); //创建连接句柄
//        if (FAILED(hr))
//        {
//            QMessageBox::information(this,tr("提示"),tr("创建连接指针失败!"));
//            return;
//        }

//        if (FAILED(pRecordset.CreateInstance(_uuidof(Recordset))))  //初始化结果集指针
//        {
//            QMessageBox::information(this,tr("提示"),tr("创建结果集指针失败!"));
//            return;
//        }

//        pconnect->CursorLocation = adUseClient; //加上这句才能获取到结果集；
//        string Connect = "Driver={sql server};server=" + host + ";uid=" + user +";pwd="+ pswd +";database=" + db +";";

//        //连接数据库
//        try
//        {
//            // Open方法连接字串必须四BSTR或者_bstr_t类型
//            //_bstr_t strConnect = "Driver={sql server};server=127.0.0.1;uid=sa;pwd=123456;database=test;";
//            _bstr_t strConnect = (_bstr_t)(Connect.c_str());
//            pconnect->Open(strConnect, "", "", adModeUnknown); //字符串连接  连接数据库
//            QMessageBox::information(this,tr("提示"),tr("SQL Server数据库连接成功!"));
////            ui->pushButton_connect->setEnabled(false);
////            ui->pushButton_breaklink->setEnabled(true);
//            isConnect = true;
//            this->close();
//        }
//        catch (_com_error &e)
//        {
//            QString errorInfo = e.Description();
//            CoUninitialize();
//            QMessageBox::information(this,tr("提示"),tr("SQL Server数据库连接失败,请重试!\n1.请检查输入数据库参数是否填写正确!\n2.请检查所选数据库是否正确!\n具体错如下:") + "\n" + errorInfo);
//            return;
//        }

//    }


//}


//void DatabaseSetting::on_pushButton_breaklink_clicked()
//{
//    if(isConnect == true)
//    {
//        bool Mysql = ui->radioButton_MySQL->isChecked();
//        bool sqlserver = ui->radioButton_SQLServer->isChecked();

//        if(Mysql == true)
//        {
//            mydb.close();
//            QMessageBox::information(this,tr("提示"),tr("数据库已断开"));
////            ui->pushButton_connect->setEnabled(true);
////            ui->pushButton_breaklink->setEnabled(false);
//            this->close();
//        }
//        if(sqlserver == true)
//        {
//            pconnect->Close();
//            QMessageBox::information(this,tr("提示"),tr("数据库已断开"));
////            ui->pushButton_connect->setEnabled(true);
////            ui->pushButton_breaklink->setEnabled(false);
//            this->close();
//        }
//        if(Mysql == false && sqlserver == false)
//        {
//            QMessageBox::information(this,tr("提示"),tr("请先连接数据库!"));
//            return;
//        }
//        isConnect = false;
//    }
//    else
//    {
//        QMessageBox::information(this,tr("提示"),tr("请先连接数据库!"));
//        return;
//    }



//}

