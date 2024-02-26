#ifndef MOUNTHELPER_H
#define MOUNTHELPER_H

#include <QString>


class MountHelper
{
public:
    //static QString isMounted(const QString& partitionPath);
    static bool Mount(const QString& devpath, const QString& mountpath);
    static bool UMount(const QString& mountpath);
};


#endif // MOUNTHELPER_H
