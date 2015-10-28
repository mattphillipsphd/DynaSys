#include "differentialmodel.h"

DifferentialModel::DifferentialModel(QObject *parent, const std::string& name) :
    NumericModelBase(parent, name), _diffVals(nullptr)
{
    const size_t num_diffs = NumPars();
    _stateVals = VecDeq(num_diffs);
    for (auto& it : _stateVals)
        it = std::deque<double>();

    _diffVals = new double[num_diffs];
}
DifferentialModel::~DifferentialModel()
{
    CleanUp();
}

const void* DifferentialModel::OpaqueData() const
{
    return _diffVals;
}
void DifferentialModel::OpaqueInit(int N)
{
    _stateVals = VecDeq(N);
    for (auto& it : _stateVals)
        it = std::deque<double>();

    CleanUp();
    _diffVals = new double[N];
}

void DifferentialModel::PushVals(const double* vals, size_t)
{
    const size_t SPUT = 1.0 / ModelMgr::Instance()->ModelStep() + 0.5;
    size_t i = 0;
    for (auto& it : _stateVals)
    {
        if (it.size() > SPUT) it.pop_front();
        it.push_back( vals[i++] );
    }

    const size_t num_pars = NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        const double val = _stateVals.at(i).back() - _stateVals.at(i)[0];
        _diffVals[i] = val;
    }
}

void DifferentialModel::CleanUp()
{
    delete[] _diffVals;
}
