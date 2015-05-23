#ifndef INITIALCONDMODEL_H
#define INITIALCONDMODEL_H

#include "numericmodelbase.h"

class InitialCondModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit InitialCondModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const override { return true; }
        virtual VecStr Initializations() const override;
        virtual std::string ShortKey(size_t idx) const override;
        virtual int ShortKeyIndex(const std::string& par_name) const override;

    signals:

    public slots:

};

#endif // INITIALCONDMODEL_H
