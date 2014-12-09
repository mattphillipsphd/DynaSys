#ifndef PHASEPLOT_H
#define PHASEPLOT_H

#include "drawbase.h"

class PhasePlot : public DrawBase
{
    Q_OBJECT

    public:
        PhasePlot(DSPlot* plot);
        virtual ~PhasePlot() override;

        virtual void* DataCopy() const override;
//        virtual void* DataCopy2() const;
        virtual void MakePlotItems() override;
        virtual int SleepMs() const override;

    protected:
        virtual void ComputeData() override;
        virtual void ClearData() override;
        virtual void Initialize() override;

    private:
        int PacketSampCt() const;

        QwtPlotCurve* _curve;
        DataVec _diffPts;
        bool _makePlots;
        QwtPlotMarker* _marker;
        std::deque<Packet*> _packets;
        int _pastDVSampsCt, _pastIPSampsCt; //Samples outside the buffer
};

#endif // PHASEPLOT_H
