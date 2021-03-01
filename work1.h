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
      OK=0,ISEMPTY,NOOUTFILE,NOLASTREC,CANNOTUNMOUNT
    };
public:
    Work1();
    static int doWork();
    static Work1Params params;

    static QStringList GetUsbDrives();    
    static QString SelectUsbDrive(const QStringList& usbdrives);
    static int GetLastRecord(const QString &drive, int*units);
    static int dd(const QString &src, const QString &dst, int bs, int count, QString *msg);
    static bool ConfirmYes();
    static QString GetFileName();
    static QStringList MountedParts(const QString &src);
    static bool UmountParts(const QStringList &src);
};

#endif // WORK1_H
