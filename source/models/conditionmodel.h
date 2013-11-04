#ifndef CONDITIONMODEL_H
#define CONDITIONMODEL_H

#include <QStandardItem>
#include <QStandardItemModel>

#include <fstream>
#include <map>
#include <vector>
#include <string>

typedef std::vector<std::string> VecStr;

//Really this is pretty general to any 2-column model
// Knuckled under and did this with QStandardItemModel, could not figure out how inheriting directly
// from QAbstractItemModel
class ConditionModel : public QStandardItemModel
{
    Q_OBJECT
    public:
        explicit ConditionModel(QObject *parent = 0);

        void Read(std::ifstream& in);
        void Write(std::ofstream& out) const;

        void AddCondition(const std::string& condition, const VecStr& exprns = VecStr());
        void AddExpression(int row, const std::string& exprn);

        const std::string Condition(int row) const;
        const VecStr Expressions(int row) const;

    signals:

    public slots:

    private:
};

#endif // CONDITIONMODEL_H
