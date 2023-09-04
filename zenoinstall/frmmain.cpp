#include "frmmain.h"
#include "zsinstance.h"
#include "../winsetup/winsetup.h"
#include "netgetzeno.h"
#include "netgetversion.h"
#include "installworker.h"


PlainLine::PlainLine(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QColor("#000000"));
    setPalette(pal);
}

PlainLine::PlainLine(int lineWidth, const QColor& clr, QWidget* parent) : QWidget(parent)
{
    setFixedHeight(lineWidth);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, clr);
    setPalette(pal);
}

void PlainLine::setColor(const QColor& clr)
{
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, clr);
    setPalette(pal);
}



frmMain::frmMain(RunType iType, QWidget *parent)
    : QWidget(parent), m_iRunType(iType)
{
    m_tempDir.setAutoRemove(true);
//    qCustomDebug << getDirSize("C:/Program Files/Everything");
    setAttribute(Qt::WA_QuitOnClose);
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);

    connect(ZsInstance::Instance(), &ZsInstance::sig_netReqFinish, this, &frmMain::slt_netReqFinish);
    connect(ZsInstance::Instance(), &ZsInstance::sig_netReqProgress, this, &frmMain::slt_netReqProgress);

    if (m_iRunType == RunType::None)
    {
        ExpandEnvironmentStrings(L"%ProgramFiles%", m_sSysPath, MAX_PATH);
        m_sInstallPath = QString("%1\\Zeno\\").arg(QString::fromStdWString(m_sSysPath));
        //Check whether the legacy ZenoSetup has been installed into C:\ProgramFiles\Zeno
        QDir installDir(m_sInstallPath);
        if (installDir.exists("data.node1.txt"))
        {
            QMessageBox msgBox(QMessageBox::Warning, "", 
                QStringLiteral("你已经使用过旧版本的安装包安装了Zeno，要先卸载旧版本的安装，才能安装新版本的Zeno，是否现在卸载旧版本的Zeno"),
                QMessageBox::Ok | QMessageBox::Cancel);
            if (QMessageBox::Ok == msgBox.exec())
            {
                QString maintanceTool = m_sInstallPath + "maintenancetool.exe";
                QProcess::startDetached(maintanceTool, {});
            }
        }

        initNoneUI();
        QTimer::singleShot(10,[=]{
            ZsInstance::Instance()->NetRequest(GET_VERS,0,URL,{},"");
        });
    }
    else if (m_iRunType == RunType::Update)
    {
        m_sVersion.clear();
        QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO",QSettings::NativeFormat);
        m_sInstallPath = set.value("path").toString();
        if (m_sInstallPath.isEmpty())
            m_sInstallPath = QString("%1\\Zeno\\").arg(QApplication::applicationDirPath());

        initNoneUI();
        initUpdateUI();
        m_pStackBtnGroup->hide();

        NetGetVersion* pGetVer = new NetGetVersion;
        pGetVer->moveToThread(&m_thdGetVer);

        connect(&m_thdGetVer, &QThread::finished, pGetVer, &QObject::deleteLater);
        connect(this, &frmMain::cudaDetected, pGetVer, &NetGetVersion::startGetLatestVer);
        connect(pGetVer, &NetGetVersion::finishGetLatestVer, this, &frmMain::onLatestVersionFound);
        connect(pGetVer, &NetGetVersion::failedGetVersion, this, [=]() {
            m_lblInstalling->setText(tr("The version data from Zeno offical is corrupted, please report to offical.").arg(m_sVersion));
        });

        m_lblInstalling->setText(tr("Get Lastest version..."));
        m_thdGetVer.start();
    }
    else if  (m_iRunType == RunType::Report)
    {

    }

    m_process = new QProcess(this);
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readnvccOutput()));
    connect(m_process, &QProcess::errorOccurred, this, [=](QProcess::ProcessError error) {
        updateCudaHintText();
    });
    m_process->start("nvcc --version");
}

