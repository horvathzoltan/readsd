#include <QCoreApplication>
#include "helpers/logger.h"
#include "helpers/signalhelper.h"
#include "helpers/coreappworker.h"
#include "helpers/userhelper.h"
#include "work1.h"
#include "typekey.h"
#include "helpers/stringify.h"

// ./readsd -p /media/pi/butyok2/clone/ -o img56 -s Aladar123 -f
// ./readsd -p /home/zoli/aaa/ -o img56 -s Aladar123 -f

/*
started readsd as zoli
[sudo] zoli jelszava:
usbdrive:/dev/sdc:1-1.4.1_1.0|/dev/sdc1:bootfs|/dev/sdc2:rootfs:infopanel2:infopanel2_1704
usbdrive:/dev/sdd:1-1.4.2_1.0|/dev/sdd1:teszt2
ok

Process exited with code: 0
*/
auto main(int argc, char *argv[]) -> int
{
#if defined (STRING) && defined (TARGI)
    auto target = STRING(TARGI);
#else
    auto target=QStringLiteral("ApplicationNameString");
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(target);
    QCoreApplication::setApplicationVersion("1");
    QCoreApplication::setOrganizationName("LogControl Kft.");
    QCoreApplication::setOrganizationDomain("https://www.logcontrol.hu/");

    SignalHelper::setShutDownSignal(SignalHelper::SIGINT_); // shut down on ctrl-c
    SignalHelper::setShutDownSignal(SignalHelper::SIGTERM_); // shut down on killall
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, true, true);

    QString user = UserHelper::GetUser();
    QString startmsg = QStringLiteral("started ")+target+" as "+(!user.isEmpty()?user:"anonymous");
    zInfo(startmsg);

    QCommandLineParser parser;
    QList<QCommandLineOption> options{
        {{"o",QStringLiteral("output")},QStringLiteral("image file name"),"output"},
        {{"p",QStringLiteral("path")},QStringLiteral("working path"),"path"},
        {{"s",QStringLiteral("passwd")},QStringLiteral("secret"),"passwd"},
        {{"f",QStringLiteral("force")},QStringLiteral("no ask")},
        {{"b",QStringLiteral("usbselect")},QStringLiteral("enable usb drive select if more available")},
        {{"q",QStringLiteral("query")},QStringLiteral("query only")},
        };
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions(options);
    parser.process(app);

    Work1::_params.Parse(&parser);

    CoreAppWorker c(Work1::doWork, &app);
    volatile auto errcode = c.run();

    switch(errcode)
    {
        case Work1::OK: zInfo("ok"); break;
        case Work1::ISEMPTY: zInfo("no block device to read"); break;
        case Work1::NO_OUTFILE: zInfo("no output file to write"); break;
        case Work1::NO_LASTREC: zInfo("cannot find last record"); break;
        case Work1::NO_UNITS: zInfo("unknown block size"); break;

        case Work1::CANNOT_UNMOUNT: zInfo("cannot unmount device"); break;
        case Work1::NO_PASSWD: zInfo("cannot sudo"); break;

        case Work1::NO_CHECK0: zInfo("cannot read drive sha"); break;
        case Work1::NO_CHECK1: zInfo("cannot read image sha"); break;
        case Work1::CHECKSUM_ERROR: zInfo("file check failed"); break;
        case Work1::NO_USBDRIVE: zInfo("no usb device to use"); break;
            case Work1::TOOMUCH_USBDRIVE: zInfo("too much usb device"); break;
    }

    auto e = QCoreApplication::exec();
    return e;
}

