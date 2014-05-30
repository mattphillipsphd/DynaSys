#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include "parammodelbase.h"
#include "../memrep/input.h"

class VariableModel : public ParamModelBase
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

    signals:

    public slots:

};

#endif // VARIABLEMODEL_H
