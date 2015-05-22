#ifndef JACOBIANMODEL_H
#define JACOBIANMODEL_H

#include "numericmodelbase.h"
// ### Probably should inherit directly from parammodelbase

class JacobianModel : public NumericModelBase
{
    Q_OBJECT

    public:
        JacobianModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
};

#endif // JACOBIANMODEL_H
