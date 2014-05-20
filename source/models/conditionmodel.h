#ifndef CONDITIONMODEL_H
#define CONDITIONMODEL_H

#include <QStandardItem>
#include <QStandardItemModel>

#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

typedef std::vector<std::string> VecStr;

//Really this is pretty general to any 2-column model
// Knuckled under and did this with QStandardItemModel, could not figure out how inheriting directly
// from QAbstractItemModel
class ConditionModel : public QStandardItemModel
{
    Q_OBJECT
    public:
        explicit ConditionModel(QObject *parent = 0);

        void Read(std::istream& in);
        void Write(std::ostream& out) const;

        void AddCondition(const std::string& condition, const VecStr& exprns = VecStr());
        void AddExpression(int row, const std::string& exprn);

        void SetExpressions(int row, const VecStr& exprns);

        const std::string Condition(int row) const;
        std::string EdString() const;
        const VecStr Expressions(int row) const;
        size_t NumPars() const { return rowCount(); }
        std::string String() const;

    signals:

    public slots:

    private:
};

#endif // CONDITIONMODEL_H
