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

        virtual void* DataCopy() const override;
        virtual void MakePlotItems() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;

    private:
        static const int DEFAULT_TAIL_LEN;

        void ResetPPs(); //Reset parser and plot items

        int _tailLength;
};

#endif // VECTORFIELD_H
