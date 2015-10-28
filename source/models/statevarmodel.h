#ifndef STATEVARMODEL_H
#define STATEVARMODEL_H

#include "numericmodelbase.h"

class StateVarModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit StateVarModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return true; }
        virtual bool DoInitialize() const override { return false; }
        virtual std::string ShortKey(size_t idx) const override;
        virtual int ShortKeyIndex(const std::string& par_name) const override;
        virtual std::string TempExpression(size_t idx) const override;
        virtual std::string TempExprnForCFile(size_t idx) const override;

    private:
        std::string ExprnInsert(const std::string& in, const std::string& exprn,
                                const std::string& token) const;
        std::string TempExpression(size_t idx, const std::string& model_step) const;
};

#endif // STATEVARMODEL_H
