#ifndef __INSTALL_WORKER_H__
#define __INSTALL_WORKER_H__

#include <QtWidgets>

class InstallWorker : public QObject
{
    Q_OBJECT
public:
    InstallWorker(const QString& zipPath, const QString& installPath);

signals:
    void installFinished();
    void installProgressUpdated(qreal);
    void installFailed();

public slots:
    void startInstall();

private:
    bool unZipFile();
    const QString m_zipPath;
    const QString m_installPath;
};

#endif