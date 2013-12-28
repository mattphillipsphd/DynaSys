#ifndef INITIALCONDMODEL_H
#define INITIALCONDMODEL_H

#include "parammodel.h"

class InitialCondModel : public ParamModel
{
    Q_OBJECT

    public:
        explicit InitialCondModel(QObject *parent, const std::string& name);

        virtual bool DoAddToParser() const override { return false; }
        virtual VecStr Initializations() const override;

    signals:

    public slots:

};

#endif // INITIALCONDMODEL_H
