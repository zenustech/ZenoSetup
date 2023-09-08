#include "updatemain.h"
#include "../winsetup/winsetup.h"
#include "netgetzeno.h"
#include "installworker.h"
#include "zsnetthread.h"

UpdateMain::UpdateMain(const QString& version, const QString& url, QWidget *parent)
    : QWidget(parent),
    m_sVersion(version)
{
    initUpdateUI();
    onLatestVersionFound(m_sVersion, url);
}

UpdateMain::~UpdateMain()
{
    if (m_thdGetZeno.isRunning())
    {
        m_thdGetZeno.quit();
        m_thdGetZeno.wait();
    }

    if (m_thdInstall.isRunning())
    {
        m_thdInstall.quit();
        m_thdInstall.wait();
    }
}

void UpdateMain::onLatestVersionFound(QString version, QString url)
{
    if (url.isEmpty())
        return;

    QString zipPath = m_tempDir.filePath(m_zipName);
    NetGetZeno* pGetZeno = new NetGetZeno(url, zipPath);
    pGetZeno->moveToThread(&m_thdGetZeno);

    connect(&m_thdGetZeno, &QThread::finished, pGetZeno, &QObject::deleteLater);
    connect(&m_thdGetZeno, &QThread::started, pGetZeno, &NetGetZeno::startDownload);
    connect(pGetZeno, &NetGetZeno::downLoadFinished, this, &UpdateMain::installing);
    connect(pGetZeno, &NetGetZeno::downloadProgressUpdated, this, &UpdateMain::updateDownloadProgress);
    connect(pGetZeno, &NetGetZeno::downloadFailed, this, [=]() {
        m_lblInstalling->setText(tr("Download failed, please check the internet environment."));
    });

    m_thdGetZeno.start();
}

void UpdateMain::installing()
{
    m_timer->stop();
    m_pLbTotalSize->hide();

    //create thread to unzip and installing.
    QString zipPath = m_tempDir.filePath(m_zipName);

    InstallWorker* pInstall = new InstallWorker(zipPath, m_sInstallPath);
    pInstall->moveToThread(&m_thdInstall);

    m_lblInstalling->setText(tr("Finish Download, now installing..."));
    //clear the progress
    m_pProgressBar->setValue(0);
    m_pProgressBar->setFormat("0%");

    m_pUpdateIconLabel->setProperty("cssClass", "installing");
    m_pUpdateIconLabel->style()->unpolish(m_pUpdateIconLabel);
    m_pUpdateIconLabel->style()->polish(m_pUpdateIconLabel);
    m_pUpdateIconLabel->update();

    connect(&m_thdInstall, &QThread::finished, pInstall, &QObject::deleteLater);
    connect(&m_thdInstall, &QThread::started, pInstall, &InstallWorker::startInstall);
    connect(pInstall, &InstallWorker::installProgressUpdated, this, &UpdateMain::updateProgress);
    connect(pInstall, &InstallWorker::installFinished, this, [=]() {
        movie->start();
        m_pStackedWidget->setCurrentIndex(1);
        m_pStackedWidget->setFixedSize(m_dpi.adj(QSize(380, 310)));
    });
    connect(pInstall, &InstallWorker::installFailed, this, [=]() {
        m_lblInstalling->setText(tr("The %1 installation has failed, please report to the developers").arg(m_sVersion));
    });
    m_thdInstall.start();
}

void UpdateMain::onTimeout()
{
    qreal value = m_pProgressBar->value() / 100.0;
    qreal dbSpeed = m_dbTotalSize * (value - m_percent);
    m_lblInstalling->setText(tr("Downloading ") + QString::number(dbSpeed, 'f', 1) + "MB/s");
    m_percent = value;
}

