#ifndef MATLABBASE_H
#define MATLABBASE_H

#include "cfilebase.h"

//This abstract class is for all file types that will be run through Matlab
class MatlabBase : public CFileBase
{
    public:
        MatlabBase(const std::string& name, const std::string& extension);

        void MakeMFiles();

        virtual std::string NameMDefs() const = 0;
        virtual std::string NameMRun() const = 0;

    protected:
        virtual void MakeMDefsFile();
        virtual void MakeMRunFile() = 0;

        static const int NUM_AUTO_ARGS;

    private:
};

#endif // MATLABBASE_H
