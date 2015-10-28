#ifndef DIFFERENTIALMODEL_H
#define DIFFERENTIALMODEL_H

#include "../memrep/modelmgr.h"
#include "numericmodelbase.h"

class DifferentialModel : public NumericModelBase
{
    Q_OBJECT

    public:
        explicit DifferentialModel(QObject *parent, const std::string& name);
        virtual ~DifferentialModel();

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const override { return false; }
        virtual void OpaqueInit(int N) override;

        virtual const void* OpaqueData() const override;
        //Push a new set of state values
        void PushVals(const double* vals, size_t N);

        const double* DiffVals() const { return _diffVals; }

    signals:

    public slots:

    private:
        void CleanUp();

        double* _diffVals;
        VecDeq _stateVals;
};

#endif // DIFFERENTIALMODEL_H