frmMain::~frmMain()
{
    if (m_thdGetVer.isRunning())
    {
        m_thdGetVer.quit();
        m_thdGetVer.wait();
    }

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

void frmMain::onLatestVersionFound(QString version, QString url)
{
    if (url.isEmpty())
        return;

    m_sVersion = version;

    m_lblInstalling->setText(tr("Downloading version %1 ...").arg(m_sVersion));

    QString zipPath = m_tempDir.filePath(m_zipName);
    NetGetZeno* pGetZeno = new NetGetZeno(url, zipPath);
    pGetZeno->moveToThread(&m_thdGetZeno);

    connect(&m_thdGetZeno, &QThread::finished, pGetZeno, &QObject::deleteLater);
    connect(&m_thdGetZeno, &QThread::started, pGetZeno, &NetGetZeno::startDownload);
    connect(pGetZeno, &NetGetZeno::downLoadFinished, this, &frmMain::installing);
    connect(pGetZeno, &NetGetZeno::progressUpdated, this, &frmMain::updateProgress);
    connect(pGetZeno, &NetGetZeno::downloadFailed, this, [=]() {
        m_lblInstalling->setText(tr("Download failed, please check the internet environment."));
    });

    m_thdGetZeno.start();
}

void frmMain::installing()
{
    //create thread to unzip and installing.
    QString zipPath = m_tempDir.filePath(m_zipName);

    InstallWorker* pInstall = new InstallWorker(zipPath, m_sInstallPath);
    pInstall->moveToThread(&m_thdInstall);

    m_lblInstalling->setText(tr("Finish Download, now installing...").arg(m_sVersion));
    //clear the progress
    m_pProgressBar->setValue(0);
    m_pProgressBar->setFormat("0%");

    connect(&m_thdInstall, &QThread::finished, pInstall, &QObject::deleteLater);
    connect(&m_thdInstall, &QThread::started, pInstall, &InstallWorker::startInstall);
    connect(pInstall, &InstallWorker::installProgressUpdated, this, &frmMain::updateProgress);
    connect(pInstall, &InstallWorker::installFinished, this, [=]() {
        movie->start();
        m_pBtnInstall->setText(tr("OPEN"));
        m_pStackBtnGroup->show();
        m_pStackBtnGroup->setCurrentIndex(1);
        m_pStackedWidget->setCurrentIndex(PAGE_FINISHED);
    });
    connect(pInstall, &InstallWorker::installFailed, this, [=]() {
        m_lblInstalling->setText(tr("The installation has failed, please report to the developers").arg(m_sVersion));
    });
    m_thdInstall.start();
}

void frmMain::setupUI()
{
    m_pLbIcon = new QLabel(this);
    m_pLbIcon->setObjectName("m_pLbIcon");
    m_pLbIcon->setFixedSize(m_dpi.adj(QSize(20, 20)));

    m_pLbTitle = new QLabel(tr("ZENO SETUP"), this);
    m_pLbTitle->setObjectName("m_pLbTitle");

    m_subTitle = new QLabel(tr(""), this);
    m_subTitle->setObjectName("mLbTitle");
    m_subTitle->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));

    m_pBtnClose = new QPushButton(this);
    m_pBtnClose->setObjectName("m_pBtnClose");
    m_pBtnClose->setFixedSize(m_dpi.adj(QSize(24, 24)));

    m_pStackedWidget = new QStackedWidget(this);
    m_pStackedWidget->setFixedSize(m_dpi.adj(QSize(380, 310)));
    m_pStackedWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_pStackedWidget->setObjectName("m_pStackedWidget");

    m_pStackedWidget->addWidget(initImage());
    m_pStackedWidget->addWidget(initChooseFolder());
    m_pStackedWidget->addWidget(initPolicy());
    m_pStackedWidget->addWidget(initVersion());
    m_pStackedWidget->addWidget(initInstalling());
    m_pStackedWidget->addWidget(initFinished());
    m_pStackedWidget->setCurrentIndex(m_iPageIdx);

    m_pBtnInstall = new QPushButton(tr("INSTALL"), this);
    m_pBtnInstall->setObjectName("m_pBtnInstall");
    m_pBtnInstall->setFixedSize(m_dpi.adj(QSize(164, 35)));

    m_pBtnNext = new QPushButton(tr("NEXT"), this);
    m_pBtnNext->setObjectName("m_pBtnNext");
    m_pBtnNext->setFixedSize(m_dpi.adj(QSize(164, 35)));
    m_pBtnNext->setVisible(false);

    m_pBtnBack = new QPushButton(tr("BACK"), this);
    m_pBtnBack->setObjectName("m_pBtnBack");
    m_pBtnBack->setFixedSize(m_dpi.adj(QSize(164, 35)));
    m_pBtnBack->setVisible(false);

    connect(m_pBtnClose, &QPushButton::clicked, this, &frmMain::slot_BtnClicked);
    connect(m_pBtnInstall, &QPushButton::clicked, this, &frmMain::slot_BtnClicked);
    connect(m_pBtnNext, &QPushButton::clicked, this, &frmMain::slot_BtnClicked);
    connect(m_pBtnBack, &QPushButton::clicked, this, &frmMain::slot_BtnClicked);
}

