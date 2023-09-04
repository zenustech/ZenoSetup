#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QtWidgets>
#include "Dpi.h"


class PlainLine : public QWidget
{
    Q_OBJECT
public:
    PlainLine(QWidget* parent = nullptr);
    explicit PlainLine(int lineWidth, const QColor& clr, QWidget* parent = nullptr);
    void setColor(const QColor& clr);
};


class frmMain : public QWidget
{
    Q_OBJECT
public:
    enum RunType{None, Update, Report};
    enum CUDAVER{NO_CUDA = 0, CUDA11, CUDA12};
    enum CUDAROLE{CPU_URL = Qt::UserRole + 1, CUDA11_URL, CUDA12_URL};
    enum PAGEIDX {
        PAGE_START,
        PAGE_CHOOSE_PATH,
        PAGE_POLICY,
        PAGE_VERSION,
        PAGE_DOWNLOAD_AND_INSTALL,
        PAGE_FINISHED,
    };

    explicit frmMain(RunType iType = RunType::None, QWidget *parent = nullptr);
    ~frmMain();

private:
    void initNoneUI();
    void initUpdateUI();
    void setupUI();
    QWidget* initTitleWid();
    QWidget* initImage();
    QWidget* initChooseFolder();
    QWidget* initPolicy();
    QWidget* initVersion();
    QWidget* initInstalling();
    QWidget* initFinished();

private slots:
    void slot_BtnClicked();
    void slt_netReqFinish(const QString& data, const QString& id);
    void slt_netReqProgress(qint64 bytesReceived, qint64 bytesTotal);
    void creatSetupInfo();
    quint64 getDirSize(const QString filePath);
    void updateCudaHintText();
    void readnvccOutput();
    void updateProgress(qreal val);
    void onLatestVersionFound(QString version, QString url);
    void installing();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* /*event*/);
    void mouseMoveEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent */*event*/);
    bool isCudaValid();

signals:
    void cudaDetected(QString);

private:
    void terminate();

    QLabel* m_pLbIcon;
    QLabel* m_pLbTitle;
    QLabel* m_subTitle;
    QPushButton* m_pBtnClose;
    QStackedWidget *m_pStackedWidget;
    QPushButton* m_pBtnInstall;
    QPushButton* m_pBtnNext;
    QPushButton* m_pBtnBack;
    QLineEdit* m_pEdtPath;
    QAction* m_pActSelectPath;
    QCheckBox* m_pCheckBox;
    QComboBox* m_cbSoftVer;
    QComboBox* m_cbCudaVer;
    QLabel* m_lblCudaHint;
    QLabel* m_lblInstalling;
    QProgressBar* m_pProgressBar;
    QStackedWidget* m_pStackBtnGroup;
    QMovie *movie;

    PlainLine* m_pTopHorline;
    PlainLine* m_pBottomHorline;

    QPoint m_Move_point;
    Dpi m_dpi;
    int m_iPageIdx = 0;
    int cBottomProgressSpace = 60;
    int m_cuda_large_version = 0;
    int m_cuda_small_version = 0;
    QProcess* m_process;
    QString m_sVersion;
    RunType m_iRunType;
    QString m_sInstallPath;
    TCHAR m_sSysPath[MAX_PATH] = { 0 };

    QThread m_thdGetVer;
    QThread m_thdGetZeno;
    QThread m_thdInstall;
    QTemporaryDir m_tempDir;
    const QString m_zipName = "zeno.zip";
};

#endif // FRMMAIN_H
