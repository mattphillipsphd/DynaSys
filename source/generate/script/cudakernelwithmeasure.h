#ifndef CUDAKERNELWITHMEASURE_H
#define CUDAKERNELWITHMEASURE_H

#include "cudakernel.h"

class CudaKernelWithMeasure : public CudaKernel
{
    public:
        CudaKernelWithMeasure(const std::string& name, const std::string& obj_fun);

        std::string ObjFunName() const;

    protected:
        virtual void MakeHFile() override;
        virtual std::string NameMDefs() const override;
        virtual std::string NameMRun() const override;
        virtual void WriteCuCall(std::ofstream& out) override;
        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteExtraFuncs(std::ofstream& out) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteMDefsCall(std::ofstream& out) override;
        virtual void WriteMRunArgCheck(std::ofstream& out) override;
        virtual void WriteMRunHeader(std::ofstream& out) override;
        virtual void WriteModelLoopBegin(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream&) override;

    private:
        std::string AddSuffix(const std::string& name) const;
        std::string MakeHName(const std::string& name) const;
        std::string MakeObjFunName(const std::string& obj_fun) const;

        static const std::string MAX_OBJ_VEC_LEN;

        const std::string _hFileName,
                _objectiveFun;
};

#endif // CUDAKERNELWITHMEASURE_H
