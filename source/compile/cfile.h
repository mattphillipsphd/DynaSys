#ifndef CFILE_H
#define CFILE_H

#include "cfilebase.h"

class CFile : public CFileBase
{
    public:
        CFile(const std::string& name);

    protected:
        virtual void MakeHFile() override {}

        virtual void WriteDataOut(std::ofstream& out, const ParamModelBase* model) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                           const ParamModelBase* init_conds) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs) override;
        void WriteVarsOut(std::ofstream& out, const ParamModelBase* variables);

    private:
};

#endif // CFILE_H
