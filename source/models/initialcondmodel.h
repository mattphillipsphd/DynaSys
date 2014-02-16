#ifndef INITIALCONDMODEL_H
#define INITIALCONDMODEL_H

#include "parammodelbase.h"

class InitialCondModel : public ParamModelBase
{
    Q_OBJECT

    public:
        explicit InitialCondModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const override { return true; }
        virtual VecStr Initializations() const override;
        virtual std::string ShortKey(size_t idx) const override;

    signals:

    public slots:

};

#endif // INITIALCONDMODEL_H
