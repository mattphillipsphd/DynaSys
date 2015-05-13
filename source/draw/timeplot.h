#ifndef TIMEPLOT_H
#define TIMEPLOT_H

#include "drawbase.h"
#include "../models/tpvtablemodel.h"

class TimePlot : public DrawBase
{
    Q_OBJECT

    public:
        TimePlot(DSPlot* plot);

        virtual void SetNonConstOpaqueSpec(const std::string &key, void *value) override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;
        virtual void MakePlotItems() override;

        virtual int SleepMs() const override { return 100; }
        virtual int SamplesShown() const override { return 1024 * 1024; }

    private:
        std::vector<QColor> _colors;
        int _eventPointCt;
        double _lastPt;
        std::deque<double> _ip;
        DataVec _diffPts, _varPts;
        std::deque<Packet*> _packets;
};

#endif // TIMEPLOT_H
