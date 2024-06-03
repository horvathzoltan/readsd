#include "work1.h"
#include "work1threads.h"
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
#include <QElapsedTimer>

#include <helpers/mounthelper.h>
#include <helpers/processhelper.h>

Work1::Params Work1::_params;

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

    if(!_params.mountData.isEmpty()){
        auto usbDrive = GetUsbDrive(_params.mountData);
        zMessage("usbdrive:"+usbDrive.toString());
        return OK;
    }
    // megszerzi a block deviceokat
    auto usbDrives = GetUsbDrives();

    if(usbDrives.isEmpty()) return ISEMPTY;

    if(_params.isQuery){
        for(auto&i:usbDrives) zMessage("usbdrive:"+i.toString());
        return OK;
    }

    UsbDriveModel usbdrive;
    if(_params.usbDrivePath.isEmpty()){
        zInfo("no usbDrivePath")
        if(usbDrives.count()>1){
            if(_params.usbSelect){
                usbdrive = SelectUsbDrive(usbDrives);
            } else{
                return TOOMUCH_USBDRIVE;
            }
        } else if(usbDrives.count()==1){
            usbdrive = usbDrives[0];
            zInfo("usbdrive:"+usbdrive.toString());
        } else{
            usbdrive=UsbDriveModel();
        }
    }
    else{
        zInfo("usbDrivePath:"+_params.usbDrivePath)
        usbdrive = SelectUsbDrive2(usbDrives, _params.usbDrivePath);
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

    zMessage(QStringLiteral("sha_tmp: ")+sha_tmp);
    zMessage(QStringLiteral("sha_img: ")+sha_img);
    bool sha_ok = sha_tmp==sha_img;
    zMessage(QStringLiteral("sha_check: ")+(sha_ok?"ok":"failed"));
    if(!sha_ok) return CHECKSUM_ERROR;

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
    auto cmd = QStringLiteral("udevadm info -q path %1").arg(dev);

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

QList<PartitionModel> Work1::GetPartitions(const QString &dev)
{
    auto cmd = QStringLiteral("lsblk -r %1 -o name,path,label,type").arg(dev);
    auto out = ProcessHelper::ShellExecute(cmd);
    if(out.exitCode) return {};
    if(out.stdOut.isEmpty()) return {};

    QList<PartitionModel> e;
    QStringList lines = out.stdOut.split('\n');
    for(auto&l:lines)
    {
        if(l.isEmpty()) continue;

        auto words=l.split(' ');

        bool isPart = words[3]=="part";

        if(isPart){
            PartitionModel m;
            m.partPath = words[1];
            m.label = words[2];
            e.append(m);
        }
    }
    return e;
}

//lsblk -r /dev/sdd -o name,path,label,type,mountpoint
QString Work1::GetMountPoint(const QString& partPath, const QString& partLabel){
    auto cmd = QStringLiteral("lsblk -r %1 -o name,path,label,type,mountpoint").arg(partPath);
    auto out = ProcessHelper::ShellExecute(cmd);
    if(out.exitCode) return {};
    if(out.stdOut.isEmpty()) {};

    QStringList lines = out.stdOut.split('\n');
    for(auto&l:lines){
        if(l.isEmpty()) continue;
        QStringList words = l.split(' ');
        int wordsL = words.count();
        if(wordsL<5) continue;

        bool equals = words[1]==partPath && words[2]==partLabel && words[3]=="part";
        if(equals){
            return words[4];
        }

    }
    return {};
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

    QElapsedTimer t2;

    t2.start();

    auto cmd = QStringLiteral("lsblk -dbro name,path,type,tran,rm,vendor,model,phy-sec,size,mountpoint,serial");
    //auto m2 = ProcessHelper::Model::Parse(cmd);
    auto out = ProcessHelper::ShellExecuteSudo(cmd);

    zInfo("t2 lsblk:"+QString::number(t2.elapsed()));

    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;
    //zInfo("devices:\n");

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;
        UsbDriveModel m = GetUsbDrives2(i);
        if(m.isValid()){
            e.append(m);
        }
    }
    zInfo("t2 GetUsbDrives2:"+QString::number(t2.elapsed()));
    for(auto&m:e){
         Work1::GetUsbDrives3(&m);
    }

    /*
    QList<Work1Thread*> threads;
    for(auto&m:e){
        auto thread = new Work1Thread();
        threads.append(thread);

        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->setM(&m);
        thread->start();
    }


    for(auto&m:threads){
        m->wait(5000);
    }
*/


    // for(auto&m:threads){
    //     m->deleteLater();
    // }

    // QList<Work1Controller*> controllers;
    // //Work1Controller* controller;
    // for(auto&m:e){
    //     //GetUsbDrives3(&m);
    //     auto controller = new Work1Controller(&m);
    //     controllers.append(controller);

    // }

    // for(Work1Controller *controller : controllers){
    //     controller->operate();
    //     break;
    // }

    // while(1){
    //     bool ready=true;
    //     for (Work1Controller *controller : controllers) {
    //         if(!controller->finished()){ ready = false; break;}
    //     }
    //     if(ready) break;
    // }

    // for (Work1Controller *controller : controllers) {
    //     delete controller;
    // }

    zInfo("t2 GetUsbDrives3:"+QString::number(t2.elapsed()));
    return e;
}

UsbDriveModel Work1::GetUsbDrive(const QString &devPath)
{
    UsbDriveModel m;
    m.devicePath = devPath;

    m.usbPath = GetUsbPath(devPath);
    m.partitions = GetPartitions(devPath);
    m.serial = "";

    GetUsbDrives3(&m);
    return m;
}

