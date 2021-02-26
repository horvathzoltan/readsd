#include "work1.h"
#include "common/logger/log.h"
//#include "common/helper/textfilehelper/textfilehelper.h"
#include "common/helper/ProcessHelper/processhelper.h"
//#include "sqlhelper.h"
//#include "settings.h"
//#include "environment.h"
#include <QTextStream>
#include <QVariant>
#include <iostream>

Work1Params Work1::params;

Work1::Work1() = default;



auto Work1::doWork() -> int
{
    // TODO 1. megtudni a kártyát
    // lsblk -dro name,path,type,tran,rm,vendor,model,phy-sec
    // ha több van, lista, választani
    // felírja:
    // sudo fdisk -l /dev/sdh
    // utolsó partíció utolsó szektora +1-ig írunk
    // https://stackoverflow.com/questions/22433257/windows-dd-esque-implementation-in-qt-5
    // mngm ~$ sudo dd if=/dev/sdb of=backup.img bs=512 count=15759360 conv=fsync

    auto usbDrives = GetUsbDrives();
    if(usbDrives.isEmpty()) return ISEMPTY;
    QString usbdrive = (usbDrives.count()>1)?SelectUsbDrive(usbDrives):usbDrives[0];
    //QStringList usbDrives = {"a", "b", "c", "d"};
    //auto usbdrive = SelectUsbDrive(usbDrives);
    int units;
    auto lastrec = GetLastRecord(usbdrive, &units);

    zInfo(usbdrive + ": "+QString::number(lastrec+1))

    return OK;
}

auto Work1::GetLastRecord(const QString& drive, int* units) -> int
{
    auto cmd = QStringLiteral("sudo fdisk -l %1").arg(drive);
    int lastrec = -1;
    auto out = com::helper::ProcessHelper::Execute(cmd);
    if(out.exitCode) return -1;
    if(out.stdOut.isEmpty()) return -1;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.startsWith(QStringLiteral("\\dev")))
        {
            auto j = i.split(' ');
            bool isok;
            auto k = j[3].toInt(&isok);
            if(isok && k>lastrec) lastrec = k;
        }
        else if(units && i.startsWith(QStringLiteral("Units:")))
        {
            auto j = i.split('=');

            auto k = j[1].split(' ');
            bool isok;
            auto u = k[1].toInt(&isok);
            if(isok) *units = u;
        }
    }

    return lastrec;

}

auto Work1::GetUsbDrives() -> QStringList
{
    QStringList e;

    auto cmd = QStringLiteral("lsblk -dro name,path,type,tran,rm,vendor,model,phy-sec");
    auto out = com::helper::ProcessHelper::Execute(cmd);
    if(out.exitCode) return e;
    if(out.stdOut.isEmpty()) return e;

    for(auto&i:out.stdOut.split('\n'))
    {
        if(i.isEmpty()) continue;
        auto j=i.split(' ');
        if(
            j[2]==QLatin1String("disk")&&
            j[3]==QLatin1String("usb")&&
            j[4]==QLatin1String("1")) e.append(j[1]);
        else if(j[0].startsWith("mmc")) e.append(j[1]);
    }

    return e;
}

QString Work1::SelectUsbDrive(const QStringList &usbdrives)
{
    int j = 1;
    for(auto&i:usbdrives) zInfo(QString::number(j++)+": "+i);j--;
    zInfo("select 1-"+QString::number(j))

    QTextStream in(stdin);
    auto intxt = in.readLine();
    bool isok;
    auto ix = intxt.toInt(&isok);
    if(!isok) return QString();
    if(ix<1||ix>j) return QString();
    return usbdrives[ix-1];
}

/*
NAME PATH TYPE TRAN RM VENDOR MODEL PHY-SEC
loop0 /dev/loop0 loop  0   512
loop1 /dev/loop1 loop  0   512
loop2 /dev/loop2 loop  0   512
loop3 /dev/loop3 loop  0   512
loop4 /dev/loop4 loop  0   512
loop5 /dev/loop5 loop  0   512
loop6 /dev/loop6 loop  0   512
loop7 /dev/loop7 loop  0   512
loop8 /dev/loop8 loop  0   512
loop9 /dev/loop9 loop  0   512
loop10 /dev/loop10 loop  0   512
loop11 /dev/loop11 loop  0   512
loop12 /dev/loop12 loop  0   512
loop13 /dev/loop13 loop  0   512
loop14 /dev/loop14 loop  0   512
loop15 /dev/loop15 loop  0   512
loop16 /dev/loop16 loop  0   512
loop17 /dev/loop17 loop  0   512
loop18 /dev/loop18 loop  0   512
loop19 /dev/loop19 loop  0   512
loop20 /dev/loop20 loop  0   512
loop21 /dev/loop21 loop  0   512
loop22 /dev/loop22 loop  0   512
loop23 /dev/loop23 loop  0   512
loop24 /dev/loop24 loop  0   512
loop25 /dev/loop25 loop  0   512
loop26 /dev/loop26 loop  0   512
loop27 /dev/loop27 loop  0   512
loop28 /dev/loop28 loop  0   512
loop29 /dev/loop29 loop  0   512
loop30 /dev/loop30 loop  0   512
loop31 /dev/loop31 loop  0   512
loop32 /dev/loop32 loop  0   512
loop33 /dev/loop33 loop  0   512
loop34 /dev/loop34 loop  0   512
loop35 /dev/loop35 loop  0   512
loop36 /dev/loop36 loop  0   512
loop37 /dev/loop37 loop  0   512
loop38 /dev/loop38 loop  0   512
loop39 /dev/loop39 loop  0   512
loop40 /dev/loop40 loop  0   512
loop41 /dev/loop41 loop  0   512
loop42 /dev/loop42 loop  0   512
loop43 /dev/loop43 loop  0   512
loop44 /dev/loop44 loop  0   512
loop45 /dev/loop45 loop  0   512
loop46 /dev/loop46 loop  0   512
loop47 /dev/loop47 loop  0   512
sda /dev/sda disk sata 0 ATA\x20\x20\x20\x20\x20 KINGSTON_SNVP325S264GB 512
sdb /dev/sdb disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD30EZRX-00DC0B0 4096
sdc /dev/sdc disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD30EZRX-00DC0B0 4096
sdd /dev/sdd disk sata 0 ATA\x20\x20\x20\x20\x20 WDC_WD1600YS-23SHB0 512
sde /dev/sde disk sata 0 ATA\x20\x20\x20\x20\x20 Hitachi_HDS721010CLA330 512
sdh /dev/sdh disk usb 1 Generic\x20 STORAGE_DEVICE 512
sr0 /dev/sr0 rom sata 1 HL-DT-ST HL-DT-STDVD-RAM_GSA-H60N 512
sr1 /dev/sr1 rom sata 1 TSSTcorp TSSTcorp_CDDVDW_SH-222AB 512
sr2 /dev/sr2 rom sata 1 TSSTcorp TSSTcorp_CDDVDW_SH-224DB 51
*/


/*
 * sudo fdisk -l /dev/sdh
[sudo] zoli jelszava:
Disk /dev/sdh: 14,86 GiB, 15931539456 bytes, 31116288 sectors
Disk model: STORAGE DEVICE
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x5e3da3da

Eszköz     Indítható  Start     Vége Szektorok  Size Id Típus
    /dev/sdh1              8192   532479    524288  256M  c W95 FAT32 (LBA)
    /dev/sdh2            532480 18702335  18169856  8,7G 83 Linux

*/
