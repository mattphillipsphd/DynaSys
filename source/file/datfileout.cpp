#include "datfileout.h"

const int DatFileOut::BUFFER_SIZE = 16 * 1024 * 1024;

DatFileOut::DatFileOut(const std::string& name)
    : _bufCt(0), _buffer(new double[BUFFER_SIZE]), _name(name),
      _numElts(0), _out(nullptr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DatFileOut::DatFileOut", std::this_thread::get_id());
#endif
}
DatFileOut::~DatFileOut()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DatFileOut::~DatFileOut", std::this_thread::get_id());
#endif
    delete[] _buffer;
}

void DatFileOut::Close()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DatFileOut::Close", std::this_thread::get_id());
#endif
    fwrite(_buffer, sizeof(double), _bufCt, _out);
    _numElts += _bufCt;
    const int num_records = _numElts/_numFields;
    fseek(_out, _recordsPos, SEEK_SET);
    fwrite(&num_records, sizeof(int), 1, _out);
    fclose(_out);
}

void DatFileOut::Open(const VecStr& fields, int nth_sample)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DatFileOut::Open", std::this_thread::get_id());
#endif
    _out = fopen( (_name).c_str() , "wb");
    if (!_out)
        throw std::runtime_error("DatFileOut::Open: Bad File.");

    const int vnum = ds::VersionNum();
    fwrite(&vnum, sizeof(int), 1, _out);

    _numFields = (int)fields.size();
    fwrite(&_numFields, sizeof(int), 1, _out);

    for (const auto& it : fields)
    {
        const int len = (int)it.length();
        fwrite(&len, sizeof(int), 1, _out);
        fputs(it.c_str(), _out);
    }

    fwrite(&nth_sample, sizeof(int), 1, _out);

    _recordsPos = ftell(_out);
    const int num_records = 0;
    fwrite(&num_records, sizeof(int), 1, _out);
}
void DatFileOut::Write(const double* data, int N)
{
    if (_bufCt+N > BUFFER_SIZE)
    {
        const int rem = BUFFER_SIZE - _bufCt;
        memcpy(_buffer+_bufCt, data, rem*sizeof(double));
        fwrite(_buffer, sizeof(double), BUFFER_SIZE, _out);
        _bufCt = 0;
        _numElts += BUFFER_SIZE;
    }
    else
    {
        memcpy(_buffer+_bufCt, data, N*sizeof(double));
        _bufCt += N;
    }
}
