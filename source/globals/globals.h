#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>

#include <QList>
#include <QStringList>

#ifdef QT_DEBUG
#define DEBUG_FUNC
#endif

typedef std::vector<std::string> VecStr;
namespace ds
{
    extern const double PI;

    extern const std::string TEMP_FILE;
    extern const std::string VERSION_STR;

    enum PMODEL
    {
        PARAMETERS = 0,
        VARIABLES,
        DIFFERENTIALS,
        INIT_CONDS,
        CONDITIONS
    };

    PMODEL Model(const std::string& model);
    std::string Model(PMODEL model);

    double sgn(double val);

    template<typename T>
    QList<T> VecToQList(const std::vector<T>& vec);

    QStringList VecStrToQSList(const VecStr& vec);

    int VersionNum(const std::string& ver_str = VERSION_STR);
        //This assumes that the actual version is at the end of the string
}

#endif // GLOBALS_H
