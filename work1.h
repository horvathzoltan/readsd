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
    enum Result : int{
      OK=0,ISEMPTY
    };
public:
    Work1();
    static int doWork();
    static Work1Params params;

    static QStringList GetUsbDrives();    
    static QString SelectUsbDrive(const QStringList& usbdrives);
    static int GetLastRecord(const QString &drive, int*units);
};

#endif // WORK1_H
