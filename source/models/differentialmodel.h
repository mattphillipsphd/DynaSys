#ifndef DIFFERENTIALMODEL_H
#define DIFFERENTIALMODEL_H

#include "parammodel.h"

class DifferentialModel : public ParamModel
{
    Q_OBJECT

    public:
        explicit DifferentialModel(QObject *parent, const std::string& name);
        virtual VecStr Expressions() const override;
};

#endif // DIFFERENTIALMODEL_H
