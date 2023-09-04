#include "zenodialog.h"
#include "Dpi.h"
#include <QTimer>

ZenoDialog::ZenoDialog(DialogType iType, QWidget *parent)
    :QDialog(parent), m_iDialogType(iType)
{
    setWindowFlags(Qt::FramelessWindowHint);
//    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    if(m_iDialogType == CheckTip){
        setObjectName("CheckTip");
    }


}

void ZenoDialog::shwo(const QString &msg)
{
    QFont font;
    font.setPixelSize(Dpi().adj(14));
    QFontMetrics fm(msg);
    QRect rect = fm.boundingRect(msg);
    int margin = Dpi(screen()).adj(30);
    resize(rect.width() + margin,rect.height() + margin);

    QLabel* lb1 = new QLabel(msg);
    lb1->setFont(font);
    lb1->setAlignment(Qt::AlignCenter);

    QHBoxLayout* lay = new QHBoxLayout;
    lay->setMargin(0);
    lay->addWidget(lb1);

    setLayout(lay);

    QTimer::singleShot(3000,this,&ZenoDialog::close);
    QDialog::exec();
}
