#include "globals.h"

const std::string ds::TEMP_FILE = ".temp.txt";
const std::string ds::VERSION_STR = "0.0.4";

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
