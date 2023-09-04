#ifndef ZENOSETUP_H
#define ZENOSETUP_H

#include <QtWidgets>
#include "Dpi.h"

class ZenoSetup : public QWidget
{
    Q_OBJECT

public:
    enum RunType{None, Update, Report};

    ZenoSetup(RunType iType = RunType::None, QWidget *parent = nullptr);
    ~ZenoSetup();

private:
    void initNoneUI();
    void initUpdateUI();
//    void install();
//    void update();

    void createLinks();
    void createUninstallInfo();
    void switchFinish();

private slots:
    void slot_close();
    void slot_install();
    void slot_selectInstallPath();
    void slot_setInstallPath();
//    void slt_netReqFinish(const int type, const QString& data);

protected:
    void mousePressEvent(QMouseEvent* event)
    {
        m_Move_point = event->pos();
    }

    void mouseReleaseEvent(QMouseEvent* /*event*/)
    {
        m_Move_point = QPoint();
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        if (!m_Move_point.isNull())
        {
            move(event->globalPos() - m_Move_point);
        }
    }

private:
    QPoint m_Move_point;

    Dpi m_dpi;

    QLabel* m_pLbBackImg;

    QLabel* m_pLbSplit;

    QLabel* m_pLbTitleIcon;
    QPushButton* m_pBtnClose;

    QPushButton* m_pBtnInstall;

    QCheckBox* m_pCbAgree;
    QLabel* m_pLbProtocol;
    QPushButton* m_pBtnCustom;

    QLineEdit* m_pEdtInstallPath;
    QPushButton* m_pBtnSelectPath;


    QLabel* pLbTip;
    QLabel* m_pLbProgress;
    QProgressBar* m_pPbDownload;

//	BKInstallThread* m_pThreadDownload;

    bool m_bVisible = false;

    RunType m_iRunType;
    QString m_sInstallPath;
    TCHAR m_sSysPath[MAX_PATH] = { 0 };
};
#endif // ZENOSETUP_H
