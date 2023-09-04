#include "zenosetup.h"
#include "zenodialog.h"
#include "../winsetup/winsetup.h"



ZenoSetup::ZenoSetup(RunType iType, QWidget *parent)
    : QWidget(parent), m_iRunType(iType)
{
    setWindowFlags(Qt::FramelessWindowHint);

    if (m_iRunType == RunType::None)
    {
        ExpandEnvironmentStrings(L"%ProgramFiles%", m_sSysPath, MAX_PATH);
        m_sInstallPath = QString("%1\\%2\\").arg(QString::fromStdWString(m_sSysPath)).arg(QGuiApplication::applicationName());
        initNoneUI();
    }
    else if  (m_iRunType == RunType::Update)
    {
        QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO",QSettings::NativeFormat);
        m_sInstallPath = set.value("path").toString();
        if (m_sInstallPath.isEmpty())
            m_sInstallPath = QString("%1\\%2\\").arg(QApplication::applicationDirPath()).arg(QGuiApplication::applicationName());
        initNoneUI();
        initUpdateUI();

    }
    else if  (m_iRunType == RunType::Report)
    {

    }
}

ZenoSetup::~ZenoSetup()
{
}

void ZenoSetup::initNoneUI()
{
    m_pLbBackImg = new QLabel(this);
    m_pLbBackImg->setObjectName("m_pLbBackImg");
    m_pLbBackImg->resize(m_dpi.adj(QSize(600,323)));
    m_pLbBackImg->move(0, 0);
    m_pLbBackImg->setScaledContents(true);
    m_pLbBackImg->setPixmap(QPixmap(":/img/install-bk"));

    m_pLbSplit = new QLabel(this);
    m_pLbSplit->setObjectName("m_pLbSplit");
    m_pLbSplit->move(0, m_dpi.adj(323));

    m_pLbTitleIcon = new QLabel(this);
    m_pLbTitleIcon->setObjectName("m_pLbTitleIcon");
    m_pLbTitleIcon->move(m_dpi.adj(QPoint(14, 14)));

    m_pBtnClose = new QPushButton(this);
    m_pBtnClose->setObjectName("m_pBtnClose");
    m_pBtnClose->move(m_dpi.adj(QPoint(564, 14)));


    pLbTip = new QLabel(QString::fromLocal8Bit("正在安装") + QGuiApplication::applicationDisplayName() , this);
    pLbTip->setObjectName("pLbTip");
    pLbTip->move(m_dpi.adj(QPoint(60, 353)));
    pLbTip->setVisible(false);

    m_pLbProgress = new QLabel(QStringLiteral("0%"), this);
    m_pLbProgress->setObjectName("m_pLbProgress");
    m_pLbProgress->setAlignment(Qt::AlignRight);
    m_pLbProgress->move(m_dpi.adj(QPoint(490, 353)));
    m_pLbProgress->setVisible(false);

    m_pPbDownload = new QProgressBar(this);
    m_pPbDownload->setObjectName("m_pPbDownload");
    m_pPbDownload->setMaximum(m_dpi.adj(100));
    m_pPbDownload->setMinimum(0);
    m_pPbDownload->setTextVisible(false);
    m_pPbDownload->move(m_dpi.adj(QPoint(60, 388)));
    m_pPbDownload->setVisible(false);

    m_pBtnInstall = new QPushButton(QStringLiteral("立即安装"), this);
    m_pBtnInstall->setObjectName("m_pBtnInstall");
    m_pBtnInstall->move(m_dpi.adj(QPoint(211, 305)));

    m_pCbAgree = new QCheckBox(this);
    m_pCbAgree->setObjectName("m_pCbAgree");
    m_pCbAgree->setText(QStringLiteral("我已仔细阅读并同意"));
    m_pCbAgree->move(m_dpi.adj(QPoint(30, 380)));

    m_pLbProtocol = new QLabel(this);
    m_pLbProtocol->setObjectName("m_pLbProtocol");
    m_pLbProtocol->setText(QStringLiteral("<p style='color: #878787;'><a style='color: #353535;' href='http://h5.everylinks.com/iosLinkServe.html'>《许可及服务协议》</a> 和 <a style='color: #353535;'href='http://h5.everylinks.com/iosLinkSecret.html'>《隐私保护指引》</a></p>").toUtf8());
    m_pLbProtocol->setAlignment(Qt::AlignLeft);
    m_pLbProtocol->move(m_dpi.adj(QPoint(160, 380)));

    m_pBtnCustom = new QPushButton(QStringLiteral("自定义安装"), this);
    m_pBtnCustom->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
    m_pBtnCustom->setObjectName("m_pBtnCustom");
    m_pBtnCustom->move(m_dpi.adj(QPoint(484, 378)));
    m_pBtnCustom->setIconSize(m_dpi.adj(QSize(20, 20)));
    m_pBtnCustom->setIcon(QIcon(":/img/down"));

    m_pEdtInstallPath = new QLineEdit(m_sInstallPath, this);
    m_pEdtInstallPath->setObjectName("m_pEdtInstallPath");
    m_pEdtInstallPath->setReadOnly(true);
    m_pEdtInstallPath->move(m_dpi.adj(QPoint(30, 414)));
    m_pEdtInstallPath->setVisible(false);

    m_pBtnSelectPath = new QPushButton(QStringLiteral("浏览"), this);
    m_pBtnSelectPath->setObjectName("m_pBtnSelectPath");
    m_pBtnSelectPath->move(m_dpi.adj(QPoint(502, 414)));
    m_pBtnSelectPath->setVisible(false);

    connect(m_pBtnClose, &QPushButton::clicked, this, &ZenoSetup::slot_close);
    connect(m_pBtnInstall, &QPushButton::clicked, this, &ZenoSetup::slot_install);
    connect(m_pBtnCustom, &QPushButton::clicked, this, &ZenoSetup::slot_selectInstallPath);
    connect(m_pBtnSelectPath, &QPushButton::clicked, this, &ZenoSetup::slot_setInstallPath);

    connect(m_pLbProtocol, &QLabel::linkActivated, [=](QString url) {
        QDesktopServices::openUrl(QUrl(url));
    });
}

