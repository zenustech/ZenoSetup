#ifndef WINAPI_H
#define WINAPI_H

#include <QObject>
#include <QMutex>
#include <QSettings>
#include <DXGI.h>

class WinApi : public QObject
{
    Q_OBJECT

public:
    static WinApi* Instance();
    static void ReleaseInstance();    

    /********获取操作系统版本，Service pack版本、系统类型************/
    QString GetOSVersion();
    bool IsWow64();//判断是否为64位操作系统

    /***获取物理内存和虚拟内存大小***/
    void GetMemoryInfo(QString &dwTotalPhys,QString &dwTotalVirtual);

    /****获取CPU名称*******/
    QString GetCpuInfo();

    /****获取硬盘信息****/
    QStringList GetDiskInfo();

    /****获取显卡信息*****/
    QStringList GetDisplayCardInfo();

private:
    static WinApi* m_Instance;
    static QMutex m_InstanceMutex;

    WinApi(QObject *parent = nullptr);

};

#endif // WINAPI_H
