#ifndef NULLCLINEMODEL_H
#define NULLCLINEMODEL_H

#include "numericmodelbase.h"

class NullclineModel : public NumericModelBase
{
    Q_OBJECT

    public:
        NullclineModel(QObject *parent, const std::string& name);
        virtual ~NullclineModel() override;

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
        virtual std::string TempExpression(size_t i) const override;

    private:
};

#endif // NULLCLINEMODEL_H
