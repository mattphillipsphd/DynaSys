#ifndef MEXFILE_H
#define MEXFILE_H

#include "matlabbase.h"

class MEXFile : public MatlabBase
{
    public:
        MEXFile(const std::string& name,
                MatlabBase::FILE_TYPE file_type = MatlabBase::MEX);

        virtual std::string ObjectiveFunc() const override;

    protected:
        virtual void MakeHFile() override {}
        virtual std::string Suffix() const override;

        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out) override;
        virtual void WriteLoadInput(std::ofstream& out) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out) override;
        virtual void WriteSaveBlockBegin(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream& out) override;

    private:
};

#endif // MEXFILE_H
