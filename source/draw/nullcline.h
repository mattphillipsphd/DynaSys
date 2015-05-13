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

        virtual void* DataCopy() const override;
        virtual void MakePlotItems() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;

        virtual int SleepMs() const { return 50; }

    private:
        struct Record
        {
            Record(size_t num_) : num_elts(num_), x(new double[num_]), y(new double[num_])
            {}
            Record(const Record& rec) : num_elts(rec.num_elts), x(new double[num_elts]), y(new double[num_elts]),
                xinc(rec.xinc), yinc(rec.yinc),
                xcross_h(rec.xcross_h), xcross_v(rec.xcross_v), ycross_h(rec.ycross_h), ycross_v(rec.ycross_v)
            {
                memcpy(x, rec.x, num_elts*sizeof(double));
                memcpy(y, rec.y, num_elts*sizeof(double));
            }
            ~Record() { delete[] x; delete[] y; }
            const size_t num_elts;
            double* x, * y;
            double xinc, yinc;
            std::vector< std::pair<int,int> > xcross_h, xcross_v, ycross_h, ycross_v;
        };

        std::vector<QColor> _colors;
        std::deque<Record*> _packets;
};

#endif // NULLCLINE_H