void UpdateMain::setupUI()
{
    m_pLbIcon = new QLabel(this);
    m_pLbIcon->setObjectName("m_pUpdateLbIcon");
    m_pLbIcon->setFixedSize(m_dpi.adj(QSize(20, 20)));

    m_pLbTitle = new QLabel(tr("Download and Update"), this);
    m_pLbTitle->setObjectName("m_pLbTitle");

    m_pBtnClose = new QPushButton(this);
    m_pBtnClose->setObjectName("m_pBtnClose");
    m_pBtnClose->setFixedSize(m_dpi.adj(QSize(24, 24)));

    m_pStackedWidget = new QStackedWidget(this);
    m_pStackedWidget->setFixedSize(m_dpi.adj(QSize(380, 100)));
    m_pStackedWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_pStackedWidget->setObjectName("m_pStackedWidget");

    m_pStackedWidget->addWidget(initInstalling());
    m_pStackedWidget->addWidget(initFinished());
    m_pStackedWidget->setCurrentIndex(0);

    connect(m_pBtnClose, &QPushButton::clicked, this, &UpdateMain::slot_BtnClicked);
}

QWidget* UpdateMain::initTitleWid()
{
    QWidget* pWid = new QWidget(this);

    //custom titlebar
    QHBoxLayout* pTitleLayout = new QHBoxLayout;
    pTitleLayout->addWidget(m_pLbIcon, 0, Qt::AlignCenter);
    pTitleLayout->addSpacing(m_dpi.adj(4));
    pTitleLayout->addWidget(m_pLbTitle, 0, Qt::AlignCenter);
    pTitleLayout->addStretch();
    pTitleLayout->addWidget(m_pBtnClose, 0, Qt::AlignCenter);
    pTitleLayout->setContentsMargins(12, 12, 12, 12);

    pWid->setLayout(pTitleLayout);
    pWid->setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#2F2F2F"));
    pWid->setPalette(pal);

    return pWid;
}

void UpdateMain::initUpdateUI()
{
    m_tempDir.setAutoRemove(true);
    setAttribute(Qt::WA_QuitOnClose);
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);

    setupUI();

    QVBoxLayout* pLayout = new QVBoxLayout;

    QWidget* pTitleWid = initTitleWid();

    QVBoxLayout* pStackLayout = new QVBoxLayout;
    pStackLayout->addWidget(m_pStackedWidget);
    pStackLayout->setContentsMargins(38, 0, 38, 32);

    pLayout->addWidget(pTitleWid);
    pLayout->addLayout(pStackLayout);
    pLayout->addSpacing(m_dpi.adj(38));
    pLayout->setMargin(0);
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(0, 0, 0, 0);

    QPalette pal;
    pal = palette();
    QColor bgClr(36, 36, 36);
    pal.setColor(QPalette::Window, bgClr);
    setAutoFillBackground(true);
    setPalette(pal);

    setLayout(pLayout);

    QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO", QSettings::NativeFormat);
    m_sInstallPath = set.value("path").toString();
    if (m_sInstallPath.isEmpty())
        m_sInstallPath = QString("%1\\Zeno\\").arg(QApplication::applicationDirPath());
}

void UpdateMain::updateProgress(qreal val)
{
    m_pProgressBar->setFormat(tr("%1%").arg(QString::number(val * 100, 'f', 1)));
    m_pProgressBar->setValue(val * 100);
}

void UpdateMain::updateDownloadProgress(qreal dnow, qreal dtotal)
{
    updateProgress(dnow / dtotal);
    if (m_pLbTotalSize->text().isEmpty())
    {
        m_dbTotalSize = dtotal / 1024 / 1024;
        QString text = QString::number(m_dbTotalSize, 'f', 1) + "MB";
        m_pLbTotalSize->setText(text);
    }
    if (!m_timer->isActive())
    {
        m_percent = 0;
        m_timer->start(1000);
    }
}

