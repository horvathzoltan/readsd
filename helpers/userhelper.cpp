#include "processhelper.h"
#include "userhelper.h"
#include "helpers/stringify.h"

QString UserHelper::GetUser(){

    QString name;
#if defined(Q_OS_WIN)
    name = getenv("USERNAME");
#elif defined(Q_OS_UNIX)
    name = getenv("USER");
    if(name.isEmpty()){
        QString cmd = L("whoami");
        ProcessHelper::Output out = ProcessHelper::ShellExecute(cmd);
        name = out.stdOut.trimmed();
    }
#endif

        return name;
}
