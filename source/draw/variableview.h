#ifndef VARIABLEVIEW_H
#define VARIABLEVIEW_H

#include "drawbase.h"

class VariableView : public DrawBase
{
    Q_OBJECT

    public:
        static const int NUM_FUNCS,
                        NUM_INCREMENTS,
                        NUM_PARSERS;

        VariableView(DSPlot* plot);
        virtual ~VariableView() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;
        virtual void MakePlotItems() override;

        virtual int SleepMs() { return 500; }

    private:
        struct VSpec
        {
            size_t idx;
            double inc, max, min;
            ds::PMODEL mi;
        };
        VSpec MakeVSpec(size_t raw_idx, double num_divs);
};

#endif // VARIABLEVIEW_H
