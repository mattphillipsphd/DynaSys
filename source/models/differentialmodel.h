#ifndef DIFFERENTIALMODEL_H
#define DIFFERENTIALMODEL_H

#include "parammodelbase.h"

class DifferentialModel : public ParamModelBase
{
    Q_OBJECT

    public:
        explicit DifferentialModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
        virtual std::string Expression(size_t idx) const override;
        virtual VecStr Expressions() const override;
        virtual std::string ShortKey(size_t idx) const override;
};

#endif // DIFFERENTIALMODEL_H
