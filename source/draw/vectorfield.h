#ifndef VECTORFIELD_H
#define VECTORFIELD_H

#include "arrowhead.h"
#include "drawbase.h"

class VectorField : public DrawBase
{
    Q_OBJECT

    public:
        VectorField(DSPlot* plot);
        virtual ~VectorField() override;

    protected:
        virtual void MakePlotItems() override;
        virtual void ComputeData() override;
        virtual void Initialize() override;

    private:
        static const int DEFAULT_TAIL_LEN;

        void InitParserMgrs();
        void ResetPlotItems(); //Reset parser and plot items

        size_t _resolution; //thread-local
        int _tailLength;
        std::deque<QPolygonF*> _packets;
};

#endif // VECTORFIELD_H
