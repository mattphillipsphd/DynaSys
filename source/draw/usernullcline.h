#ifndef USERNULLCLINE_H
#define USERNULLCLINE_H

#include "drawbase.h"

class UserNullcline : public DrawBase
{
    public:
        UserNullcline(DSPlot* plot);
        virtual ~UserNullcline() override;

        virtual void MakePlotItems() override;

    protected:
        virtual void ComputeData() override;
        virtual void Initialize() override;

        virtual int SleepMs() const { return 50; }

    private:
        struct Record
        {
            Record(size_t nncs) : num_ncs(nncs), x(new double[XRES]), y(new double[nncs*XRES])
            {}
            Record(const Record& rec)
                : num_ncs(rec.num_ncs), x(new double[XRES]), y(new double[rec.num_ncs*XRES])
            {
                memcpy(x, rec.x, XRES*sizeof(double));
                memcpy(y, rec.y, num_ncs*XRES*sizeof(double));
            }
            ~Record() { delete[] x; delete[] y; }
            const size_t num_ncs;
            double* x, * y;
        };

        const static int XRES;
        std::vector<QColor> _colors;
        std::deque<Record*> _packets;
};

#endif // USERNULLCLINE_H
