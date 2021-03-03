#include <QCoreApplication>
#include "common/logger/log.h"
#include "common/helper/signalhelper/signalhelper.h"
#include "common/coreappworker/coreappworker.h"
#include "common/helper/CommandLineParserHelper/commandlineparserhelper.h"
//#include "settings.h"
//#include "environment.h"
#include "work1.h"
//#include "buildnumber.h"
#include "typekey.h"

struct ParserKeyValueDesc{
    QString key;
    QString value;
    QString desc;
};

void ParserInit(QCommandLineParser *p, QCoreApplication *a, const QString& desc, const QList<ParserKeyValueDesc>& opts)
{
    p->setApplicationDescription(desc);
    p->addHelpOption();
    p->addVersionOption();

    //const QString OPTION_OUT = QStringLiteral("output");
    for(auto&i:opts) com::helper::CommandLineParserHelper::addOption(p, i.key, i.desc);

    p->process(*a);
}

auto main(int argc, char *argv[]) -> int
{
    com::helper::SignalHelper::setShutDownSignal(com::helper::SignalHelper::SIGINT_); // shut down on ctrl-c
    com::helper::SignalHelper::setShutDownSignal(com::helper::SignalHelper::SIGTERM_); // shut down on killall

    //    zInfo(QStringLiteral("started: %1").arg(Buildnumber::buildnum));
    //zInfo(QStringLiteral("started: %1").arg(BUILDNUMBER));

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("readsd"));

    QCommandLineParser parser;

    ParserInit(&parser, &a, QStringLiteral("reads a sd card"),
               {
                   {
                       zkey(Work1Params::ofile),
                       QStringLiteral("output"),
                       QStringLiteral("file as output")
                   }
               });

//    parser.setApplicationDescription(QStringLiteral("reads a sd card"));
//    parser.addHelpOption();
//    parser.addVersionOption();

//    const QString OPTION_OUT = QStringLiteral("output");

    //com::helper::CommandLineParserHelper::addOption(&parser, OPTION_OUT, QStringLiteral("file as output"));

    //parser.process(a);

    //Work1::params.ofile = parser.value(OPTION_OUT);

    com::CoreAppWorker c(Work1::doWork, &a, &parser);
    volatile auto errcode = c.run();

    switch(errcode)
    {
        case Work1::OK: zInfo("ok"); break;
        case Work1::ISEMPTY: zInfo("no block device to read"); break;
        case Work1::NOOUTFILE: zInfo("no output file to write"); break;
        case Work1::NOLASTREC: zInfo("cannot find last record"); break;
        case Work1::NOUNITS: zInfo("unknown block size"); break;

        case Work1::CANNOTUNMOUNT: zInfo("cannot unmount device"); break;
    }

    auto e = QCoreApplication::exec();
    return e;
}

