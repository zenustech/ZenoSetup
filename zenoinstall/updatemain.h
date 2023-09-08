#ifndef UPDATEMAIN_H
#define UPDATEMAIN_H

#include <QtWidgets>
#include "Dpi.h"

class UpdateMain : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateMain(const QString& version, const QString &url, QWidget *parent = nullptr);
    ~UpdateMain();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* /*event*/);
    void mouseMoveEvent(QMouseEvent* event);

private:
    void initUpdateUI();
    void setupUI();
    QWidget* initTitleWid();
    QWidget* initInstalling();
    QWidget* initFinished();
    quint64 getDirSize(const QString filePath);

private slots:
    void slot_BtnClicked();
    //void slt_netReqFinish(const QString& data, const QString& id);
    //void slt_netReqProgress(qint64 bytesReceived, qint64 bytesTotal);
    void creatSetupInfo();
    void updateProgress(qreal val);
    void updateDownloadProgress(qreal dnow, qreal dtotal);
    void onLatestVersionFound(QString version, QString url);
    void installing();
    void onTimeout();

private:

    QLabel* m_pLbIcon;
    QLabel* m_pLbTitle;
    QPushButton* m_pBtnClose;
    QPushButton* m_pBtnOpen;
    QStackedWidget *m_pStackedWidget;
    QLabel* m_lblInstalling;
    QProgressBar* m_pProgressBar;
    QMovie *movie;
    QLabel* m_pUpdateIconLabel;
    QLabel* m_pLbTotalSize;
    QTimer* m_timer;
    qreal m_percent;
    qreal m_dbTotalSize;


    QPoint m_Move_point;
    Dpi m_dpi;
    QString m_sVersion;
    QString m_sInstallPath;
    
    QThread m_thdGetZeno;
    QThread m_thdInstall;
    QTemporaryDir m_tempDir;
    const QString m_zipName = "zeno.zip";
};

#endif // UpdateMain_H
