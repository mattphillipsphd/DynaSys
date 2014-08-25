#ifndef TIMEPLOT_H
#define TIMEPLOT_H

#include "drawbase.h"
#include "../models/tpvtablemodel.h"

class TimePlot : public DrawBase
{
    Q_OBJECT

    public:
        TimePlot(DSPlot* plot);

        virtual void ComputeData() override;
        virtual void Initialize() override;
        virtual void MakePlotItems() override;

        virtual int SleepMs() const override { return 100; }
        virtual int SamplesShown() const override { return 1024 * 1024; }

    private:
        std::vector<QColor> _colors;
};

#endif // TIMEPLOT_H
