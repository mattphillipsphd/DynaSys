#include "conditionmodel.h"

ConditionModel::ConditionModel(QObject *parent) :
    QStandardItemModel(parent)
{
}

void ConditionModel::AddCondition(const std::string& condition, const VecStr& exprns)
{
    QStandardItem* itm = new QStandardItem(condition.c_str());
    for (const auto& it : exprns)
    {
        QStandardItem* exprn = new QStandardItem(it.c_str());
        itm->appendRow(exprn);
    }
    appendRow(itm);
}
void ConditionModel::AddExpression(int row, const std::string& exprn)
{
    QStandardItem* parent = item(row),
            * exprn_itm = new QStandardItem(exprn.c_str());
    parent->appendRow(exprn_itm);
}

const std::string ConditionModel::Condition(int row) const
{
    QStandardItem* cond = item(row);
    return cond->text().toStdString();
}
const VecStr ConditionModel::Expressions(int row) const
{
    VecStr vstr;
    QStandardItem* parent = item(row);
    const int num_exprns = rowCount(parent->index());
    for (int j=0; j<num_exprns; ++j)
    {
        QStandardItem* exprn = parent->child(j);
        vstr.push_back(exprn->text().toStdString());
    }
    return vstr;
}
void ConditionModel::Read(std::ifstream& in)
{
    clear();
    std::string line;
    while (line.empty() && !in.eof())
        std::getline(in, line);
    if (in.eof()) return;
    int tab = line.find_first_of('\t');
    int num_conds = std::stoi( line.substr(tab+1) );
    for (int i=0; i<num_conds; ++i)
    {
        std::getline(in, line);
        tab = line.find_first_of('\t');
        std::string cond = line.substr(0,tab);
        int num_results = std::stoi( line.substr(tab+1) );
        VecStr results(num_results);
        for (int j=0; j<num_results; ++j)
        {
            std::getline(in, line);
            results[j] = line.substr(1); //The first character is a tab
        }
        AddCondition(cond, results);
    }
}
void ConditionModel::Write(std::ofstream& out) const
{
    const int row_count = rowCount();
    out << "Conditions\t" << row_count << std::endl;
    for (int i=0; i<row_count; ++i)
    {
        QStandardItem* cond = item(i);
        const int num_exprns = rowCount(cond->index());
        out << cond->text().toStdString() << "\t" << num_exprns << std::endl;
        for (int j=0; j<num_exprns; ++j)
            out << "\t" << cond->child(j)->text().toStdString() << std::endl;
    }
    out << std::endl;
}


