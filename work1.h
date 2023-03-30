#ifndef WORK1_H
#define WORK1_H
#include "helpers/processhelper.h"
#include <QCommandLineParser>
#include <QMap>
#include <QStringList>
#include <QObject>
#include "typekey.h"

struct Work1Params{
public:
    QString tmpfile;
    QString ofile;
    QString projname;
    QString workingpath;
    QString passwd;

    auto Parse(QCommandLineParser *p) -> Work1Params
    {
        Work1Params a;
        a.ofile = p->value(zkey(Work1Params::ofile));
        return a;
    }
};

class Work1
{
public:
    enum Result : int{
      OK=0,ISEMPTY,NOOUTFILE,NOLASTREC,CANNOTUNMOUNT,NOUNITS, NOTCONFIRMED, DDERROR,NOCHECK0,NOCHECK1,CHECKSUMERROR
    };
public:
    static int doWork();
    static Work1Params params;

    static QStringList GetUsbDrives();    
    static QString SelectUsbDrive(const QStringList& usbdrives);
    static int GetLastRecord(const QString &drive, int* units);
    static int dd(const QString &src, const QString &dst, int bs, int count, QString *msg);
    static bool ConfirmYes();
    static QString GetFileName();
    static QStringList MountedParts(const QString &src);
    static bool UmountParts(const QStringList &src);
    static ProcessHelper::Output Execute2(const QString& cmd);
    static QString BytesToString(double b);
    static int sha256sumFile(const QString &fn);
    static QString getSha(const QString &fn);
    static int sha256sumDevice(const QString &fn, int r, qint64 b, const QString &sha_fn);
};

#endif // WORK1_H
