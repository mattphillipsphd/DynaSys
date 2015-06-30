#ifndef CUDAKERNEL_H
#define CUDAKERNEL_H

#include "matlabbase.h"

class CudaKernel : public MatlabBase
{
    public:
        CudaKernel(const std::string& name,
                   MatlabBase::FILE_TYPE file_type = MatlabBase::CUDA);

    protected:
        static const std::string IDX_SUF; //Index suffix
        static const std::string STATE_ARR;

        std::string ArrayEltReplace(const std::string& exprn) const;
        virtual std::string FuncArgs(ds::PMODEL mi, size_t i) const override;
        virtual void MakeHFile() override {}
        virtual std::string ObjectiveFunc() const override;
        virtual std::string Suffix() const override;
        std::string ToArrayElt(const std::string& var) const;

        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteFuncs(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteGlobalConst(std::ofstream& out) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out) override;
        virtual void WriteLoadInput(std::ofstream& out) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out) override;
        virtual void WriteSaveBlockBegin(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream& out) override;
        virtual void WriteVarDecls(std::ofstream& out) override;

    private:
        VecStr _allElts;
};

#endif // CUDAKERNEL_H
