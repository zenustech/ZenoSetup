#ifndef __NET_GETVERSION_H__
#define __NET_GETVERSION_H__

#include <QObject>

class NetGetVersion : public QObject
{
    Q_OBJECT
public:
    NetGetVersion(QObject* parent = nullptr);
    ~NetGetVersion();

signals:
    void finishGetLatestVer(QString version, QString url);
    void finishGetVersion(QString response);
    void failedGetVersion();

public slots:
    void startGetLatestVer(const QString& cudaVer);
    void startGetVersion();

private:
    QString getVersionData();
    const QString m_url = "https://web.zenustech.com/web-api/version/version/list";
};

#endif