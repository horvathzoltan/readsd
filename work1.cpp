#include "work1.h"
#include "helpers/logger.h"
#include "helpers/textfilehelper.h"
#include "helpers/processhelper.h"
//#include "sqlhelper.h"
//#include "settings.h"
//#include "environment.h"
#include <QTextStream>
#include <QVariant>
//#include <iostream>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

Work1::Params Work1::_params;

// ./readsd -p /media/pi/butyok2/clone/ -o img56 -s Aladar123 -f
// ./readsd -p /home/zoli/aaa/ -o img56 -s Aladar123 -f

int Work1::doWork()
{
    //bool ok;
    //qint64 size = QStringLiteral("15931539456").toLongLong(&ok);
    //bool hasCard = ok && size>0;
    // "15931539456"
    //zInfo("size:"+QString::number(size));
    bool no_password = _params.passwd.isEmpty();
    if(no_password){
        _params.passwd = GetFileName("Add sudo password.");
    }

    if(_params.passwd.isEmpty()) return NO_PASSWD;

    ProcessHelper::SetPassword(_params.passwd);
    // TODO 1. megtudni a kártyát
    // lsblk -dro name,path,type,tran,rm,vendor,model,phy-sec,mountpoint
    // ha több van, lista, választani
    // felírja:
    // sudo fdisk -l /dev/sdh
    // utolsó partíció utolsó szektora +1-ig írunk
    // https://stackoverflow.com/questions/22433257/windows-dd-esque-implementation-in-qt-5
    // mngm ~$ sudo dd if=/dev/sdb of=backup.img bs=512 count=15759360 conv=fsync
    // sudo dd of=/dev/sdm bs=512 if=/media/zoli/mentes/QT_raspi_anti/raspicam3.img status=progress oflag=sync
    //if(params.ofile.isEmpty()) return NOOUTFILE;
    //if(!params.ofile.endsWith(".img")) params.ofile+=".img";
    QString working_path = _params.path;
    if(working_path.isEmpty()) working_path = QDir::currentPath();

    // megszerzi a block deviceokat
    auto usbDrives = GetUsbDrives();

    if(usbDrives.isEmpty()) return ISEMPTY;


    UsbDriveModel usbdrive;

    if(usbDrives.count()>1){
        usbdrive = SelectUsbDrive(usbDrives);
    } else if(usbDrives.count()==1){
        usbdrive = usbDrives[0];
        zInfo("usbdrive: "+usbdrive.toString());
    } else{
        usbdrive=UsbDriveModel();
    }

    if(!usbdrive.isValid()) return NO_USBDRIVE;

    int r=55;
    auto lastrec = GetLastRecord(usbdrive.devicePath, &r);
    if(lastrec==-1) return NO_LASTREC;
    if(r==0) return NO_UNITS;


    qint64 b = (qint64)r*(qint64)lastrec;
    auto b_txt = BytesToString((double)b);
    zInfo(QStringLiteral("reading: %1 bytes (%2)").arg(b).arg(b_txt))

    QStringList mountedparts = MountedParts(usbdrive.devicePath);
    if(!mountedparts.isEmpty() && !UmountParts(mountedparts)) return CANNOT_UNMOUNT;

    zInfo("Last record on " + usbdrive.devicePath + ": "+QString::number(lastrec+1));
    QString msg;
    bool confirmed = false;

    if(_params.ofile.isEmpty())
    {
        confirmed = true;
        _params.ofile = GetFileName("Add output file name.");
        //zInfo("beírta a kezével");//reméljük azzal írta be
    }

    if(_params.ofile.isEmpty()) return NO_OUTFILE;
    if(!_params.ofile.endsWith(".img")) _params.ofile+=".img";
    if(_params.force){
        confirmed = true;
    }
    if(!confirmed) confirmed = ConfirmYes();
    if(!confirmed) return NOT_CONFIRMED;

    auto fn =  QDir(working_path).filePath(_params.ofile);
    QString shaFn = fn+".sha256";
    auto sha_tmp_fn = QDir(working_path).filePath("temp.sha256");
    //QString csvFn = fn+".csv";

    // ha van már ilyen file, temp és sha, töröljük
    TextFileHelper::Delete(fn);
    TextFileHelper::Delete(sha_tmp_fn);
    TextFileHelper::Delete(shaFn);
    //TextFileHelper::Delete(csvFn);

    /*QString lr = QString::number(lastrec)+','+QString::number(r);
    QString csvfn = fn;
    csvfn.replace(".img",".csv");
    TextFileHelper::Save(lr, csvfn);*/


    auto ddr = dd(usbdrive.devicePath, fn, r, lastrec+1, &msg);
    if(ddr) return DD_ERROR;
    sha256sumFile(fn);

    // kiszámoljuk a partíció sha-ját a tempbe

    sha256sumDevice(usbdrive.devicePath, r, lastrec+1, sha_tmp_fn);
    QString sha_tmp = getSha(sha_tmp_fn);

    TextFileHelper::Delete(sha_tmp_fn);
    //TextFileHelper::Delete(csvfn);

    // kiszámoljuk a kiírt img file sha-ját a fn+".sha256"-be
    if(sha_tmp.isEmpty()) return NO_CHECK0;


    QString sha_img = getSha(shaFn);
    if(sha_tmp.isEmpty()) return NO_CHECK1;

    zInfo(QStringLiteral("sha_tmp: ")+sha_tmp)
    zInfo(QStringLiteral("sha_img: ")+sha_img)
        if(sha_tmp!=sha_img) return CHECKSUM_ERROR;

    return OK;
}