QWidget* frmMain::initTitleWid()
{
    QWidget* pWid = new QWidget;

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

void frmMain::initUpdateUI()
{
    m_pStackedWidget->setCurrentIndex(PAGE_DOWNLOAD_AND_INSTALL);
}

void frmMain::initNoneUI()
{
    setupUI();

    QVBoxLayout* pLayout = new QVBoxLayout;

    QWidget* pTitleWid = initTitleWid();

    QColor bgClr(36, 36, 36);

    //顶部分割线
    QWidget* pTopLine = new QWidget;
    pTopLine->setFixedHeight(2);
    pTopLine->setAutoFillBackground(true);

    QHBoxLayout* pSubTitleLayout = new QHBoxLayout;
    pSubTitleLayout->addWidget(m_subTitle);
    pSubTitleLayout->setContentsMargins(38, 13, 0, 13);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#53DEE4"));
    pTopLine->setPalette(pal);

    m_pTopHorline = new PlainLine;
    m_pTopHorline->setColor(bgClr);

    QVBoxLayout* pStackLayout = new QVBoxLayout;
    pStackLayout->addWidget(m_pStackedWidget);
    pStackLayout->setContentsMargins(38, 38, 38, 32);

    m_pStackBtnGroup = new QStackedWidget;
    {
        QWidget* pGrp1 = new QWidget;
        QHBoxLayout* pButtonLayout = new QHBoxLayout;
        pButtonLayout->addWidget(m_pBtnBack);
        pButtonLayout->addStretch();
        pButtonLayout->addWidget(m_pBtnNext);
        pButtonLayout->setContentsMargins(38, 0, 38, 0);
        pGrp1->setLayout(pButtonLayout);

        QWidget* pGrp2 = new QWidget;
        pButtonLayout = new QHBoxLayout;
        pButtonLayout->addWidget(m_pBtnInstall, 0, Qt::AlignCenter);
        pGrp2->setLayout(pButtonLayout);

        m_pStackBtnGroup->addWidget(pGrp1);
        m_pStackBtnGroup->addWidget(pGrp2);
    }
    m_pStackBtnGroup->setCurrentIndex(1);

    m_pBottomHorline = new PlainLine;
    m_pBottomHorline->setColor(bgClr);

    pLayout->addWidget(pTitleWid);
    pLayout->addWidget(pTopLine);
    pLayout->addLayout(pSubTitleLayout);
    pLayout->addWidget(m_pTopHorline);
    pLayout->addLayout(pStackLayout);
    pLayout->addWidget(m_pStackBtnGroup);
    pLayout->addSpacing(m_dpi.adj(38));
    pLayout->addWidget(m_pBottomHorline);
    pLayout->addSpacing(m_dpi.adj(cBottomProgressSpace));   //left space for painting bottom progress.

    pLayout->setMargin(0);
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(0, 0, 0, 0);

    pal = palette();
    pal.setColor(QPalette::Window, bgClr);
    setAutoFillBackground(true);
    setPalette(pal);

    setLayout(pLayout);
}

QWidget *frmMain::initImage()
{
    QLabel* mLbFirstImage = new QLabel;
    mLbFirstImage->setObjectName("mLbFirstImage");
    mLbFirstImage->setFixedSize(m_dpi.adj(QSize(208, 208)));

    QHBoxLayout* HLay = new QHBoxLayout;
    HLay->addWidget(mLbFirstImage);

    QWidget* wgt = new QWidget(this);
    wgt->setObjectName("wgt");
    wgt->setLayout(HLay);
    return  wgt;
}

QWidget *frmMain::initChooseFolder()
{
    m_pEdtPath = new QLineEdit(m_sInstallPath);
    m_pEdtPath->setObjectName("m_pEdtPath");
    m_pEdtPath->setReadOnly(true);
    m_pEdtPath->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    m_pActSelectPath = new QAction;
    QIcon icon;
    icon.addPixmap(QPixmap(":/img/select-dir"), QIcon::Normal, QIcon::Off);
    m_pActSelectPath->setIcon(icon);
    m_pEdtPath->addAction(m_pActSelectPath, QLineEdit::TrailingPosition);
    connect(m_pActSelectPath, &QAction::triggered, this, &frmMain::slot_BtnClicked);

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    //VLay->addSpacing(m_dpi.adj(33));
    VLay->addWidget(m_pEdtPath);
    VLay->addStretch();

    QWidget* wgt = new QWidget;
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);

    return wgt;
}

