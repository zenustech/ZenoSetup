#ifndef ZENODIALOG_H
#define ZENODIALOG_H

#include <QtWidgets>

class ZenoDialog : public QDialog
{
    Q_OBJECT
public:
    enum DialogType{CheckTip, MsgTip, ConfirmTip};

    ZenoDialog(DialogType iType, QWidget* parent = nullptr);

    void shwo(const QString& msg);

private:
    DialogType m_iDialogType;

};

#endif // ZENODIALOG_H