void ZenoSetup::initUpdateUI()
{
    m_pBtnClose->setVisible(false);

    m_pBtnInstall->setVisible(false);

    m_pCbAgree->setVisible(false);
    m_pLbProtocol->setVisible(false);
    m_pBtnCustom->setVisible(false);
    m_pBtnCustom->setIcon(QIcon(":/img/down"));

    resize(m_dpi.adj(QSize(600, 426)));
    m_bVisible = false;
    m_pEdtInstallPath->setVisible(false);
    m_pBtnSelectPath->setVisible(false);

//    qCustomDebug << m_pEdtInstallPath->text();
//    m_pThreadDownload = new BKInstallThread(m_pEdtInstallPath->text());
//    connect(m_pThreadDownload, &BKInstallThread::netReqFinish, this, &CloudPCUpDate::slt_netReqFinish);
//    m_pThreadDownload->start();

    pLbTip->setVisible(true);
    m_pPbDownload->setVisible(true);
    m_pLbProgress->setVisible(true);

    pLbTip->setText(QStringLiteral("正在更新 ") + QGuiApplication::applicationName());
}

void ZenoSetup::createLinks()
{
    QString srcRunFile = m_pEdtInstallPath->text() + "bin/zenoedit.exe";
    QString srcUninstallFile = m_pEdtInstallPath->text() + "bin/uninstall.exe";
    winsetup_create_desktop_link("ZENO",srcRunFile.toLocal8Bit().data());
    winsetup_create_menu_link("ZENO",srcRunFile.toLocal8Bit().data(),srcUninstallFile.toLocal8Bit().data());
}

void ZenoSetup::createUninstallInfo()
{
    QString srcRunFile = m_pEdtInstallPath->text() + "bin/zenoedit.exe";
    QString srcUninstallFile = m_pEdtInstallPath->text() + "bin/uninstall.exe";
//    auto version = QString("%1.%2.%3.%4") .arg(BKRC_MAJOR) .arg(BKRC_MINOR) .arg(BKRC_PATCH).arg(BKRC_VCODE).toLocal8Bit();
    auto version = "1.0.0.1";
    auto install_date = QDateTime::currentDateTime().toString("yyyy-MM-dd").toLocal8Bit();
    auto display_icon = srcRunFile.toLocal8Bit();
    auto uninstall_path = srcUninstallFile.toLocal8Bit();
    winsetup_app_info appinfo;
    appinfo.display_version = version;
    appinfo.display_name = "EveryLinks";
    appinfo.install_date = install_date.data();
    appinfo.display_icon = display_icon.data();
    appinfo.app_size = 330 * 1024;
    appinfo.uninstall_path = uninstall_path.data();
    winsetup_put_app_info("ZENO",&appinfo);
    QSettings set("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO",QSettings::NativeFormat);
    set.setValue("path", m_pEdtInstallPath->text());
    auto cn = set.value("channel").toString();
}

