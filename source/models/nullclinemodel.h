#ifndef NULLCLINEMODEL_H
#define NULLCLINEMODEL_H

#include "numericmodelbase.h"

class NullclineModel : public NumericModelBase
{
    Q_OBJECT

    public:
        NullclineModel(QObject *parent, const std::string& name);

        virtual void AddParameter(const std::string& param, const std::string& value = "");
        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
        virtual std::string TempExpression(size_t i) const override;

    private:
        const static std::string PREFIX;
};

#endif // NULLCLINEMODEL_H
