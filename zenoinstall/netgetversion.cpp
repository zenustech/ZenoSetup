#include "netgetversion.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


static size_t writeToString(void* ptr, size_t size, size_t count, void* stream)
{
    ((std::string*)stream)->append((char*)ptr, 0, size * count);
    return size * count;
}



NetGetVersion::NetGetVersion(QObject* parent)
    : QObject(parent)
{
}

NetGetVersion::~NetGetVersion()
{
}

QString NetGetVersion::getVersionData()
{
    QString zenoUrl;
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
    return data;
}

void NetGetVersion::startGetLatestVer(const QString& cudaVer)
{
    QString response = getVersionData();

    QJsonParseError e;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &e);
    if (e.error != QJsonParseError::NoError && jsonDoc.isNull())
    {
        emit failedGetVersion();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj.value("code").toInt() == 20000)
    {
        QJsonObject tempObj = jsonObj.value("data").toObject();
        QJsonArray tempArr = tempObj.value("records").toArray();
        if (!tempArr.isEmpty())
        {
            QJsonObject rec = tempArr[0].toObject();
            QString sVersion = rec.value("version").toString();
            QJsonArray tempSubAry = rec.value("platforms").toArray();
            for (auto pSub : tempSubAry)
            {
                QJsonObject tempSub2Obj = pSub.toObject();
                QString url = tempSub2Obj.value("url").toString();
                QString platformName = tempSub2Obj.value("platformName").toString();
                if (cudaVer == platformName)
                {
                    emit finishGetLatestVer(sVersion, url);
                    return;
                }
            }
        }
    }
    emit failedGetVersion();
}

void NetGetVersion::startGetVersion()
{
    QString response = getVersionData();
    emit finishGetVersion(response);
}