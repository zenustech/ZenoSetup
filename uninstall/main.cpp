#include "uninstall.h"
#include "Dpi.h"
#include <QApplication>
#include <QTime>
#include <QMutex>
#include <QDir>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QProcess>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

#if 0
    if (argc == 1) {
        //in order to remove uninstall.exe itself.
        auto tpath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/ZENO-uninstall.exe";
        QFile::remove(tpath);
        QFile::copy(a.applicationFilePath(),tpath);
        QProcess::startDetached(tpath,{"--hello"});
        return 0;
    }
#endif

    a.setApplicationName("Zeno");
    a.setApplicationDisplayName(QStringLiteral("Zeno卸载程序"));

    a.setStyleSheet(Dpi().loadStyle(":/qss/default"));

	uninstall w;
	w.show();
	return a.exec();
}
