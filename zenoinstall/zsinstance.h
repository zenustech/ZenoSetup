#ifndef ZSINSTANCE_H
#define ZSINSTANCE_H

#include <QObject>
#include <QMutex>
#include "zsnetthread.h"

class ZsInstance : public QObject
{
    Q_OBJECT
public:
    static ZsInstance* Instance();
    static void ReleaseInstance();

    /*! @function
    ********************************************************************************
    函数名   : NetRequest
    功能     : 网络请求
    参数     : [IN] id        :发送请求方的唯一标识ID
              [IN] type		  :网络请求类型 0:netPost 1:getImage 2:getSpee 3:startGame 4:netGet 5:KillProcess()
              [IN] url		  :网络请求链接
              [IN] reqHeader  :网络请求头
              [IN] reqData    :网络请求参数
    返回值   : 无
    回调函数 ：(netReqFinish)(netUploadProgress)
    *******************************************************************************/
    Q_INVOKABLE void NetRequest(const QString& id, int type, QString url, QVector<ZS_NET_REQ_INFO> reqHeader, QByteArray reqData);

private slots:
    void slot_netReqFinish(const QString& data, const QString& id);
    void slot_netReqProgress(qint64 bytesReceived, qint64 bytesTotal);

signals:
    void sig_netReqFinish(const QString& data, const QString& id);
    void sig_netReqProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    ZsInstance(QObject *parent = nullptr);
    ~ZsInstance();

    static ZsInstance* _instance;
    static QMutex  m_instanceMutex;
    QList<QThread*> m_pThreadList;

};

#endif // ZSINSTANCE_H