QWidget *UpdateMain::initInstalling()
{
    m_lblInstalling = new QLabel(tr("Downloading %1").arg(m_sVersion), this);
    m_lblInstalling->setObjectName("mLbInstallingTip");
    m_lblInstalling->setAlignment(Qt::AlignCenter);
    m_pLbTotalSize = new QLabel(this);
    m_pLbTotalSize->setObjectName("mLbInstallingTip");
    m_pLbTotalSize->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QHBoxLayout* HLabelLay = new QHBoxLayout;
    HLabelLay->addWidget(m_lblInstalling);
    HLabelLay->addWidget(m_pLbTotalSize);

    m_pProgressBar = new QProgressBar(this);
    m_pProgressBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_pProgressBar->setObjectName("m_pProgressBar");
    m_pProgressBar->setOrientation(Qt::Horizontal);
    m_pProgressBar->setMinimum(0);
    m_pProgressBar->setMaximum(100);
    m_pProgressBar->setValue(0);  // 当前进度
    double dProgress = (m_pProgressBar->value() - m_pProgressBar->minimum()) * 100.0
                    / (m_pProgressBar->maximum() - m_pProgressBar->minimum()); // 百分比计算公式
//    m_pProgressBar->setFormat(QString::fromLocal8Bit("当前进度为：%1%").arg(QString::number(dProgress, 'f', 1)))
    m_pProgressBar->setFormat(tr("%1%").arg(QString::number(dProgress, 'f', 1)));
    m_pProgressBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 对齐方式
    m_pProgressBar->setFixedWidth(m_dpi.adj(380));

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    VLay->addLayout(HLabelLay);
    VLay->setSpacing(10);
    VLay->addWidget(m_pProgressBar);
    VLay->addStretch();

    QWidget* wgt = new QWidget(this);
    wgt->setObjectName("wgt");
    m_lblInstalling->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QHBoxLayout* HLay = new QHBoxLayout;
    m_pUpdateIconLabel = new QLabel(this);
    m_pUpdateIconLabel->setFixedSize(m_dpi.adj(QSize(60, 60)));
    m_pUpdateIconLabel->setProperty("cssClass", "downloading");
    QVBoxLayout* pLayout = new QVBoxLayout;
    pLayout->addSpacing(m_dpi.adj(10));
    pLayout->addWidget(m_pUpdateIconLabel);
    pLayout->addStretch();
    HLay->addLayout(pLayout);
    HLay->addLayout(VLay);
    HLay->setSpacing(m_dpi.adj(20));
    HLay->setContentsMargins(0, m_dpi.adj(30), 0, 0);
    wgt->setLayout(HLay);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &UpdateMain::onTimeout);
    return wgt;
}

QWidget *UpdateMain::initFinished()
{
    QLabel* pFinishImage = new QLabel(this);
    pFinishImage->setObjectName("mLbFinishedImage");
    pFinishImage->setAlignment(Qt::AlignCenter);

    movie = new QMovie(":/img/finish");
    pFinishImage->setMovie(movie); // 1. 设置要显示的 GIF 动画图片
//    movie->start();
    QObject::connect(movie, &QMovie::frameChanged, [=](int frameNumber) {
      // GIF 动画执行一次就结束
        if (frameNumber == movie->frameCount() - 1) {
            movie->stop();
        }
    });

    m_pBtnOpen = new QPushButton(tr("OPEN"), this);
    m_pBtnOpen->setObjectName("m_pBtnInstall");
    m_pBtnOpen->setFixedSize(m_dpi.adj(QSize(164, 35)));
    connect(m_pBtnOpen, &QPushButton::clicked, this, &UpdateMain::slot_BtnClicked);

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    VLay->addWidget(pFinishImage, 0, Qt::AlignCenter);
    VLay->addWidget(m_pBtnOpen, 1, Qt::AlignCenter);
    VLay->addStretch(0);

    QWidget* wgt = new QWidget(this);
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);
    return wgt;
}

void UpdateMain::slot_BtnClicked()
{
    if(sender() == m_pBtnClose)
    {
        terminate();
        close();
    }
    if (sender() == m_pBtnOpen) {
        QString runpath = QString("\"%1\"").arg(m_sInstallPath + "bin/" + EXE_NAME);
        QString rundir = m_sInstallPath.replace("/", "\\");
        QString runParam = "";
        ShellExecuteW(NULL, L"open", runpath.toStdWString().c_str(), runParam.toStdWString().c_str(), rundir.toStdWString().c_str(), SW_SHOWNORMAL);
        this->close();
    }
}

