#include "zsnetthread.h"
#include "qfileinfo.h"
#define CURL_STATICLIB
#include <curl/curl.h>

#include "zsinstance.h"
#include "unzip.h"
#include <QDir>

#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

size_t writeToData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
//    qCustomDebug << written << size << nmemb;
    return written;
}

size_t writeToString(void* ptr, size_t size, size_t count, void* stream)
{
    ((std::string*)stream)->append((char*)ptr, 0, size * count);
    return size * count;
}

int progress_callback(void *ptr, double dltotal, double dlnow, double ultotal, double ulnow)
{
//    static int downloadcallcount=0;
//    downloadcallcount++;
//    if(downloadcallcount%10000==0 && dltotal > 0)
//    {
//       //printf("\n dltotal[%03lf] dlnow[%03lf] ultotal[%03lf] ulnow[%03lf]",dltotal,dlnow,ultotal,ulnow);
//        int nPersent = (int)(100.0*dlnow/dltotal);
//        qCustomDebug <<"persent" << nPersent;
//    }
//    qCustomDebug <<"progress:" << dltotal << dlnow << ultotal << ulnow;
    emit ZsInstance::Instance()->sig_netReqProgress(dltotal, dlnow);
    return 0;
}

ZsNetThread::ZsNetThread(QObject *parent)
    : QThread{parent}
{

}

void ZsNetThread::setParam(const QString &id, int type, QString url, QVector<ZS_NET_REQ_INFO> reqHeader, QByteArray reqData)
{
    m_id = id;
    m_reqType = type;
    //if (id == GET_SOFT_FILE) {
    //    m_url = "http://localhost:8000/zeno-2022.03.31-14.06-x86_64-windows-cuda12.zip";
    //}
    //else
    {
        m_url = url;
    }
    m_reqHeader = reqHeader;
    m_reqData = reqData;
}

void ZsNetThread::run()
{
    switch (m_reqType)
    {
    case 0:netGet(); break;
    case 1:netGetFile(); break;
    case 2:unZipFile(); break;
    default:
        break;
    }
}

void ZsNetThread::netGet()
{
    CURL* curl;
    CURLcode res;
    std::string strData;
    QString data;
    curl = curl_easy_init();
    if (curl)
    {
        curl_slist* pHeadlist = NULL;
        pHeadlist = curl_slist_append(pHeadlist, "Content-Type:application/json;charset=UTF-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pHeadlist);
        curl_easy_setopt(curl, CURLOPT_URL, m_url.toStdString().c_str());
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strBody.c_str());//POST参数

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);//对返回的数据进行操作的函数地址
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strData);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        curl_easy_setopt(curl, CURLOPT_POST, 0);   // 0 是get ，1 是post
        res = curl_easy_perform(curl);

        /* Check for errors */

        if (res != CURLE_OK)
            data = QString("{\"code\":-1,\"message\":\"network error:%1\"}").arg(curl_easy_strerror(res));
        else
            data = strData.c_str();

        curl_easy_cleanup(curl);
        curl_slist_free_all(pHeadlist);
    }

    qCustomDebug << data;
    emit netReqFinish(data, m_id);
}

void ZsNetThread::netGetFile()
{
    CURL* curl;
    CURLcode res;
    FILE* fp = NULL;
    std::string strData;
    QString data;
    std::string fileName = "";
    curl = curl_easy_init();
    if (curl)
    {
        QFileInfo fileInfo = QFileInfo(m_url);
        QString sl = QString("%1.%2").arg(SOFT_FILE_PATH).arg(fileInfo.suffix());
        qCustomDebug << m_url << sl;

        fileName = sl.toLocal8Bit().toStdString();
        int isopen = fopen_s(&fp, fileName.c_str(), "wb");
        if (isopen != 0)
        {
            qCustomDebug << fileName.c_str() << "file open error " << isopen << GetLastError();
            return;
        }
        curl_easy_setopt(curl, CURLOPT_URL, m_url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,NULL);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        res = curl_easy_perform(curl);
        if (fp != NULL)
            fclose(fp);

        if (res != CURLE_OK)
            data = QString("{\"code\":-1,\"message\":\"network error:%1\"}").arg(curl_easy_strerror(res));
        else
            data = QString("{\"code\": 20000,\"path\":\"%1\"}").arg(sl);

        curl_easy_cleanup(curl);
    }

    qCustomDebug << "get local file" << QString::fromLocal8Bit(fileName.c_str());
    emit netReqFinish(data, m_id);
}

void ZsNetThread::netPost()
{

}

bool ZsNetThread::unZipFile()
{
    QString installPath;
    for (int i = 0; i < m_reqHeader.size(); ++i) {
        if(m_reqHeader[i].key == "installpath"){
            installPath = m_reqHeader[i].value;
            break;
        }
    }
    qCustomDebug << QDir::homePath() << installPath;
    QDir dir;
    if (!dir.mkpath(installPath))
    {
        QJsonObject sendData;
        sendData.insert("code", -1);
        sendData.insert("message", QStringLiteral("创建文件失败，请确认是否拥有管理员权限！"));
        sendData.insert("error", QStringLiteral("创建安装目录失败"));
        emit netReqFinish(QJsonDocument(sendData).toJson(),m_id);
        return false;
    }
    installPath = installPath.replace("/", "\\");
    QString m_SavePath = m_url.replace("/", "\\");

    HZIP hz = OpenZip(QStringToTCHAR(m_SavePath), 0);
    if (hz)
    {
        //ModifName();
        SetUnzipBaseDir(hz, QStringToTCHAR(installPath));
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
                        if (DeleteFileW(StrOldName.toStdWString().c_str()))
                        {
                            qCustomDebug << QString("DeleteFileW old file suc(%1)").arg(StrOldName);
                            break;
                        }
                        else
                            qCustomDebug << QString("DeleteFileW old file suc(%1)  ErrorCode %d2").arg(StrOldName).arg(GetLastError());
                    }
                    else
                    {
                        qCustomDebug << "old file Name" << StrOldName;
                        break;
                    }
                }
                if (!MoveFileExW(StrExistName.toStdWString().c_str(), StrOldName.toStdWString().c_str(), MOVEFILE_REPLACE_EXISTING))
                    qCustomDebug << "MoveFileExW" << StrExistName << StrOldName << GetLastError();

                zeRet = UnzipItem(hz, i, ze.name);
                if (zeRet != ZR_OK)
                    qCustomDebug << "UnzipItem second  Error" << zeRet;
                else
                    qCustomDebug << "UnzipItem second  Suc";
            }
//            QJsonObject sendData;
//            sendData.insert("code", 0);
//            sendData.insert("total", numitems);
//            sendData.insert("recv", i + 1);
//            emit netReqFinish(QJsonDocument(sendData).toJson(), m_id);
            emit ZsInstance::Instance()->sig_netReqProgress(numitems, i + 1);
        }
        CloseZip(hz);
        QFile::remove(m_SavePath);
        QJsonObject sendData;
        sendData.insert("code", 20000);
        emit netReqFinish(QJsonDocument(sendData).toJson(), m_id);
        return true;
    }
    return false;
}
