#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>

#include <QList>
#include <QStringList>

typedef std::vector<std::string> VecStr;
namespace ds
{
    extern const std::string TEMP_FILE;
    extern const std::string VERSION_STR;

    template<typename T>
    QList<T> VecToQList(const std::vector<T>& vec);

    QStringList VecStrToQSList(const VecStr& vec);

    int VersionNum(const std::string& ver_str = VERSION_STR);
        //This assumes that the actual version is at the end of the string
}

#endif // GLOBALS_H
