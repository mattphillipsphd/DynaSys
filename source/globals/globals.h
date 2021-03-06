#ifndef GLOBALS_H
#define GLOBALS_H

#include <algorithm>
#include <assert.h>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QStringList>
#include <QStringListModel>
#include <QTimer>

#ifdef QT_DEBUG
#define DEBUG_FUNC
#endif

typedef std::pair<std::string, std::string> PairStr;
typedef std::map<std::string, std::string> MapStr;
typedef std::vector<std::string> VecStr;
namespace ds
{
    extern const double DEFAULT_MODEL_STEP;
    extern const int MAX_NUM_PARS;
    extern const double PI;
    extern const int TABLEN;

    extern const std::string TEMP_FILE;
    extern const std::string TEMP_DAT_FILE;
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
        INP = 0,
        VAR,
        DIFF,
        INIT,
        COND,
        NC,
        JAC,
        NUM_MODELS //Nice trick from SO
    };

    enum EQ_CAT
    {
        UNKNOWN = -1,
        STABLE_NODE,
        STABLE_FOCUS,
        SADDLE,
        UNSTABLE_NODE,
        UNSTABLE_FOCUS
    };
    struct Equilibrium
    {
        Equilibrium(double x, double y, EQ_CAT ec = UNKNOWN)
            : x(x), y(y), eq_cat(ec)
        {}
        Equilibrium() : eq_cat(UNKNOWN)
        {}
        Equilibrium(const Equilibrium& other) : x(other.x), y(other.y), eq_cat(other.eq_cat)
        {}
        double x, y;
        EQ_CAT eq_cat;
    };
    std::string EqCatStr(EQ_CAT eq_cat);

    extern int thread_ct;
    extern std::map<std::thread::id, QColor> thread_map;

    extern const std::string UNC_INFIX;

    void AddThread(std::thread::id tid);

    std::string Join(const VecStr& vec, const std::string& delim);

    PMODEL Model(const std::string& model);
    std::string Model(PMODEL mi);

    void RemoveThread(std::thread::id tid);
    void RemoveWhitespace(std::string& s);
    VecStr RemoveWhitespace(const VecStr& vec);

    double sgn(double val);

    VecStr Split(const std::string& str, const std::string& delim);
    std::string StripPath(const std::string& file_name);
    std::string StripQuotes(const std::string& str);

    const std::vector<QColor> TraceColors();
    QColor ThreadColor(std::thread::id tid);

    template<typename T>
    QList<T> VecToQList(const std::vector<T>& vec);

    QStringList VecStrToQSList(const VecStr& vec);

    int VersionNum(const std::string& ver_str = VERSION_STR);
        //This assumes that the actual version is at the end of the string
}

#endif // GLOBALS_H
