#ifndef WORK1_H
#define WORK1_H


#include <QStringList>

struct Work1Params{
public:
    QString tmpfile;
    QString ofile;
    QString projname;
};

class Work1
{
public:
    Work1();
    static int doWork();
    static Work1Params params;

    static QStringList GetUsbDrives();
};

#endif // WORK1_H
