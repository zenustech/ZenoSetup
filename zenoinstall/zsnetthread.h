#ifndef ZSNETTHREAD_H
#define ZSNETTHREAD_H

#include <QThread>
#include <QObject>
#include <QDebug>

#define URL "https://web.zenustech.com/web-api/version/version/list"
#define GET_VERS "version_list"
#define GET_SOFT_FILE "get_soft_file"
#define UNZIP_SOFT_FILE "unzip_soft_file"
#define SOFT_FILE_PATH "tmp"
#define EXE_NAME "zenoedit.exe"

#ifdef UNICODE

#define QStringToTCHAR(x)     (wchar_t*) x.utf16()
#define PQStringToTCHAR(x)    (wchar_t*) x->utf16()
#define TCHARToQString(x)     QString::fromUtf16((x))
#define TCHARToQStringN(x,y)  QString::fromUtf16((x),(y))

#else

#define QStringToTCHAR(x)     x.local8Bit().constData()
#define PQStringToTCHAR(x)    x->local8Bit().constData()
#define TCHARToQString(x)     QString::fromLocal8Bit((x))
#define TCHARToQStringN(x,y)  QString::fromLocal8Bit((x),(y))

#endif

typedef struct __tagNetRequestInfo
{
    QString key = "";
    QString value = "";
}ZS_NET_REQ_INFO;

#define qCustomDebug qDebug() << "===>" << __FUNCTION__ << __LINE__

class ZsNetThread : public QThread
{
    Q_OBJECT
public:
    explicit ZsNetThread(QObject *parent = nullptr);
    void setParam(const QString& id, int type, QString url, QVector<ZS_NET_REQ_INFO> reqHeader, QByteArray reqData);

protected:
    void run();

private:
    void netGet();
    void netGetFile();
    void netPost();
    bool unZipFile();

signals:
    void netReqFinish(const QString& data, const QString& id);
    void netReqProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    int m_reqType;
    QString m_id;
    QString m_url;
    QVector<ZS_NET_REQ_INFO> m_reqHeader;
    QByteArray m_reqData;

    bool m_bReport = false;
};

#endif // ZSNETTHREAD_H
