#ifndef WORK1_H
#define WORK1_H
#include "helpers/processhelper.h"
#include <QCommandLineParser>
#include <QMap>
#include <QStringList>
#include <QObject>
#include "typekey.h"

struct PartitionModel{
    QString partPath;
    QString label;
    QString project;

    QString toString() const;
};

struct UsbDriveModel{
    QString devicePath;
    QString usbPath;

    QList<PartitionModel> partitions;

    QString toString() const;
    bool isValid();
};

class Work1
{
public:
    struct Params{
    public:
        QString ofile;
        QString path;
        QString passwd;
        bool force;
        bool usbSelect;
        bool isQuery;

        void Parse(QCommandLineParser *p)
        {
            ofile = p->value("o");
            path = p->value("p");
            passwd = p->value("s");
            force = p->isSet("f");
            usbSelect = p->isSet("b");
            isQuery = p->isSet("q");
        }
    };


public:
    enum Result : int{
        OK=0,
        ISEMPTY,
        NO_OUTFILE,
        NO_LASTREC,
        CANNOT_UNMOUNT,
        NO_UNITS,
        NOT_CONFIRMED,
        DD_ERROR,
        NO_CHECK0,
        NO_CHECK1,
        CHECKSUM_ERROR,
        NO_PASSWD,
        NO_USBDRIVE,
        TOOMUCH_USBDRIVE
    };
public:
    static int doWork();
    static Params _params;

    static QList<UsbDriveModel> GetUsbDrives();
    static UsbDriveModel SelectUsbDrive(const QList<UsbDriveModel>& usbdrives);
    static int GetLastRecord(const QString &drive, int* units);
    static int dd(const QString &src, const QString &dst, int bs, int count, QString *msg);
    static bool ConfirmYes();
    static QString GetFileName(const QString &src);
    static QStringList MountedParts(const QString &src);
    static bool UmountParts(const QStringList &src);
    static ProcessHelper::Output Execute2(const QString& cmd);
    static QString BytesToString(double b);
    static int sha256sumFile(const QString &fn);
    static QString getSha(const QString &fn);
    static int sha256sumDevice(const QString &fn, int r, qint64 b, const QString &sha_fn);
    static QString GetUsbPath(const QString& dev);

    static QList<PartitionModel> GetPartitions(const QString& dev);
    static QString GetMountPoint(const QString& dev, const QString& label);
    
    //static QString Mount(const QString& partPath);
    //static void UMount(const QString& mountPoint);
    static QString MkMountPoint();
    static void RmMountPoint(const QString& path);
private:
    static QString GetProject(const QString& path);
};

#endif // WORK1_H
