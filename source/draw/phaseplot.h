#ifndef PHASEPLOT_H
#define PHASEPLOT_H

#include "drawbase.h"

class PhasePlot : public DrawBase
{
    Q_OBJECT

    public:
        PhasePlot(DSPlot* plot);
        virtual ~PhasePlot() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;
        virtual void MakePlotItems() override;

    private:
        QwtPlotCurve* _curve;
        QwtPlotMarker* _marker;
        int _pastDVSampsCt, _pastIPSampsCt; //Samples outside the buffer
};

#endif // PHASEPLOT_H
