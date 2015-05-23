#include "matlabbase.h"
#include "matlab_interface/mruncudakernel.h"
#include "matlab_interface/mrunckwm.h"
#include "matlab_interface/mrunmex.h"
#include "matlab_interface/mrunmexwm.h"

MatlabBase::MatlabBase(const std::string& name, const std::string& extension,
                       FILE_TYPE type)
    : CFileBase(name, extension),
      _mDefsFile(new MDefsFile(name)), _mRunFile(MRunFactory(type, name))
{
}

void MatlabBase::MakeMFiles() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MatlabBase::MakeMFiles", std::this_thread::get_id());
#endif
    _mRunFile->SetSuffix( Suffix() ); //Because of inheritance, can't do this in constructor
    _mDefsFile->Make();
    _mRunFile->Make();
}


MRunBase* MatlabBase::MRunFactory(FILE_TYPE type, const std::string& name) const
{
    MRunBase* file = nullptr;
    switch (type)
    {
        case MEX:
            file = new MRunMEX(name);
            break;
        case MEX_WM:
            file = new MRunMEXWM(name);
            break;
        case CUDA:
            file = new MRunCudaKernel(name);
            break;
        case CUDA_WM:
            file = new MRunCKWM(name);
            break;
    }
    return file;
}

