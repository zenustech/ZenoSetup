#include "frmmain.h"
#include "updatemain.h"

static bool isAppRunOnce()
{
    QSystemSemaphore semaphore("ZENO-Setup.singleton.lock", 1);
    semaphore.acquire();
    QSharedMemory* shareMemory = new QSharedMemory("ZENO-Setup.singleton", qApp);
    if (!shareMemory->create(1))
    {
        semaphore.release();
        return false;
    }
    semaphore.release();
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    qDebug() << WinApi::Instance()->GetOSVersion();

    a.setApplicationName("ZENO SETUP");
    a.setApplicationDisplayName("ZENO SETUP");

    if (!isAppRunOnce())
    {
        QMessageBox::information(0, QObject::tr("ZENO SETUP"), QObject::tr("Exist Run!"));
        return -1;
    }

    QFontDatabase::addApplicationFont(":/font/Alibaba-PuHuiTi-Bold");
    QFontDatabase::addApplicationFont(":/font/Alibaba-PuHuiTi-Heavy");
    QFontDatabase::addApplicationFont(":/font/Alibaba-PuHuiTi-Light");
    QFontDatabase::addApplicationFont(":/font/Alibaba-PuHuiTi-Medium");
    QFontDatabase::addApplicationFont(":/font/Alibaba-PuHuiTi-Regular");

    Dpi dpi;        

    QFont font("Alibaba PuHuiTi", 10);
    a.setStyleSheet(dpi.loadStyle(":/qss/default"));
    a.setFont(font);

    QTranslator t;
    if (t.load(":languages/zh.qm"))
        a.installTranslator(&t);

    frmMain::RunType iType = frmMain::None;
    QString argValue;
    QString url;
    QString version;
    if (argc > 1)
    {        
         QCommandLineParser cmdParser;
         cmdParser.addOptions({{"update", "check app update"},
                              {"bugreport", "upload app dump file"},
                              {"version", "Installation version", "version"},
                              {"url", "Installation package url", "url"},
             });
         cmdParser.process(a);
         if (cmdParser.isSet("update"))
         {
             iType = frmMain::Update;
             argValue = cmdParser.value("update");
         }
         else if (cmdParser.isSet("bugreport"))
         {
             iType = frmMain::Report;
         }
         else if (cmdParser.isSet("version") && cmdParser.isSet("url"))
         {
             version = cmdParser.value("version");
             url = cmdParser.value("url");
         }
    }
    if (url.isEmpty())
    {
        frmMain w(iType);
        w.show();
        return a.exec();
    }
    else
    {
        UpdateMain w(version, url);
        w.show();
        return a.exec();
    } 
}