QWidget *frmMain::initPolicy()
{
    QFile file(":/license/license");
    file.open(QFile::ReadOnly);
    QString data = file.readAll();
    QTextEdit* m_pTextEdit = new QTextEdit;
    m_pTextEdit->setObjectName("m_pTextEdit");
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->append(data);
    m_pTextEdit->setFixedHeight(m_dpi.adj(220));

    m_pTextEdit->verticalScrollBar()->setValue(0);
    m_pTextEdit->moveCursor(QTextCursor::Start);

    m_pCheckBox = new QCheckBox;
    m_pCheckBox->setObjectName("m_pCheckBox");
    m_pCheckBox->setText(tr("I have read it carefully and agree above information."));

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    VLay->addWidget(m_pTextEdit);
    VLay->addSpacing(m_dpi.adj(10));
    VLay->addWidget(m_pCheckBox);
    VLay->addStretch(10);

    QWidget* wgt = new QWidget;
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);
    return wgt;
}

bool frmMain::isCudaValid()
{
    if (NO_CUDA == m_cbCudaVer->currentIndex())
    {
        if (m_cuda_large_version == 12) {
            return false;
        }
        else if (m_cuda_large_version == 11) {
            return false;
        }
        else {
            return false;
        }
    }
    else if (CUDA11 == m_cbCudaVer->currentIndex())
    {
        if (m_cuda_large_version == 12) {
            return true;
        }
        else if (m_cuda_large_version == 11) {
            return true;
        }
        else {
            return false;
        }
    }
    else if (CUDA12 == m_cbCudaVer->currentIndex())
    {
        if (m_cuda_large_version == 12) {
            return true;
        }
        else if (m_cuda_large_version == 11) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

void frmMain::readnvccOutput()
{
    QString output = m_process->readAllStandardOutput();
    //Cuda compilation tools, release 12.1
    QRegExp rx("Cuda compilation tools, release (\\d+)\\.(\\d+)");
    if (rx.indexIn(output) != -1)
    {
        auto caps = rx.capturedTexts();
        if (caps.length() == 3) {
            m_cuda_large_version = caps[1].toInt();
            m_cuda_small_version = caps[2].toInt();
        }
    }
    updateCudaHintText();
    m_process->close();

    QString cudaVer;
    if (m_cuda_large_version == 12)
    {
        emit cudaDetected("cuda12");
    }
    else if (m_cuda_large_version == 11 && m_cuda_small_version >= 7)
    {
        emit cudaDetected("cuda11");
    }
    else
    {
        emit cudaDetected("cpu");
    }
}

void frmMain::updateProgress(qreal val)
{
    m_pProgressBar->setFormat(tr("%1%").arg(QString::number(val * 100, 'f', 1)));
    m_pProgressBar->setValue(val * 100);
}

void frmMain::updateCudaHintText()
{
    QString lblQss = "\
        QLabel {\
            font-family: \"Alibaba PuHuiTi\";\
            font-size: 12px;\
            color: %1;\
            font-weight: 500;\
        }";

    QString textClr;
    QString showText;

    QString qsVer = QString("%1.%2").arg(m_cuda_large_version).arg(m_cuda_small_version);
    QString driverVer = tr("CUDA driver version on your machine is %1").arg(qsVer);

    if (m_cuda_large_version == 0)
    {
        m_lblCudaHint->setText(tr("zeno needs cuda runtime, you can install <br>cuda driver<a href=\"https://developer.nvidia.com/cuda-toolkit\" style=\"color:DodgerBlue;\">here</a><br>cuda 11.7 and cuda 12.1 are recommending version."));
        m_lblCudaHint->setTextFormat(Qt::RichText);
        m_lblCudaHint->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_lblCudaHint->setOpenExternalLinks(true);
        return;
    }
    else if (m_cuda_large_version < 11 || (m_cuda_large_version == 11 && m_cuda_small_version < 7))
    {
        m_lblCudaHint->setText(tr("The cuda version has to greather than 11.7, you can install it <br><a href=\"https://developer.nvidia.com/cuda-toolkit\" style=\"color:DodgerBlue;\">here</a><br>cuda 11.7 and cuda 12.1 are recommending version."));
        m_lblCudaHint->setTextFormat(Qt::RichText);
        m_lblCudaHint->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_lblCudaHint->setOpenExternalLinks(true);
        return;
    }

    if (NO_CUDA == m_cbCudaVer->currentIndex()) {
        if (m_cuda_large_version == 12) {
            showText = tr("%1. \nyou should choose the CUDA12 option.").arg(driverVer);
            textClr = "rgb(236, 30, 95)";
        }
        else if (m_cuda_large_version == 11) {
            showText = tr("%1. \nyou should choose the CUDA11 option.").arg(driverVer);
            textClr = "rgb(236, 30, 95)";
        }
    }
    else if (CUDA11 == m_cbCudaVer->currentIndex()) {
        if (m_cuda_large_version == 12) {
            showText = driverVer;
            textClr = "rgb(252, 197, 39)";
        }
        else if (m_cuda_large_version == 11) {
            showText = driverVer;
            textClr = "rgb(138,226,138)";
        }
    }
    else if (CUDA12 == m_cbCudaVer->currentIndex()) {
        if (m_cuda_large_version == 12) {
            showText = driverVer;
            textClr = "rgb(138,226,138)";
        }
        else if (m_cuda_large_version == 11) {
            showText = tr("%1. \nyou'd better to choose CUDA11").arg(driverVer);
            textClr = "rgb(252, 197, 39)";
        }
    }

    m_lblCudaHint->setStyleSheet(lblQss.arg(textClr));
    m_lblCudaHint->setText(showText);
}

QWidget* frmMain::initVersion()
{
    QLabel* mLbSoftVer = new QLabel(tr("Software Version"));
    mLbSoftVer->setObjectName("mLbSoftVer");
    m_cbSoftVer = new QComboBox;
    m_cbSoftVer->setView(new QListView());
    m_cbSoftVer->setObjectName("m_pCbbSoftVer");
//    m_pCbbSoftVer->addItems(QStringList()<<"11111111111111111" << "22222222222222222222");

    m_lblCudaHint = new QLabel(tr("Your Cuda version is ..."));
    m_lblCudaHint->setStyleSheet("\
        QLabel {\
            font-family: \"Alibaba PuHuiTi\";\
            font-size: 12px;\
            color: rgba(236, 30, 95);\
            font-weight: 500;\
        }\
        ");

    QLabel* plblCudaVer = new QLabel(tr("Cuda Version"));
    plblCudaVer->setObjectName("mLbDriveVer");
    m_cbCudaVer = new QComboBox;
    m_cbCudaVer->setView(new QListView());
    m_cbCudaVer->setObjectName("m_pCbbDriveVer");
    m_cbCudaVer->addItems(QStringList() << "No Cuda" << "CUDA 11" << "CUDA 12");
    m_cbCudaVer->setCurrentIndex(2);
    connect(m_cbCudaVer, &QComboBox::currentTextChanged, this, &frmMain::updateCudaHintText);

    //updateCudaHintText();

    QHBoxLayout* HLay1 = new QHBoxLayout;
    HLay1->addWidget(mLbSoftVer);
    HLay1->addWidget(m_cbSoftVer);

    QHBoxLayout* HLay3 = new QHBoxLayout;
    HLay3->addWidget(plblCudaVer);
    HLay3->addWidget(m_cbCudaVer);

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->addLayout(HLay1);
    VLay->addWidget(m_lblCudaHint);
    VLay->addLayout(HLay3);
    VLay->addStretch();

    QWidget* wgt = new QWidget;
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);
    return wgt;
}

QWidget *frmMain::initInstalling()
{
    m_lblInstalling = new QLabel(tr("Installing..."));
    m_lblInstalling->setObjectName("mLbInstallingTip");
    m_lblInstalling->setAlignment(Qt::AlignCenter);

    m_pProgressBar = new QProgressBar;
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
    m_pProgressBar->setFixedWidth(m_dpi.adj(380.));

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    VLay->addWidget(m_lblInstalling);
    VLay->addSpacing(10);
    VLay->addWidget(m_pProgressBar/*, 0, Qt::AlignCenter*/);
    VLay->addStretch();

    QWidget* wgt = new QWidget;
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);
    return wgt;
}

