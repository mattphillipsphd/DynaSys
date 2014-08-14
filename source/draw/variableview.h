#ifndef VARIABLEVIEW_H
#define VARIABLEVIEW_H

#include "drawbase.h"

class VariableView : public DrawBase
{
    Q_OBJECT

    public:
        static const int NUM_ZFUNCS,
                        NUM_INCREMENTS;

        VariableView(DSPlot* plot);
        virtual ~VariableView() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;
        virtual void MakePlotItems() override;

        virtual int SleepMs() { return 250; }

    private:
        struct VSpec
        {
            size_t idx;
            double inc, max, min;
            ds::PMODEL mi;
        };
        VSpec MakeVSpec(size_t raw_idx, double num_divs);

        int _numFuncs;
};

#endif // VARIABLEVIEW_H
