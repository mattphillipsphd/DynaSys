#include "globals.h"

const double ds::DEFAULT_MODEL_STEP = 0.001;
const double ds::PI = 3.14159265358979;
const int ds::TABLEN = 4;

const std::string ds::TEMP_FILE = ".temp.txt";
const std::string ds::TEMP_MODEL_FILE = ".temp_model.txt";
const std::string ds::VERSION_STR = "0.1.4";

const std::vector<QColor> ds::THREAD_COLORS = {Qt::black, Qt::red, Qt::green, Qt::gray, Qt::blue };

int ds::thread_ct = 0;
std::map<std::thread::id, QColor> ds::thread_map;

void ds::AddThread(std::thread::id tid)
{
    thread_map[tid] = THREAD_COLORS.at( thread_ct % THREAD_COLORS.size() );
    ++thread_ct;
}

ds::PMODEL ds::Model(const std::string& model)
{
    if (model=="Inputs") return INPUTS;
    if (model=="Variables") return VARIABLES;
    if (model=="Differentials") return DIFFERENTIALS;
    if (model=="InitialConds") return INIT_CONDS;
    if (model=="Conditions") return CONDITIONS;
    throw ("Bad Model");
}
std::string ds::Model(PMODEL model)
{
    switch (model)
    {
        case INPUTS:
            return "Inputs";
        case VARIABLES:
            return "Variables";
        case DIFFERENTIALS:
            return "Differentials";
        case INIT_CONDS:
            return "InitialConds";
        case CONDITIONS:
            return "Conditions";
        case NUM_MODELS:
            throw("Not a model");
    }
    throw("Bad Model");
}

void ds::RemoveThread(std::thread::id tid)
{
    thread_map.equal_range(tid);
    --thread_ct;
}

double ds::sgn(double val)
{
    return (val<0) ? -1.0 : (val>0.0); //Thanks SO
        //http://stackoverflow.com/questions/1903954
}

QColor ds::ThreadColor(std::thread::id tid)
{
    return thread_map[tid];
}

template <typename T>
QList<T> ds::VecToQList(const std::vector<T>& vec)
{
    QList<T> ql;
    for (const auto& it : vec)
        ql.push_back(it);
    return ql;
}

QStringList ds::VecStrToQSList(const VecStr& vec)
{
    QStringList ql;
    for (const auto& it : vec)
        ql.push_back(it.c_str());
    return ql;
}

int ds::VersionNum(const std::string& ver_str)
{
    int major, minor, build;
    size_t dot = ver_str.find_last_of('.');
    build = std::stoi(ver_str.substr(dot+1));
    std::string rest = ver_str.substr(0,dot);
    dot = rest.find_last_of('.');
    minor = std::stoi(rest.substr(dot+1));
    size_t spc = rest.find_last_of(" \t");
    major = std::stoi(rest.substr(spc+1, dot));
    return 10000*major + 100*minor + build;
}
