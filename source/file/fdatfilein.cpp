#include "fdatfilein.h"

FDatFileIn::FDatFileIn(const std::string& name)
    : _data(nullptr), _fp(nullptr), _len(-1), _name(name)
{
}
FDatFileIn::~FDatFileIn()
{
    if (_data) delete[] _data;
    if (_fp) fclose(_fp);
}

void FDatFileIn::Read(bool do_init)
{
    _fp = fopen(_name.c_str(), "rb");
    if (!_fp)
        throw std::runtime_error("FDatFileIn::Open: File "
                                 + _name + " didn't open.");

    fread(&_len, sizeof(int), 1, _fp);
    if (do_init) _data = new double[_len];
    fread(_data, sizeof(double), _len, _fp);

    if (_fp) fclose(_fp);
}
