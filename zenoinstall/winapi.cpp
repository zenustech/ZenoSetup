#include "winapi.h"
#include <QDebug>


WinApi* WinApi::m_Instance = nullptr;
QMutex WinApi::m_InstanceMutex;

WinApi *WinApi::Instance()
{
    if(m_Instance == nullptr)
    {
        QMutexLocker locker(&m_InstanceMutex);
        if(m_Instance == nullptr)
            m_Instance = new WinApi;
    }

    return m_Instance;
}

void WinApi::ReleaseInstance()
{
    if(m_Instance != nullptr)
    {
        QMutexLocker locker(&m_InstanceMutex);
        if(m_Instance != nullptr)
        {
            delete m_Instance;
            m_Instance = nullptr;
        }
    }
}

QString WinApi::GetOSVersion()
{
    return QSysInfo::prettyProductName();
}

//void WinApi::GetOSVersion(QString &strOSVersion, QString &strServiceVersion)
//{

//    qDebug() << "WindowsVersion: " << QSysInfo::WindowsVersion;
//    qDebug() << "buildAbi: " << QSysInfo::buildAbi();
//    qDebug() << "buildCpuArchitecture: " << QSysInfo::buildCpuArchitecture();
//    qDebug() << "currentCpuArchitecture: " << QSysInfo::currentCpuArchitecture();
//    qDebug() << "kernelType: " << QSysInfo::kernelType();
//    qDebug() << "kernelVersion: " << QSysInfo::kernelVersion();
//    qDebug() << "machineHostName: " << QSysInfo::machineHostName();
//    qDebug() << "prettyProductName: " << QSysInfo::prettyProductName();
//    qDebug() << "productType: " << QSysInfo::productType();
//    qDebug() << "productVersion: " << QSysInfo::productVersion();
//    qDebug() << "Windows Version: " << QSysInfo::windowsVersion();
//}

bool WinApi::IsWow64()
{
    QString info = QSysInfo::currentCpuArchitecture();
    bool ret = false;
    if(info == "x86_64")
        ret = true;
    return ret;
#if 0
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    BOOL bIsWow64 = FALSE;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress( GetModuleHandle("kernel32"),"IsWow64Process");
    if (NULL != fnIsWow64Process)
    {
        fnIsWow64Process(GetCurrentProcess(),&bIsWow64);
    }
    return bIsWow64;
#endif
}


void WinApi::GetMemoryInfo(QString &dwTotalPhys, QString &dwTotalVirtual)
{
    MEMORYSTATUS   Mem;
    GlobalMemoryStatus(&Mem);

    DWORD dwSize = (DWORD)Mem.dwTotalPhys/(1024*1024);
    DWORD dwVirtSize = (DWORD)Mem.dwTotalVirtual/(1024*1024);

    dwTotalPhys = QString("物理内存:%1 MB").arg(dwSize);
    dwTotalVirtual = QString("虚拟内存:%ld MB").arg(dwVirtSize);
}

QString WinApi::GetCpuInfo()
{
    QSettings info("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",QSettings::NativeFormat);
    return info.value("ProcessorNameString").toString();
}

QStringList WinApi::GetDiskInfo()
{
#if 0
    QString m_diskDescribe = "";
    QFileInfoList list = QDir::drives();
    foreach (QFileInfo dir, list)
    {
        QString dirName = dir.absolutePath();
        dirName.remove("/");
        LPCWSTR lpcwstrDriver = (LPCWSTR)dirName.utf16();
        ULARGE_INTEGER liFreeBytesAvailable, liTotalBytes, liTotalFreeBytes;
        if(GetDiskFreeSpaceEx(lpcwstrDriver, &liFreeBytesAvailable, &liTotalBytes, &liTotalFreeBytes) )
        {
            QString free = QString::number((double) liTotalFreeBytes.QuadPart / GB, 'f', 1);
            free += "G";
            QString all = QString::number((double) liTotalBytes.QuadPart / GB, 'f', 1);
            all += "G";

            QString str = QString("%1 %2/%3       ").arg(dirName, free, all);
            m_diskDescribe += str;

            double freeMem = (double) liTotalFreeBytes.QuadPart / GB;

            if(freeMem > m_maxFreeDisk)
                m_maxFreeDisk = freeMem;
        }
    }

    return m_diskDescribe;
#endif
    return QStringList();
}

QStringList WinApi::GetDisplayCardInfo()
{
    // 参数定义
    IDXGIFactory * pFactory;
    IDXGIAdapter * pAdapter;
    std::vector <IDXGIAdapter*> vAdapters;            // 显卡
    int iAdapterNum = 0; // 显卡的数量

    // 创建一个DXGI工厂
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
    if (FAILED(hr))
        return QStringList();

    // 枚举适配器
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    int iCount = 0;

    // 信息输出
    qDebug() << "===============获取到" << iAdapterNum << "块显卡===============";
    QStringList infos;
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        // 获取信息
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);
        QString sinfo = QString::fromWCharArray(adapterDesc.Description);

        if(sinfo.contains("NVIDIA") == false)
            continue;
        // 输出显卡信息
        qDebug() << QStringLiteral("系统视频内存:") << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M";
        qDebug() << QStringLiteral("专用视频内存:") << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M";
        qDebug() << QStringLiteral("共享系统内存:") << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M";
        qDebug() << QStringLiteral("设备描述:") << sinfo;
        iCount++;
    }
    vAdapters.clear();
    return infos;
}

WinApi::WinApi(QObject *parent)
{
}
