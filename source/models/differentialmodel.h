#ifndef DIFFERENTIALMODEL_H
#define DIFFERENTIALMODEL_H

#include "numericmodelbase.h"

class DifferentialModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit DifferentialModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
        virtual std::string ShortKey(size_t idx) const override;
        virtual int ShortKeyIndex(const std::string& par_name) const override;
        virtual std::string TempExpression(size_t idx) const override;
};

#endif // DIFFERENTIALMODEL_H
