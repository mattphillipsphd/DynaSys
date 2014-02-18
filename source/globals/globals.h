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
}

#endif // GLOBALS_H