QWidget *frmMain::initFinished()
{
    QLabel* pFinishImage = new QLabel;
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

    QVBoxLayout* VLay = new QVBoxLayout;
    VLay->setMargin(0);
    VLay->addWidget(pFinishImage, 0, Qt::AlignCenter);
    VLay->addStretch(0);

    QWidget* wgt = new QWidget;
    wgt->setObjectName("wgt");
    wgt->setLayout(VLay);
    return wgt;
}

void frmMain::terminate()
{
    //todo: notify the curl aborted.
}

void frmMain::slot_BtnClicked()
{
    int tmpPageIdx = m_iPageIdx;
    if(sender() == m_pBtnClose)
    {
        terminate();
        close();
    }
    else if(sender() == m_pBtnInstall){
        if(m_pBtnInstall->text() == tr("INSTALL"))
            m_iPageIdx = 1;
        else if(m_pBtnInstall->text() == tr("CANCEL")){
            m_pBtnClose->click();
            m_iPageIdx += 1;
            close();
            return;
        }
        else if(m_pBtnInstall->text() == tr("OPEN")){
            QString runpath = QString("\"%1\"").arg(m_pEdtPath->text() + "bin/" + EXE_NAME);
            QString rundir = m_pEdtPath->text().replace("/", "\\");
            QString runParam = "";
            ShellExecuteW(NULL, L"open", runpath.toStdWString().c_str(), runParam.toStdWString().c_str(), rundir.toStdWString().c_str(), SW_SHOWNORMAL);
            this->close();
        }
    }
    else if(sender() == m_pBtnNext){        
        if(m_iPageIdx == 2){
            if(!m_pCheckBox->isChecked()){
                QMessageBox::warning(this,
                                     QApplication::applicationDisplayName(),
                                     tr("Please read the following license agreement carefully"));
                return;
            }
        }else if(m_iPageIdx == 3){
            //todo: choose url by cuda version.
            if (!isCudaValid()) {
                QMessageBox::warning(this,
                    QApplication::applicationDisplayName(),
                    tr("The cuda version is invalid."));
                return;
            }
            int idxSelectCuda = m_cbCudaVer->currentIndex();
            QString url;
            if (NO_CUDA == idxSelectCuda) {
                url = m_cbSoftVer->currentData(CPU_URL).toString();
            }
            else if (CUDA11 == idxSelectCuda) {
                url = m_cbSoftVer->currentData(CUDA11_URL).toString();
            }
            else if (CUDA12 == idxSelectCuda) {
                url = m_cbSoftVer->currentData(CUDA12_URL).toString();
            }
            ZsInstance::Instance()->NetRequest(GET_SOFT_FILE, 1, url, {}, "");
        }
        m_iPageIdx += 1;
    }
    else if(sender() == m_pBtnBack){
        m_iPageIdx -= 1;
    }
    else if(sender() == m_pActSelectPath){
        QString tmpPath =  QFileDialog::getExistingDirectory(this, tr("Choose Installation Folder"),
                                                             "",
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);
        if (!tmpPath.isEmpty())
        {
            tmpPath = tmpPath.replace('/', '\\');
            if (!tmpPath.endsWith('\\'))
                tmpPath.append('\\');
            m_pEdtPath->setText(tmpPath);
        }
    }

    if(m_iPageIdx == 1){
        m_pBtnInstall->setVisible(false);
        m_pBtnBack->setVisible(false);
        m_pBtnNext->setVisible(true);

        m_pStackBtnGroup->setCurrentIndex(0);

        m_pTopHorline->setColor(QColor());
        m_pBottomHorline->setColor(QColor());
    }
    else if(m_iPageIdx > 1 && m_iPageIdx < 4){

        //m_pBtnInstall->setVisible(false);
        m_pBtnBack->setVisible(true);
        m_pBtnNext->setVisible(true);
        m_pStackBtnGroup->setCurrentIndex(0);
    }
    else{
        m_pBtnInstall->setVisible(true);
        //m_pBtnBack->setVisible(false);
        //m_pBtnNext->setVisible(false);
        m_pStackBtnGroup->setCurrentIndex(1);
    }

    if(m_iPageIdx == 4){
        m_pBtnInstall->setText(tr("CANCEL"));
        m_pStackBtnGroup->setCurrentIndex(1);
    }
    else if(m_iPageIdx == 5){
        m_pBtnInstall->setText(tr("OPEN"));
        m_pStackBtnGroup->setCurrentIndex(1);
    }

    if(tmpPageIdx != m_iPageIdx)
    {
        qDebug() << m_iPageIdx;
        m_pStackedWidget->setCurrentIndex(m_iPageIdx);

        switch (m_iPageIdx)
        {
        case 1: m_subTitle->setText(tr("Choose Installation Folder")); break;
        case 2: m_subTitle->setText(tr("User Agreement and Privacy Policy")); break;
        case 3: m_subTitle->setText(tr("Select Version")); break;
        case 4: m_subTitle->setText(tr("Installing")); break;
        case 5: m_subTitle->setText(tr("Finished")); break;
        }

        if(m_iPageIdx == 5){
            movie->start();
        }
    }

    update();
}

