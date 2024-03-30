#ifndef WORK1THREADS_H
#define WORK1THREADS_H

#include "work1.h"

#include <QObject>
#include <QThread>

class Work1Thread : public QThread
{
    UsbDriveModel* _m;
public:
    void setM(UsbDriveModel* v){_m = v;}

private:
    void run() override{
        Work1::GetUsbDrives3(_m);
    }
};

#endif // WORK1THREADS_H
