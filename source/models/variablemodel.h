#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include "parammodel.h"
#include "../memrep/input.h"

class VariableModel : public ParamModel
{
    Q_OBJECT

    public:
        explicit VariableModel(QObject *parent, const std::string& name);
        virtual VecStr Expressions() const override;

    signals:

    public slots:

};

#endif // VARIABLEMODEL_H
