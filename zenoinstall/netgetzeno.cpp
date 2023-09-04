#include "netgetzeno.h"
#define CURL_STATICLIB
#include <curl/curl.h>


static size_t writeToData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    //    qCustomDebug << written << size << nmemb;
    return written;
}

static int progress_callback(void* ptr, double dltotal, double dlnow, double ultotal, double ulnow)
{
    NetGetZeno* pWorker = static_cast<NetGetZeno*>(ptr);
    emit pWorker->progressUpdated(dltotal > 0 ? dlnow / dltotal : 0);
    return 0;
}


NetGetZeno::NetGetZeno(const QString& url, const QString& savePath, QObject* parent)
    : QObject(parent)
    , m_url(url)
    , m_savePath(savePath)
{
}

NetGetZeno::~NetGetZeno()
{
}

void NetGetZeno::startDownload()
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
        fileName = m_savePath.toLocal8Bit().toStdString();
        int isopen = fopen_s(&fp, fileName.c_str(), "wb");
        if (isopen != 0)
        {
            return;
        }
        curl_easy_setopt(curl, CURLOPT_URL, m_url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        res = curl_easy_perform(curl);
        if (fp != NULL)
            fclose(fp);

        if (res != CURLE_OK) {
            emit downloadFailed();
            return;
        }

        curl_easy_cleanup(curl);
    }
    emit downLoadFinished();
}