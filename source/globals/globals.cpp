#include "globals.h"

const double ds::DEFAULT_MODEL_STEP = 0.001;
const int ds::MAX_NUM_PARS = 1024;
const double ds::PI = 3.14159265358979;
const int ds::TABLEN = 4;

const std::string ds::TEMP_FILE = ".temp.txt";
const std::string ds::TEMP_DAT_FILE = ".temp.dsdat";
const std::string ds::TEMP_MODEL_FILE = ".temp_model.txt";
const std::string ds::VERSION_STR = "0.4.0";

#ifndef Q_OS_WIN
const std::vector<QColor> ds::THREAD_COLORS = {Qt::black, Qt::red, Qt::green, Qt::gray, Qt::blue };
#else
std::vector<QColor> ds::THREAD_COLORS;
void ds::InitThreadColors()
{
    THREAD_COLORS.push_back(Qt::black);
    THREAD_COLORS.push_back(Qt::red);
    THREAD_COLORS.push_back(Qt::green);
    THREAD_COLORS.push_back(Qt::gray);
    THREAD_COLORS.push_back(Qt::blue);
}
#endif

int ds::thread_ct = 0;
std::map<std::thread::id, QColor> ds::thread_map;

const std::string ds::UNC_INFIX = "_for_d";

void ds::AddThread(std::thread::id tid)
{
    thread_map[tid] = THREAD_COLORS.at( thread_ct % THREAD_COLORS.size() );
    ++thread_ct;
}

std::string ds::Join(const VecStr& vec, const std::string& delim)
{
    if (vec.empty()) return "";
    std::string out = vec.at(0);
    const size_t num_elts = vec.size();
    for (size_t i=1; i<num_elts; ++i)
        out += delim + vec.at(i);
    return out;
}

ds::PMODEL ds::Model(const std::string& model)
{
    if (model=="Inputs") return INP;
    if (model=="Variables") return VAR;
    if (model=="Differentials") return DIFF;
    if (model=="InitialConds") return INIT;
    if (model=="Conditions") return COND;
    if (model=="Nullclines") return NC;
    if (model=="Jacobian") return JAC;
    throw std::runtime_error("ds::Model: Bad Model");
}
std::string ds::Model(PMODEL mi)
{
    switch (mi)
    {
        case INP:
            return "Inputs";
        case VAR:
            return "Variables";
        case DIFF:
            return "Differentials";
        case INIT:
            return "InitialConds";
        case COND:
            return "Conditions";
        case NC:
            return "Nullclines";
        case JAC:
            return "Jacobian";
        case NUM_MODELS:
            throw std::runtime_error("ds::Model: Not a model");
    }
    throw std::runtime_error("ds::Model: Bad Model");
}

void ds::RemoveThread(std::thread::id tid)
{
    thread_map.equal_range(tid);
    --thread_ct;
}
void ds::RemoveWhitespace(std::string& s)
{
    s.erase( std::remove_if(s.begin(), s.end(), ::isspace), s.end() );
}
VecStr ds::RemoveWhitespace(const VecStr& vec)
{
    VecStr out;
    for (const auto& it : vec)
    {
        std::string s = it;
        RemoveWhitespace(s);
        out.push_back(s);
    }
    return out;
}

double ds::sgn(double val)
{
    return (val<0) ? -1.0 : (val>0.0); //Thanks SO
        //http://stackoverflow.com/questions/1903954
}

VecStr ds::Split(const std::string& str, const std::string& delim)
{
    VecStr out;
    std::string haystack = str;
    size_t pos = haystack.find(delim.c_str());
    const size_t dlen = delim.length();
    while (pos != std::string::npos)
    {
        out.push_back(haystack.substr(0,pos));
        haystack.erase(0, pos+dlen);
        pos = haystack.find(delim.c_str());
    }
    if (!haystack.empty()) out.push_back(haystack);

    return out;
}
std::string ds::StripPath(const std::string& file_name)
{
    const size_t pos = file_name.find_last_of('/');
    return (pos==std::string::npos) ? file_name : file_name.substr(pos+1);
}
std::string ds::StripQuotes(const std::string& str)
{
    std::string out = str;
    size_t pos = out.find_first_of('"');
    if (pos!=std::string::npos)
    {
        out.erase(0,pos+1);
        pos = out.find_last_of('"');
        if (pos!=std::string::npos)
            out.erase(pos);
    }
    return out;
}

const std::vector<QColor> ds::TraceColors()
{
    std::vector<QColor> vc;
    vc.push_back(Qt::black);
    vc.push_back(Qt::blue);
    vc.push_back(Qt::red);
    vc.push_back(Qt::green);
    vc.push_back(Qt::gray);
    vc.push_back(Qt::cyan);
    vc.push_back(Qt::magenta);
    vc.push_back(Qt::yellow);
    vc.push_back(Qt::darkBlue);
    vc.push_back(Qt::darkRed);
    vc.push_back(Qt::darkGreen);
    vc.push_back(Qt::darkGray);
    vc.push_back(Qt::darkCyan);
    vc.push_back(Qt::darkMagenta);
    vc.push_back(Qt::darkYellow);
    return vc;
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
