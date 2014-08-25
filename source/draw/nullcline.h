#ifndef NULLCLINE_H
#define NULLCLINE_H

#include <unordered_set>

#include "drawbase.h"

class Nullcline : public DrawBase
{
    Q_OBJECT

    public:
        Nullcline(DSPlot* plot);
        virtual ~Nullcline() override;

        virtual void MakePlotItems() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;

        virtual int SleepMs() const { return 250; }

    private:
        struct Record
        {
            Record(size_t num_elts) : x(new double[num_elts]), y(new double[num_elts])
            {}
            ~Record() { delete[] x; delete[] y; }
            double* x, * y;
            double xinc, yinc;
            std::vector< std::pair<int,int> > xcross_h, xcross_v, ycross_h, ycross_v;
        };

        std::vector<QColor> _colors;
};

#endif // NULLCLINE_H
