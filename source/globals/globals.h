#ifndef GLOBALS_H
#define GLOBALS_H

#include <map>
#include <string>
#include <thread>
#include <vector>

#include <QColor>
#include <QList>
#include <QStringList>

#ifdef QT_DEBUG
#define DEBUG_FUNC
#endif

typedef std::vector<std::string> VecStr;
namespace ds
{
    extern const double DEFAULT_MODEL_STEP;
    extern const double PI;
    extern const int TABLEN;

    extern const std::string TEMP_FILE;
    extern const std::string TEMP_MODEL_FILE;
    extern const std::string VERSION_STR;

#ifndef Q_OS_WIN
    extern const std::vector<QColor> THREAD_COLORS;
#else
    extern std::vector<QColor> THREAD_COLORS;
    void InitThreadColors();
#endif

    enum PMODEL
    {
        INPUTS = 0,
        VARIABLES,
        DIFFERENTIALS,
        INIT_CONDS,
        CONDITIONS,
        NUM_MODELS //Nice trick from SO
    };

    extern int thread_ct;
    extern std::map<std::thread::id, QColor> thread_map;

    void AddThread(std::thread::id tid);

    PMODEL Model(const std::string& model);
    std::string Model(PMODEL model);

    void RemoveThread(std::thread::id tid);

    double sgn(double val);

    QColor ThreadColor(std::thread::id tid);

    template<typename T>
    QList<T> VecToQList(const std::vector<T>& vec);

    QStringList VecStrToQSList(const VecStr& vec);

    int VersionNum(const std::string& ver_str = VERSION_STR);
        //This assumes that the actual version is at the end of the string
}

#endif // GLOBALS_H
