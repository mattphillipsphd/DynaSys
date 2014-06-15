#ifndef DATFILEOUT_H
#define DATFILEOUT_H

#include <cstdio>

#include "../globals/scopetracker.h"

class DatFileOut
{
    public:
        DatFileOut(const std::string& name);
        ~DatFileOut();

        void Close();
        void Open(const VecStr& fields, int nth_sample);
        void Write(const double* data, int N);

    private:
        static const int BUFFER_SIZE;

        int _bufCt;
        double* const _buffer;
        const std::string _name;
        int _numElts, _numFields;
        mutable FILE* _out;
        long _recordsPos;
};

#endif // DATFILEOUT_H
