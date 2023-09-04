#include "uninstall.h"
#include "../winsetup/winsetup.h"
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj_core.h>


uninstall::uninstall(QWidget* parent)
	: QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);

	QVBoxLayout* pMainLayout = new QVBoxLayout;

	//title widget
	QWidget* pTitleWid = initTitleWid();
	m_stackWid = new QStackedWidget;

	//first page.
	QWidget* pFirstWid = new QWidget;
	QVBoxLayout* pVLayout = new QVBoxLayout;
	{
        m_pLbBackImg1 = new QLabel(this);
        m_pLbBackImg1->setPixmap(QPixmap(":/img/unins"));
        m_pLbBackImg1->setAlignment(Qt::AlignCenter);

        m_pLbTip1 = new QLabel(this);
        m_pLbTip1->setObjectName("m_pLbTip1");
        m_pLbTip1->setText(QStringLiteral("确定卸载Zeno？"));
        m_pLbTip1->setAlignment(Qt::AlignCenter);

        m_pBtnCancel = new QPushButton(QStringLiteral("取消"), this);
        m_pBtnCancel->setObjectName("m_pBtnCancel");
        connect(m_pBtnCancel, &QPushButton::clicked, [=] {this->close(); });

        m_pBtnUninstall = new QPushButton(QStringLiteral("确认"), this);
        m_pBtnUninstall->setObjectName("m_pBtnUninstall");
        connect(m_pBtnUninstall, &QPushButton::clicked, [=] {
            m_stackWid->setCurrentIndex(1);

            QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/").append(QStringLiteral("ZENO.lnk"));
            QString startMenuPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation).append("/").append(QStringLiteral("ZENO"));
            QFile::remove(desktopPath);
            QFile::remove(startMenuPath + "/" + QStringLiteral("ZENO.lnk"));
            QFile::remove(startMenuPath + "/" + QStringLiteral("卸载.lnk"));
            QDir dir(startMenuPath);
            qDebug() << startMenuPath << dir.rmdir(startMenuPath);
            //QProcess::execute("zeno-bc.exe --uninstall",{});

            QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\ZENO", QSettings::NativeFormat);
            QString path = settings.value("path").toString();
            //path = QApplication::applicationDirPath();
            qDebug() << path;
            dir.setPath(path);
            QStringList filter;
            QList<QFileInfo> fileInfo = QList<QFileInfo>(dir.entryInfoList(filter));
            m_pPbDownload->setMaximum(fileInfo.count());
            for (int i = 0; i < fileInfo.size(); i++)
            {
                //QFile::remove(fileInfo.at(i).absolutePath());
                m_pPbDownload->setValue(i + 1);
            }
            qDebug() << path << dir.removeRecursively();
            settings.clear();
            winsetup_rm_app_info("ZENO");

            m_stackWid->setCurrentIndex(2);
        });

        QHBoxLayout* pButtonLayout = new QHBoxLayout;
        pButtonLayout->addWidget(m_pBtnCancel);
        pButtonLayout->addWidget(m_pBtnUninstall);

        pVLayout->addWidget(m_pLbBackImg1);
        pVLayout->addWidget(m_pLbTip1);
        pVLayout->addLayout(pButtonLayout);
    }
    pFirstWid->setLayout(pVLayout);

    //second page.
    QWidget* pSecondWid = new QWidget;
    pVLayout = new QVBoxLayout;
    {
        m_pPbDownload = new QProgressBar(this);
        m_pPbDownload->setObjectName("m_pPbDownload");
        m_pPbDownload->setMinimum(0);
        m_pPbDownload->setMaximum(0);
        m_pPbDownload->setTextVisible(false);
        m_pPbDownload->setVisible(false);

        m_pLbTip2 = new QLabel(this);
        m_pLbTip2->setObjectName("m_pLbTip1");
        m_pLbTip2->setText(QStringLiteral("正在卸载中..."));
        m_pLbTip2->setAlignment(Qt::AlignCenter);

        pVLayout->addWidget(m_pPbDownload);
        pVLayout->addWidget(m_pLbTip2);
    }
    pSecondWid->setLayout(pVLayout);

    //last page.
    QWidget* pLastWid = new QWidget;
    pVLayout = new QVBoxLayout;
    {
        m_pLbBackImg3 = new QLabel(this);
        //m_pLbBackImg->setObjectName("m_pLbBackImg");
        m_pLbBackImg3->setPixmap(QPixmap(":/img/unins_over"));
        m_pLbBackImg3->setAlignment(Qt::AlignCenter);

        m_pLbTip2 = new QLabel(this);
        m_pLbTip2->setObjectName("m_pLbTip2");
        m_pLbTip2->setText(QStringLiteral("卸载完成"));
        m_pLbTip2->setAlignment(Qt::AlignCenter);

        m_pLbTip3 = new QLabel(this);
        m_pLbTip3->setObjectName("m_pLbTip3");
        m_pLbTip3->setText(QStringLiteral("感谢您的选择，期待下次再见"));
        m_pLbTip3->setAlignment(Qt::AlignCenter);

        m_pBtnOK = new QPushButton(QStringLiteral("下次再会"), this);
        m_pBtnOK->setObjectName("m_pBtnOK");
        connect(m_pBtnOK, &QPushButton::clicked, [=] { close(); });

        pVLayout->addWidget(m_pLbBackImg3, 0, Qt::AlignCenter);
        pVLayout->addWidget(m_pLbTip2, 0, Qt::AlignCenter);
        pVLayout->addWidget(m_pLbTip3, 0, Qt::AlignCenter);
        pVLayout->addWidget(m_pBtnOK, 0, Qt::AlignCenter);
    }
    pLastWid->setLayout(pVLayout);

    m_stackWid->addWidget(pFirstWid);
    m_stackWid->addWidget(pSecondWid);
    m_stackWid->addWidget(pLastWid);

    pMainLayout->addWidget(pTitleWid);
    pMainLayout->addWidget(m_stackWid);
    pMainLayout->setContentsMargins(0, 0, 0, m_dpi.adj(20));
    setLayout(pMainLayout);

    m_stackWid->setCurrentIndex(0);
}

QWidget* uninstall::initTitleWid()
{
    QWidget* pWid = new QWidget;

    //custom titlebar
    m_pLbTitleIcon = new QLabel(this);
    m_pLbTitleIcon->setObjectName("m_pLbTitleIcon");
    m_pLbTitleIcon->move(m_dpi.adj(QPoint(20, 15)));

    m_pLbTitle = new QLabel(QStringLiteral("Zeno卸载"), this);
    m_pLbTitle->setObjectName("m_pLbTitle");

    m_pBtnClose = new QPushButton(this);
    m_pBtnClose->setObjectName("m_pBtnClose");
    connect(m_pBtnClose, &QPushButton::clicked, [=] { this->close(); });

    QHBoxLayout* pTitleLayout = new QHBoxLayout;
    pTitleLayout->addWidget(m_pLbTitleIcon, 0, Qt::AlignCenter);
    pTitleLayout->addSpacing(m_dpi.adj(4));
    pTitleLayout->addWidget(m_pLbTitle, 0, Qt::AlignCenter);
    pTitleLayout->addStretch();
    pTitleLayout->addWidget(m_pBtnClose, 0, Qt::AlignCenter);
    pTitleLayout->setContentsMargins(12, 12, 12, 12);

    pWid->setLayout(pTitleLayout);
    pWid->setAutoFillBackground(false);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#2F2F2F"));
    pWid->setPalette(pal);

    return pWid;
}
