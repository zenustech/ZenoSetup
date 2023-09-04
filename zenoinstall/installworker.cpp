#include "installworker.h"
#include "unzip.h"


InstallWorker::InstallWorker(const QString& zipPath, const QString& installPath)
    : m_installPath(installPath)
    , m_zipPath(zipPath)
{
}

void InstallWorker::startInstall()
{
    bool ret = unZipFile();
    if (ret) {
        emit installFinished();
    }
    else {
        emit installFailed();
    }
}

bool InstallWorker::unZipFile()
{
    QString installPath = m_installPath;

    QDir dir;
    if (!dir.mkpath(installPath))
    {
        QJsonObject sendData;
        sendData.insert("code", -1);
        sendData.insert("message", QStringLiteral("创建文件失败，请确认是否拥有管理员权限！"));
        sendData.insert("error", QStringLiteral("创建安装目录失败"));
        emit installFailed();
        return false;
    }
    installPath = installPath.replace("/", "\\");

    HZIP hz = OpenZip((wchar_t*)m_zipPath.utf16(), 0);
    if (hz)
    {
        //ModifName();
        SetUnzipBaseDir(hz, (wchar_t*)installPath.utf16());
        ZIPENTRY ze;
        GetZipItem(hz, -1, &ze);
        int numitems = ze.index;
        for (int i = 0; i < numitems; i++)
        {
            ZRESULT zeRet = GetZipItem(hz, i, &ze);
            if (zeRet != ZR_OK) continue;

            zeRet = UnzipItem(hz, i, ze.name);
            //            qInfo() << "unzip => " <<  QString::fromWCharArray(ze.name) << " " << zeRet;
            if (zeRet != ZR_OK)
            {
                QString StrExistName = QString("%1/%2").arg(installPath).arg(QString::fromWCharArray(ze.name));
                QString StrOldName = QString("%1/%2_old").arg(installPath).arg(QString::fromWCharArray(ze.name));
                qInfo() << " want " << StrExistName << " => " << StrOldName;
                for (size_t nindex = 0; nindex < 10; nindex++)
                {
                    StrOldName = StrOldName.replace("/", "\\");
                    QFileInfo finfo(StrOldName);
                    if (finfo.exists())
                    {
                        DeleteFileW(StrOldName.toStdWString().c_str());
                    }
                }
                MoveFileExW(StrExistName.toStdWString().c_str(), StrOldName.toStdWString().c_str(), MOVEFILE_REPLACE_EXISTING);

                zeRet = UnzipItem(hz, i, ze.name);
                if (zeRet != ZR_OK)
                {
                    return false;
                }
            }
            emit installProgressUpdated((qreal)(i + 1) / numitems);
        }
        CloseZip(hz);
        return true;
    }
    return false;
}