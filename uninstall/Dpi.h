#ifndef DPI_H
#define DPI_H

#include <QScreen>
#include <QGuiApplication>
#include <QSize>
#include <QPoint>
#include <QString>
#include <QFile>
#include <QRegExp>
#include <QWidget>

class Dpi {
public:
    Dpi(QScreen *screen = QGuiApplication::primaryScreen(),qreal designDpi = 96.0) {
        init(screen,designDpi);
    }

    void init(QScreen *screen,qreal designDpi) {
        _runDpi = screen->logicalDotsPerInch();
        _scale = _runDpi / designDpi;
        _limitSize = screen->availableSize();
    }

    int adj(int v) {return static_cast<int>(v * _scale);}
    qreal adj(qreal v) {return v * _scale;}
    QSize adj(QSize v) {
        return QSize(qMin(adj(v.width()),_limitSize.width()),
                     qMin(adj(v.height()),_limitSize.height())
                     );
    }
    QPoint adj(QPoint v) {
        return QPoint(qMin(adj(v.x()),_limitSize.width()),
                      qMin(adj(v.y()),_limitSize.height())
                      );
    }

    qreal current() {return _runDpi;}

    QString adjStyle(QString style) {
        QString ret;
        QRegExp re("(\\d+)px");
        int pos = 0;
        int lastPos = 0;
        while (pos >= 0) {
            pos = re.indexIn(style,pos);
            if (pos >= 0) {
                int v = re.cap(1).toInt();
                int v2 = adj(v);
                ret.append(style.mid(lastPos,pos - lastPos));
                ret.append(QString::number(v2) + "px");
                pos += re.matchedLength();
                lastPos = pos;
            }
        }

        if (lastPos < style.size())
            ret.append(style.right(style.size() - lastPos));

        return ret;
    }


    QString loadStyle(const QString &path) {
        QFile f(path);
        if (f.open(QFile::ReadOnly)) {
            return adjStyle(f.readAll());
        }
        return QString();
    }

    void loadStyle(const QString &path,QWidget *target) {
        target->setStyleSheet(loadStyle(path));
    }

    void loadStyleByStr(const QString &css,QWidget *target) {
        target->setStyleSheet(adjStyle(css));
    }

private:
    qreal _scale;
    qreal _runDpi;
    QSize _limitSize;
};

#endif // DPI_H