void frmMain::slt_netReqFinish(const QString &data, const QString &id)
{
    QJsonParseError e;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8(), &e);
    if (e.error != QJsonParseError::NoError && jsonDoc.isNull())
    {
        qCustomDebug << e.errorString() << data;
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj.value("code").toInt() == 20000)
    {
        if (id == GET_VERS)
        {
            QJsonObject tempObj = jsonObj.value("data").toObject();
            QJsonArray tempAry = tempObj.value("records").toArray();
            for (auto p: tempAry)
            {
                QJsonObject tempSubObj = p.toObject();
                m_sVersion = tempSubObj.value("version").toString();
                QString verName = m_sVersion;

                m_cbSoftVer->addItem(verName);
                int idx = m_cbSoftVer->findText(verName);

                QJsonArray tempSubAry = tempSubObj.value("platforms").toArray();
                for (auto pSub: tempSubAry)
                {
                    QJsonObject tempSub2Obj = pSub.toObject();
                    QString url = tempSub2Obj.value("url").toString();
                    if (tempSub2Obj.value("platformName").toString() == "cpu")
                    {
                        m_cbSoftVer->setItemData(idx, url, CPU_URL);
                    }
                    else if (tempSub2Obj.value("platformName").toString() == "cuda11")
                    {
                        m_cbSoftVer->setItemData(idx, url, CUDA11_URL);
                    }
                    else if (tempSub2Obj.value("platformName").toString() == "cuda12")
                    {
                        m_cbSoftVer->setItemData(idx, url, CUDA12_URL);
                    }
                }
            }
        }
        else if(id == GET_SOFT_FILE)
        {
            QString url = jsonObj.value("path").toString();
            ZsInstance::Instance()->NetRequest(UNZIP_SOFT_FILE,2,url,{{"installpath",m_pEdtPath->text()}},"");
        }
        else if(id == UNZIP_SOFT_FILE)
        {
            creatSetupInfo();
            m_pBtnNext->click();
        }
    }
}

