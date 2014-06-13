#ifndef FDATFILEIN_H
#define FDATFILEIN_H

#include <cstdio>

#include "../globals/globals.h"
#include "../globals/scopetracker.h"

class FDatFileIn
{
    public:
        FDatFileIn(const std::string& name);
        ~FDatFileIn();

        void Read(bool do_init);

        inline const double* Data() const { return _data; }
        inline int Length() const { return _len; }

    private:
        double* _data;
        FILE* _fp;
        int _len;
        const std::string _name;
};

#endif // FDATFILEIN_H
