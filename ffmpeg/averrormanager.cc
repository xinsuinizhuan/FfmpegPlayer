#include "averrormanager.hpp"
#include "averror.h"

namespace Ffmpeg {

class AVErrorManager::AVErrorManagerPrivate
{
public:
    AVErrorManagerPrivate(AVErrorManager *q)
        : q_ptr(q)
    {}

    AVErrorManager *q_ptr;

    bool print = false;
    int max = 100;
    QVector<int> errorCodes{};
    AVError error;
};

void AVErrorManager::setPrint(bool print)
{
    d_ptr->print = print;
}

void AVErrorManager::setMaxCaches(int max)
{
    Q_ASSERT(max > 0);
    d_ptr->max = max;
}

void AVErrorManager::setErrorCode(int errorCode)
{
    d_ptr->errorCodes.append(errorCode);
    d_ptr->error.setErrorCode(errorCode);
    if (d_ptr->errorCodes.size() > d_ptr->max) {
        d_ptr->errorCodes.takeFirst();
    }
    if (d_ptr->print) {
        qWarning() << QString("Error[%1]:%2.")
                          .arg(QString::number(d_ptr->error.errorCode()),
                               d_ptr->error.errorString());
    }
    emit error(d_ptr->error);
}

QString AVErrorManager::lastErrorString() const
{
    return d_ptr->error.errorString();
}

QVector<int> AVErrorManager::errorCodes() const
{
    return d_ptr->errorCodes;
}

AVErrorManager::AVErrorManager(QObject *parent)
    : QObject{parent}
    , d_ptr(new AVErrorManagerPrivate(this))
{
    qRegisterMetaType<Ffmpeg::AVError>("Ffmpeg::AVError");
}

AVErrorManager::~AVErrorManager() {}

} // namespace Ffmpeg