void frmMain::slt_netReqProgress(qint64 bytesReceived, qint64 bytesTotal)
{
//    qCustomDebug << bytesReceived << bytesTotal;
    double dProgress = bytesTotal * 100.0 / bytesReceived * 0.5; // 百分比计算公式
    if(dProgress < m_pProgressBar->value())
        dProgress+=50;
    m_pProgressBar->setValue(dProgress);
    m_pProgressBar->setFormat(tr("%1%").arg(QString::number(dProgress, 'f', 1)));
}

void frmMain::creatSetupInfo()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/").append(QStringLiteral("ZENO.lnk"));
    QString startMenuPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation).append("/").append(QStringLiteral("ZENO"));
    QString srcRunFile = m_pEdtPath->text() + "bin/" + EXE_NAME;
    QString srcUninstallFile = m_pEdtPath->text() + "bin/uninstall.exe";
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
        //返回值大于32表示成功
        //if (nRet <= 32)	qCustomDebug << QStringLiteral("无法将快捷方式锁定到任务栏！");
        //解锁任务栏
        //nRet = (int)::ShellExecute(NULL, QString("taskbarunpin").toStdWString().c_str(), desktopPath.toStdWString().c_str(), NULL, NULL, SW_SHOW);
        //if (nRet <= 32)	qCustomDebug << QStringLiteral("解锁失败！");
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
    appinfo.app_size = getDirSize(m_pEdtPath->text());
    appinfo.uninstall_path = uninstall_path.data();
    winsetup_put_app_info("ZENO",&appinfo);
    QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO",QSettings::NativeFormat);
    set.setValue("path", m_pEdtPath->text());
}

