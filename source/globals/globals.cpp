#include "globals.h"

const double ds::PI = 3.14159265358979;

const std::string ds::TEMP_FILE = ".temp.txt";
const std::string ds::VERSION_STR = "0.1.1";

ds::PMODEL ds::Model(const std::string& model)
{
    if (model=="Parameters") return PARAMETERS;
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
        case PARAMETERS:
            return "Parameters";
        case VARIABLES:
            return "Variables";
        case DIFFERENTIALS:
            return "Differentials";
        case INIT_CONDS:
            return "InitialConds";
        case CONDITIONS:
            return "Conditions";
    }
    throw("Bad Model");
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
