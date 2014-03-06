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

    signals:

    public slots:

};

#endif // VARIABLEMODEL_H
