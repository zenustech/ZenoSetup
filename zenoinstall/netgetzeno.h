#ifndef __NET_GETZENO_H__
#define __NET_GETZENO_H__

#include <QtWidgets>

class NetGetZeno : public QObject
{
    Q_OBJECT
public:
    NetGetZeno(const QString& url, const QString& savePath, QObject* parent = nullptr);
    ~NetGetZeno();

signals:
    void downLoadFinished();
    void downloadFailed();
    void progressUpdated(qreal);
    void downloadProgressUpdated(qreal dlnow, qreal dlTotal);

public slots:
    void startDownload();

private:
    const QString m_url;
    const QString m_savePath;   //path for downloaded zip.
};

#endif