void UpdateMain::creatSetupInfo()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/").append(QStringLiteral("ZENO.lnk"));
    QString startMenuPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation).append("/").append(QStringLiteral("ZENO"));
    QString srcRunFile = m_sInstallPath + "bin/" + EXE_NAME;
    QString srcUninstallFile = m_sInstallPath + "bin/uninstall.exe";
    //创建桌面快捷键
    QFile::remove(desktopPath);
    QFile::link(srcRunFile, desktopPath);

    QDir dir(startMenuPath);
    if (!dir.exists())
        dir.mkdir(startMenuPath);
    if (dir.exists())
    {
        //创建开始菜单
        QString linkRunFile = startMenuPath + "/" + QStringLiteral("ZENO.lnk");
        QFile::remove(linkRunFile);
        qDebug() << QFile::link(srcRunFile, linkRunFile);
        QString linkUnInstallFile = startMenuPath + "/" + QStringLiteral("卸载.lnk");
        QFile::remove(linkUnInstallFile);
        qDebug() << QFile::link(srcUninstallFile, linkUnInstallFile);
    }
    if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7 ||
        QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS8 ||
        QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS8_1)
    {
        //锁定到任务栏
        HINSTANCE nRet = ShellExecute(NULL, QString("taskbarpin").toStdWString().c_str(), desktopPath.toStdWString().c_str(), NULL, NULL, SW_SHOW);
    }
    else if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS10)
    {
        //1.拼接任务栏路径
        QString taskBarPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        int index = taskBarPath.lastIndexOf("/");
        taskBarPath = taskBarPath.mid(0, index + 1) + "Microsoft/Internet Explorer/Quick Launch/User Pinned/TaskBar/" + QStringLiteral("ZENO.lnk");//
        //2.向指定路径创建快捷方式
        QFile::link(srcRunFile, taskBarPath);
    }


    auto version = m_sVersion.toLocal8Bit();
    auto install_date = QDateTime::currentDateTime().toString("yyyy-MM-dd").toLocal8Bit();
    auto display_icon = srcRunFile.toLocal8Bit();
    auto uninstall_path = srcUninstallFile.toLocal8Bit();
    winsetup_app_info appinfo;
    appinfo.display_version = version.data();
    appinfo.display_name = "ZENO";
    appinfo.install_date = install_date.data();
    appinfo.display_icon = display_icon.data();
    appinfo.app_size = getDirSize(m_sInstallPath);
    appinfo.uninstall_path = uninstall_path.data();
    winsetup_put_app_info("ZENO",&appinfo);
    QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO",QSettings::NativeFormat);
    set.setValue("path", m_sInstallPath);
}

quint64 UpdateMain::getDirSize(const QString filePath)
{
    QDir tmpDir(filePath);
    quint64 size = 0;

    /*获取文件列表  统计文件大小*/
    foreach(QFileInfo fileInfo, tmpDir.entryInfoList(QDir::Files))
    {
        size += fileInfo.size();
    }

    /*获取文件夹  并且过滤掉.和..文件夹 统计各个文件夹的文件大小 */
    foreach(QString subDir, tmpDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        size += getDirSize(filePath + QDir::separator() + subDir); //递归进行  统计所有子目录
    }

    return size / 1024;
}

void UpdateMain::mousePressEvent(QMouseEvent* event)
{
    m_Move_point = event->pos();
    //QWidget::mousePressEvent(event);
}

void UpdateMain::mouseReleaseEvent(QMouseEvent* event)
{
    m_Move_point = QPoint();
    //QWidget::mouseReleaseEvent(event);
}

void UpdateMain::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_Move_point.isNull())
    {
        move(event->globalPos() - m_Move_point);
    }
    //QWidget::mouseMoveEvent(event);
}