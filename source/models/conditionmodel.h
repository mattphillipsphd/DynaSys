#ifndef CONDITIONMODEL_H
#define CONDITIONMODEL_H

#include <QStandardItem>
#include <QStandardItemModel>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "parammodelbase.h"

//Doing it this way, the key is the condition and the value/expression is the (comma-delimited) string
//of results
class ConditionModel : public ParamModelBase
{
    Q_OBJECT
    public:
        enum
        {
            TEST = NUM_BASE_COLUMNS,
            NUM_COND_COLUMNS
        };

        explicit ConditionModel(QObject *parent, const std::string& name);
        virtual ~ConditionModel() override;

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const { return false; }
        virtual void ProcessParamFileLine(const std::string& key, std::string rem) override;

        void AddCondition(const std::string& condition, const VecStr& exprns = VecStr());
        void AddResult(int row, const std::string& result);

        void SetResults(int row, const VecStr& exprns);

        std::string Condition(int row) const;
        VecStr Results(int row) const;
        virtual std::string String() const override;

        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    private:
        static const std::string DELIM;

        mutable std::mutex _cmutex;
};

#endif // CONDITIONMODEL_H
