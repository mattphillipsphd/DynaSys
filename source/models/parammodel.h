#ifndef PARAMMODEL_H
#define PARAMMODEL_H

#include "numericmodelbase.h"

class ParamModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit ParamModel(QObject *parent, const std::string& name);

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const override { return true; }

    signals:

    public slots:

};

#endif // PARAMMODEL_H