quint64 frmMain::getDirSize(const QString filePath)
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

    return size/1024;
}

void frmMain::mousePressEvent(QMouseEvent *event)
{
    m_Move_point = event->pos();
    //QWidget::mousePressEvent(event);
}

void frmMain::mouseReleaseEvent(QMouseEvent *event)
{
    m_Move_point = QPoint();
    //QWidget::mouseReleaseEvent(event);
}

void frmMain::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_Move_point.isNull())
    {
        move(event->globalPos() - m_Move_point);
    }
    //QWidget::mouseMoveEvent(event);
}

void frmMain::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if(m_iPageIdx > 0)
    {
        /*========================绘制底部进度圆圈==================================*/
        QRect rc = this->rect();
        qreal bt = rc.bottom();
        qreal newTop = bt - m_dpi.adj(cBottomProgressSpace);
        rc.setTop(newTop);

        static const qreal radius = m_dpi.adj(4);
        static const qreal margin = m_dpi.adj(8);
        static const int nCircles = 5;

        int xPos = rc.width() / 2 - (nCircles * radius * 2 + (nCircles - 1) * margin) / 2.;
        int yPos = this->rect().bottom() - rc.height() / 2. - radius;

        painter.setPen(QColor(83, 222, 228, 255));
        for(int i = 1; i <= nCircles; i++)
        {
            QRect rc(xPos, yPos, 2 * radius, 2 * radius);
            if(i <= m_iPageIdx)
                painter.setBrush(QColor(83, 222, 228, 255));
            else
                painter.setBrush(QColor(83, 222, 228, 0));
            painter.drawEllipse(rc);
            xPos += 2 * radius + margin;
        }
    }
}