QString Work1::getSha(const QString& fn){
    QString fn2 = TextFileHelper::GetFileName(fn);
    zInfo("sha256sum from: "+fn);
    QString txt;
    bool ok = TextFileHelper::Load(fn, &txt);
    if(!ok) return QString();
    if(txt.isEmpty()) return QString();
    auto ix0 = txt.indexOf(' ');
    if(ix0<0) return QString();
    return txt.left(ix0);
}

int Work1::sha256sumDevice(const QString& fn, int r, qint64 b, const QString& sha_fn)
{
    auto sha_fn2 = TextFileHelper::GetFileName(sha_fn);
    zInfo("sha256sum on dev: "+fn+" -> "+sha_fn2);
    auto cmd_dd = QStringLiteral("dd bs=%2 count=%3 if=%1 status=progress | sha256sum").arg(fn).arg(r).arg(b);

    //zInfo("cmd_dd:"+cmd_dd);
    //auto m = ProcessHelper::Model::ParseAsSudo(cmd1, params.passwd);
    //m[1].showStdErr=false;
    //m.append({.cmd="sha256sum", .args = {}, .timeout=-1, .showStdErr = false });

    auto out = ProcessHelper::ShellExecuteSudo(cmd_dd);

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;

    TextFileHelper::Save(out.stdOut, sha_fn);
    zInfo("ok");//:" + sha_fn);
    return 0;
}

QString Work1::GetUsbPath(const QString &dev)
{
    auto cmd = QStringLiteral("udevadm info -q path");
    cmd+=" "+dev;
    //auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::ShellExecute(cmd);
    if(out.exitCode) return "";
    if(out.stdOut.isEmpty()) return "";

    QString e = "";
    int ix0 = out.stdOut.indexOf("/usb");
    if(ix0>=0){
        int ix1 = out.stdOut.indexOf("/host", ix0);
        if(ix1>=0){
            e = out.stdOut.mid(ix0, ix1-ix0);
        }
    }

    return e;
}

QStringList Work1::GetPartLabels(const QString &dev)
{
    auto cmd = QStringLiteral("lsblk -r %1 -o NAME,path,LABEL,Type").arg(dev);
    //auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::ShellExecute(cmd);
    if(out.exitCode) return QStringList();
    if(out.stdOut.isEmpty()) return QStringList();

    QStringList e;
    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;

        auto j=i.split(' ');

        bool isPart = j[3]=="part";

        if(isPart){
            e.append(j[2]);
        }
    }
    return e;
}

int Work1::sha256sumFile(const QString& fn)
{
    zInfo("sha256sum on file");//:"+fn);
    QString cmd = QStringLiteral("sha256sum %1").arg(fn);
    auto out = ProcessHelper::ShellExecute(cmd);

    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;

    auto sha_fn = fn+".sha256";
    TextFileHelper::Save(out.stdOut, sha_fn);
    zInfo("ok");//:"+sha_fn);
    return 0;
}


QString Work1::BytesToString(double b)
{
    double gb = b;

    if(gb<1024) return QStringLiteral("%1 Bytes").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 KB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 MB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    if(gb<1024) return QStringLiteral("%1 GB").arg(gb, 0, 'f', 1);
    gb = gb/1024;
    return QStringLiteral("%1 TB").arg(gb, 0, 'f', 1);
}

int Work1::GetLastRecord(const QString& drive, int* units)
{
    //sudo partx -r /dev/sdh
    /*
     * zoli@ubul:~$ sudo partx -r /dev/sdh
NR START END SECTORS SIZE NAME UUID
1 8192 532479 524288 256M  5e3da3da-01
2 532480 18702335 18169856 8,7G  5e3da3da-02

*/
    //auto cmd = QStringLiteral("sudo fdisk -l %1").arg(drive);
    auto cmd = QStringLiteral("partx -rb %1").arg(drive);
    //auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    //m2[0].showStdErr= false;
    //m2[1].showStdErr= false;
    auto out = ProcessHelper::ShellExecuteSudo(cmd);

    int lastrec = -1;
    if(out.exitCode) return -1;
    if(out.stdOut.isEmpty()) return -1;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;

        auto j = i.split(' ');
        bool isok;
        auto k = j[2].toInt(&isok);
        if(isok && k>lastrec)
        {
             lastrec = k;
             if(units!=nullptr)
             {
                 quint64 sectors = j[3].toULongLong();
                 quint64 size = j[4].toULongLong();
                 int r = size / sectors;
                 *units = r;
             }
        }

//        if(units!=nullptr && i.startsWith(QStringLiteral("Units:")))
//        {
//            auto j = i.split('=');

//            auto k = j[1].split(' ');
//            bool isok;
//            auto u = k[1].toInt(&isok);
//            if(isok) *units = u;
//        }
    }

    return lastrec;

}