void ZenoSetup::switchFinish()
{
    m_pBtnClose->setVisible(true);
    m_pBtnInstall->setVisible(true);
    m_pBtnInstall->setText(QStringLiteral("立即打开"));
    m_pBtnInstall->move(m_dpi.adj(QPoint(210, 273)));
    m_pPbDownload->setVisible(false);
    m_pLbProgress->setVisible(false);
    pLbTip->setVisible(false);
    m_pLbSplit->setVisible(false);
    m_pLbBackImg->resize(m_dpi.adj(QSize(600, 426)));
    m_pLbBackImg->setPixmap(QPixmap(":/img/fulfill"));
}

void ZenoSetup::slot_close()
{
    if (m_pBtnInstall->text() == QStringLiteral("立即安装"))
    {
//            int ret = BkUConfirmWnd::makeAndShow(QStringLiteral("确定要取消安装并退出吗?"),this);
//            if (ret == QDialog::Accepted)
                this->close();
    }
    else
    {
        this->close();
    }
}

void ZenoSetup::slot_install()
{
    if (m_pBtnInstall->text() == QStringLiteral("立即安装"))
    {
        if (!m_pCbAgree->isChecked())
        {
            ZenoDialog *tip = new ZenoDialog(ZenoDialog::CheckTip,this);
            auto pos = m_pCbAgree->pos();
            pos.setX(pos.x() - m_dpi.adj(20));
            pos.setY(pos.y() - tip->height() - m_dpi.adj(20));
            tip->move(pos);
            tip->shwo(QStringLiteral("请勾选同意《许可及服务协议》和《隐私保护协议》"));
            return;
        }
        m_pBtnClose->setVisible(false);
        m_pBtnInstall->setVisible(false);
        m_pCbAgree->setVisible(false);
        m_pLbProtocol->setVisible(false);
        m_pBtnCustom->setVisible(false);
        m_pBtnCustom->setIcon(QIcon(":/img/down"));

        resize(600, 426);
        m_bVisible = false;
        m_pEdtInstallPath->setVisible(false);
        m_pBtnSelectPath->setVisible(false);

//        qCustomDebug << m_pEdtInstallPath->text();
//        m_pThreadDownload = new BKInstallThread(m_pEdtInstallPath->text());
//        connect(m_pThreadDownload, &BKInstallThread::netReqFinish, this, &CloudPCUpDate::slt_netReqFinish);
//        m_pThreadDownload->start();
        QTimer::singleShot(3000,this,[=](){
            createLinks();
            createUninstallInfo();
            switchFinish();
        });

        pLbTip->setVisible(true);
        m_pPbDownload->setVisible(true);
        m_pLbProgress->setVisible(true);
    }
    else
    {
        QString runpath = QString("\"%1\"").arg(m_pEdtInstallPath->text() + "bin/zenoedit.exe");
        QString rundir = m_pEdtInstallPath->text().replace("/", "\\");
        QString runParam = "";
        ShellExecuteW(NULL, L"open", runpath.toStdWString().c_str(), runParam.toStdWString().c_str(), rundir.toStdWString().c_str(), SW_SHOWNORMAL);
        this->close();
    }
}

void ZenoSetup::slot_selectInstallPath()
{
    if (!m_bVisible)
    {
        resize(m_dpi.adj(QSize(600, 466)));
        m_bVisible = true;
        m_pEdtInstallPath->setVisible(true);
        m_pBtnSelectPath->setVisible(true);
        m_pBtnCustom->setIcon(QIcon(":/img/up"));
    }
    else
    {
        resize(m_dpi.adj(QSize(600, 426)));
        m_bVisible = false;
        m_pEdtInstallPath->setVisible(false);
        m_pBtnSelectPath->setVisible(false);
        m_pBtnCustom->setIcon(QIcon(":/img/down"));
    }
}

void ZenoSetup::slot_setInstallPath()
{
    QString tmpPath = QFileDialog::getExistingDirectory(this, QStringLiteral("选择安装路径"), QString::fromStdWString(m_sSysPath));
    if (tmpPath.isEmpty())
        return;
    m_pEdtInstallPath->setText(tmpPath + "/" + QGuiApplication::applicationName() + "/");
}

