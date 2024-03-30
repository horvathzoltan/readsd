#include "mounthelper.h"
#include "processhelper.h"
/*
bool MountHelper::isMounted(MountPathTypes mtype, QString* s){
    auto p = _mountPaths.value(mtype);
    //if(p.isEmpty()) return false;

    //TODO előtte ping - ha a host nem megy nem elérhető
    QString cmd = QStringLiteral(R"(df --output=target %1)").arg(p.localPath);
    auto out = ProcessHelper::Execute(cmd);

    if(out.exitCode!=0) return false;
    if(out.stdOut.isEmpty()) return false;
    auto a = out.stdOut.split('\n');
    if(a.size()<2) return false;
    if(a[1].isEmpty()) return false;

    if(s) *s=a[1];
    return a[1]!="/";
}
*/
bool MountHelper::Mount(const QString& devpath, const QString& mountpath){
    QString cmd = QStringLiteral(R"(mount -t ext4 %1 %3)").arg(devpath,mountpath);
    auto out = ProcessHelper::ShellExecuteSudo(cmd);

    if(out.exitCode) return false;
    if(!out.stdErr.isEmpty()) return false;

    return true;
}

bool MountHelper::UMount(const QString& mountpath){
    QString cmd = QStringLiteral(R"(umount %1)").arg(mountpath);
    auto out = ProcessHelper::ShellExecuteSudo(cmd);

    if(out.exitCode) return false;
    if(!out.stdErr.isEmpty()) return false;

    return true;
}
