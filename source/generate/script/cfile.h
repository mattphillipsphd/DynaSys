#ifndef CFILE_H
#define CFILE_H

#include "cfilebase.h"

class CFile : public CFileBase
{
    public:
        CFile(const std::string& name);

    protected:
        virtual void MakeHFile() override {}
        virtual std::string Suffix() const override { return ""; }

        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out) override;
        void WriteVarsOut(std::ofstream& out, ds::PMODEL mi);

    private:
};

#endif // CFILE_H