UsbDriveModel Work1::GetUsbDrives2(const QString& i){
   // QElapsedTimer t;
   // t.start();

    auto j=i.split(' ');

    bool isBoot = j[9]=='/'||j[9]=="/boot";

    if(isBoot) return{};

    bool isRemovableDisk = j[2]==QLatin1String("disk")&&
                 j[4]==QLatin1String("1");

    if(!isRemovableDisk) return{};

    bool isUsb = j[3]==QLatin1String("usb");

    bool isMmc = j[0].startsWith("mmc");

    bool ok;
    qint64 size = j[8].toLongLong(&ok);
    bool hasCard = ok && size>0;

    if(isRemovableDisk && (isUsb || isMmc) && hasCard){
        UsbDriveModel m;
        m.devicePath = j[1];
    //    t.restart();
        m.usbPath = GetUsbPath(j[1]);
   //     zInfo("t GetUsbPath:"+QString::number(t.elapsed()));

    //    t.restart();
        m.partitions = GetPartitions(j[1]);
  //      zInfo("t GetPartitions:"+QString::number(t.elapsed()));

        m.serial = j[10];
        //zInfo("device:"+m.toString());
        //e.append(m);
        return m;
    }

    return{};
}

void Work1::GetUsbDrives3(UsbDriveModel *m){
   // QElapsedTimer t;
//    t.start();

    //for(auto&m:e){
    for(auto&partition:m->partitions){
    //    t.restart();
        auto mountPoint = GetMountPoint(partition.partPath, partition.label);
    //    zInfo("t GetMountPoint:"+QString::number(t.elapsed()));

        if(mountPoint.isEmpty()){
            auto mp2 = MkMountPoint();
         //   t.restart();
            bool mount_ok = MountHelper::Mount(partition.partPath, mp2);
         //   zInfo("t Mount:"+QString::number(t.elapsed())+" "+mp2);

            if(!mount_ok){
        //        t.restart();
                RmMountPoint(mp2);
           //     zInfo("t RmMountPoint err:"+QString::number(t.elapsed()));
                zInfo("cannot mount partition:"+partition.toString());
                continue;
            }
            mountPoint = mp2;
        }

  //      t.restart();
        QString project = GetProject(mountPoint);
   //     zInfo("t GetProject:"+QString::number(t.elapsed()));

        bool umount_ok = MountHelper::UMount(mountPoint);
        if(umount_ok){
     //       t.restart();
            RmMountPoint(mountPoint);
 //           zInfo("t RmMountPoint ok:"+QString::number(t.elapsed()));
        }
        partition.project = project;
    }
   // }
    return ;
}

QString Work1::MkMountPoint(){
    static int counter;

    QString mountPoint = QStringLiteral("/mnt/mountpoint_%1").arg(counter++);

    QString cmd = QStringLiteral("mkdir -p %1").arg(mountPoint);

    ProcessHelper::ShellExecuteSudo(cmd);

    return mountPoint;
}

 void Work1::RmMountPoint(const QString &mountPoint) {
     QString cmd = QStringLiteral("rmdir %1").arg(mountPoint);
     ProcessHelper::ShellExecuteSudo(cmd);
 }

 QString Work1::GetProject(const QString &path)
 {
     QString cmd = QStringLiteral("readlink %1/home/pi/run").arg(path);
     ProcessHelper::Output out = ProcessHelper::ShellExecuteSudo(cmd);

     if(out.stdOut.isEmpty()) return{};
     QStringList words = out.stdOut.split("/");
     QString p;
     if(words.count()>=3){
         p=words[3];
     }
     if(words.count()>=4){
         p+=":"+words.last().trimmed();
     }
     if(p.isEmpty())
         p = out.stdOut;
     return p;
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

UsbDriveModel Work1::SelectUsbDrive2(const QList<UsbDriveModel> &usbdrives, const QString& usbDrivePath)
{
    for(auto&d:usbdrives){
        QString usbp0 = d.GetUsbDevicePath();
        if(usbp0==usbDrivePath) return d;
    }
    return UsbDriveModel();
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
    zInfo("copying:"+src+"->"+dst );
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

QString UsbDriveModel::GetUsbDevicePath() const{
    return usbPath;
    /*QString usbn;
    int ix0 = usbPath.lastIndexOf('/');
    int maxix = usbPath.length()-1;

    if(ix0>=0 && ix0<maxix){
        usbn = usbPath.mid(ix0+1);
    } else{
        usbn =  usbPath;
    }

    return usbn.replace(':','_');*/
}

QString UsbDriveModel::toString() const
{
    QString n =  devicePath;

    /*QString usbn;
    int ix0 = usbPath.lastIndexOf('/');
    int maxix = usbPath.length()-1;

    if(ix0>=0 && ix0<maxix){
        usbn = usbPath.mid(ix0+1);
    } else{
        usbn =  usbPath;
    } */
    QString usbn = GetUsbDevicePath();

    QString labels="";    
    for(auto&p:partitions){
        if(!labels.isEmpty()) labels+="|";
        labels += p.toString();
    }
    QString msg = n+","+usbn+","+serial;//.replace(':','_');
    if(!labels.isEmpty()) msg+="|"+labels;
    return msg;
}

bool UsbDriveModel::isValid()
{
    if(devicePath.isEmpty()) return false;
    if(usbPath.isEmpty()) return false;
    return true;
}

QString PartitionModel::toString() const
{
    QString msg = partPath;
    if(!label.isEmpty()) msg+=","+label;
    if(!project.isEmpty()) msg+=","+project;
    return msg;
}