// 1000000000
// udevadm info -q path /dev/sdg
QList<UsbDriveModel> Work1::GetUsbDrives()
{
    QList<UsbDriveModel> e;

    auto cmd = QStringLiteral("lsblk -dbro name,path,type,tran,rm,vendor,model,phy-sec,size,mountpoint");
    //auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::ShellExecuteSudo(cmd);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        zInfo("devices:"+i)
        if(i.isEmpty()) continue;
        auto j=i.split(' ');

        bool isBoot = j[9]=='/'||j[9]=="/boot";

        if(isBoot) continue;

        bool isRemovableDisk = j[2]==QLatin1String("disk")&&
                     j[4]==QLatin1String("1");

        if(!isRemovableDisk) continue;

        bool isUsb = j[3]==QLatin1String("usb");

        bool isMmc = j[0].startsWith("mmc");

        bool ok;
        qint64 size = j[8].toLongLong(&ok);
        bool hasCard = ok && size>0;
// "15931539456"
        if(isRemovableDisk && (isUsb || isMmc) && hasCard){
            UsbDriveModel m;
            m.devicePath = j[1];
            m.usbPath = GetUsbPath(j[1]);
            m.partLabels = GetPartLabels(j[1]);
            e.append(m);
        }
    }

    return e;
}

UsbDriveModel Work1::SelectUsbDrive(const QList<UsbDriveModel> &usbdrives)
{
    int j = 1;
    for(auto&i:usbdrives) zInfo(QString::number(j++)+": "+i.toString());j--;
    zInfo("select 1-"+QString::number(j))

    QTextStream in(stdin);
    auto intxt = in.readLine();
    bool isok;
    auto ix = intxt.toInt(&isok);
    if(!isok) return UsbDriveModel();
    if(ix<1||ix>j) return UsbDriveModel();
    return usbdrives[ix-1];
}

bool Work1::ConfirmYes()
{
    zInfo("Say 'yes' to continue or any other to quit.")
    QTextStream in(stdin);
    auto intxt = in.readLine();
    return intxt.trimmed().toLower()=="yes";
}

auto Work1::GetFileName(const QString& msg) ->QString
{
    zInfo(msg)
    QTextStream in(stdin);
    QString txt = in.readLine();
    //    QString txt="kacaj";
    return txt;
}

QStringList Work1::MountedParts(const QString &src)
{
    QStringList e;
    auto cmd = QStringLiteral("mount -l");
    //auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    auto out = ProcessHelper::ShellExecuteSudo(cmd);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        auto k = i.split(' ');
        if(k[0].startsWith(src)) e.append(k[0]);
    }
    return e;
}

bool Work1::UmountParts(const QStringList &src)
{
    bool isok = true;
    for(auto&i:src)
    {
        auto cmd = QStringLiteral("umount %1").arg(i);
        //auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
        auto out = ProcessHelper::ShellExecuteSudo(cmd);

        if(out.exitCode) isok = false;
    }
    return isok;
}

int Work1::dd(const QString& src, const QString& dst, int bs, int count, QString *mnt)
{
    zInfo("copying...");//:"+src+"->"+dst );
//    QString e;
    //oflag sync
    auto cmd_dd = QStringLiteral("dd of=%1 bs=%3 count=%4 if=%2 status=progress conv=fdatasync").arg(dst,src).arg(bs).arg(count);
    //zInfo(cmd);
    //return 1;
    //auto m2 = ProcessHelper::Model::ParseAsSudo(cmd, params.passwd);
    auto out = ProcessHelper::ShellExecuteSudo(cmd_dd);//2
    if(out.exitCode) return 1;
    zInfo("copy ok. syncing...");
    auto cmd_sync = QStringLiteral("sync");//ProcessHelper::Model::Parse(QStringLiteral("sync"));
    ProcessHelper::ShellExecute(cmd_sync);//2
    if(mnt)*mnt = out.ToString();
    if(out.exitCode) return out.exitCode;
    if(out.stdOut.isEmpty()) return out.exitCode;
    return 0;
}
// length = 4
// maxix = 3
// 0123
// abc/

QString UsbDriveModel::toString() const
{
    QString n =  devicePath;

    QString usbn;
    int ix0 = usbPath.lastIndexOf('/');
    int maxix = usbPath.length()-1;

    if(ix0>=0 && ix0<maxix){
        usbn = usbPath.mid(ix0+1);
    } else{
        usbn =  usbPath;
    }

    QString labels="";
    bool hasParts = !partLabels.isEmpty();
    if(hasParts){
        labels = partLabels.join(',');
    }
    return n+": "+usbn+": "+labels;
}

bool UsbDriveModel::isValid()
{
    if(devicePath.isEmpty()) return false;
    if(usbPath.isEmpty()) return false;
    return true;
}
