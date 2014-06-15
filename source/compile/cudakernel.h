#ifndef CUDAKERNEL_H
#define CUDAKERNEL_H

#include "cfilebase.h"

class CudaKernel : public CFileBase
{
    public:
        CudaKernel(const std::string& name);

    protected:
        virtual void MakeHFile() override {}

        virtual void WriteDataOut(std::ofstream& out, const ParamModelBase* model) override;
        virtual void WriteFuncs(std::ofstream& out, const ParamModelBase* model) override;
        virtual void WriteGlobalConst(std::ofstream& out, const ParserMgr& parser_mgr) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                           const ParamModelBase* init_conds) override;
        virtual void WriteLoadInput(std::ofstream& out, const ParamModelBase* variables) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs) override;
        virtual void WriteSaveBlockBegin(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream& out) override;
        virtual void WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr) override;

//        virtual std::string MakeName(const std::string& name) const override;

    private:
        static const int NUM_AUTO_ARGS;

        int _inputCt;
};

#endif // CUDAKERNEL_H
