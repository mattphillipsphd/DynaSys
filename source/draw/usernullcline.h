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
        std::string DependentVar(size_t i) const;
        std::string EquationVar(size_t i) const;
        virtual void Initialize() override;

        virtual int SleepMs() const { return 50; }

    private:
        enum EQ_CAT
        {
            UNKNOWN = -1,
            STABLE,
            SADDLE,
            UNSTABLE
        };
        struct Equilibrium
        {
            Equilibrium(double x, double y, EQ_CAT ec = UNKNOWN)
                : x(x), y(y), eq_cat(ec)
            {}
            Equilibrium() : eq_cat(UNKNOWN)
            {}
            double x, y;
            EQ_CAT eq_cat;
        };

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
            std::vector<Equilibrium> equilibria;
            const size_t num_ncs;
            double* x, * y;
        };

        bool LineIntersection(double p0_x, double p0_y, double p1_x, double p1_y,
            double p2_x, double p2_y, double p3_x, double p3_y, double *i_x, double *i_y);

        const static int XRES;
        std::vector<QColor> _colors;
        std::deque<Record*> _packets;
};

#endif // USERNULLCLINE_H
