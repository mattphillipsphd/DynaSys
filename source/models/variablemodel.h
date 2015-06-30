#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include "numericmodelbase.h"
#include "../memrep/input.h"

class VariableModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit VariableModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return true; }
        virtual VecStr Expressions() const override;
        virtual VecStr Initializations() const override;
        virtual std::string TempExpression(size_t i) const override;
        virtual VecStr TempExpressions() const override;
        int TypeCount(Input::TYPE type) const;

    signals:

    public slots:

};

#endif // VARIABLEMODEL_H
