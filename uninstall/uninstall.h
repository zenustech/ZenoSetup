#pragma once

#include <QtWidgets>
#include "Dpi.h"

class uninstall : public QWidget
{
	Q_OBJECT

public:
	uninstall(QWidget* parent = Q_NULLPTR);

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
	QWidget* initTitleWid();

	QPoint m_Move_point;

	QLabel* m_pLbBackImg1;
	QLabel* m_pLbBackImg2;
	QLabel* m_pLbBackImg3;

	QLabel* m_pLbTitle;
	QLabel* m_pLbTitleIcon;
	QPushButton* m_pBtnClose;

	QLabel* m_pLbTip1;
	QLabel* m_pLbTip2;
	QLabel* m_pLbTip3;

	QPushButton* m_pBtnCancel;
	QPushButton* m_pBtnUninstall;

	QProgressBar* m_pPbDownload;
	QStackedWidget* m_stackWid;

	QPushButton* m_pBtnOK;

	QString m_prodir = "";

	Dpi m_dpi;
};
