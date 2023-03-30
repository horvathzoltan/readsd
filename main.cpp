#include <QCoreApplication>
#include "helpers/logger.h"
#include "helpers/signalhelper.h"
#include "helpers/commandlineparserhelper.h"
#include "helpers/coreappworker.h"

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
    for(auto&i:opts) CommandLineParserHelper::addOption(p, i.key, i.desc);

    p->process(*a);
}

auto main(int argc, char *argv[]) -> int
{
    SignalHelper::setShutDownSignal(SignalHelper::SIGINT_); // shut down on ctrl-c
    SignalHelper::setShutDownSignal(SignalHelper::SIGTERM_); // shut down on killall
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, true, true);

    //zInfo(QStringLiteral("started: %1").arg(BUILDNUMBER));
    zInfo(QStringLiteral("started: %1").arg("readsd"));

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
    const QString OPTION_PATH = QStringLiteral("path");

//    const QString OPTION_OUT = QStringLiteral("output");
    CommandLineParserHelper::addOption(&parser, OPTION_PATH, QStringLiteral("output folder"));

    //com::helper::CommandLineParserHelper::addOption(&parser, OPTION_OUT, QStringLiteral("file as output"));

    //parser.process(a);

    //Work1::params.ofile = parser.value(OPTION_OUT);
    Work1::params.workingpath = parser.value(OPTION_PATH);

    CoreAppWorker c(Work1::doWork, &a, &parser);
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

