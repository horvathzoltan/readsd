#include <QCoreApplication>
#include "helpers/logger.h"
#include "helpers/signalhelper.h"
#include "helpers/commandlineparserhelper.h"
#include "helpers/coreappworker.h"

#include "work1.h"
#include "typekey.h"
#include "helpers/stringify.h"

struct ParserKeyValueDesc{
    QString key;
    QString value;
    QString desc;
    bool isBool = false;
};

void ParserInit(QCommandLineParser *p, QCoreApplication *a, const QString& desc, const QList<ParserKeyValueDesc>& opts)
{
    p->setApplicationDescription(desc);
    p->addHelpOption();
    p->addVersionOption();

    //const QString OPTION_OUT = QStringLiteral("output");
    for(auto&i:opts){
        if(i.isBool){
            CommandLineParserHelper::addOptionBool(p, i.key, i.desc);
        } else{
            CommandLineParserHelper::addOption(p, i.key, i.desc);
        }
    }

    p->process(*a);
}

auto main(int argc, char *argv[]) -> int
{
#if defined (STRING) && defined (TARGI)
    auto target = STRING(TARGI);
#else
    auto target=QStringLiteral("ApplicationNameString");
#endif

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName(target);
    QCoreApplication::setApplicationVersion("1");
    QCoreApplication::setOrganizationName("LogControl Kft.");
    QCoreApplication::setOrganizationDomain("https://www.logcontrol.hu/");

    SignalHelper::setShutDownSignal(SignalHelper::SIGINT_); // shut down on ctrl-c
    SignalHelper::setShutDownSignal(SignalHelper::SIGTERM_); // shut down on killall
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, true, true);

    QString user = qgetenv("USER");
    zInfo(QStringLiteral("started ")+target+" as "+user);

    QCommandLineParser parser;

    ParserInit(&parser, &a, QStringLiteral("reads a sd card"),
               {
                {"o",QStringLiteral("output"),QStringLiteral("file as output")},
                {"p",QStringLiteral("path"),QStringLiteral("working path")},
                {"s",QStringLiteral("passwd"),QStringLiteral("secret")},
                {"f",QStringLiteral("force"),QStringLiteral("no ask"), true},
               });    

    Work1::params.Parse(&parser);

    CoreAppWorker c(Work1::doWork, &a, &parser);
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
    }

    auto e = QCoreApplication::exec();
    return e;
}

