#ifndef MATLABBASE_H
#define MATLABBASE_H

#include "cfilebase.h"
#include "matlab_interface/mdefsfile.h"
#include "matlab_interface/mrunbase.h"

//This abstract class is for all file types that will be run through Matlab
class MatlabBase : public CFileBase
{
    public:
        enum FILE_TYPE
        {
            MEX = 0,
            MEX_WM,
            CUDA,
            CUDA_WM
        };

        MatlabBase(const std::string& name, const std::string& extension, FILE_TYPE type);

        void MakeMFiles() const;

    protected:
        MDefsFile* const _mDefsFile;
        MRunBase* const _mRunFile;

    private:
        MRunBase* MRunFactory(FILE_TYPE type, const std::string& name) const;
};

#endif // MATLABBASE_H
