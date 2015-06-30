#ifndef MEXFILEWITHMEASURE_H
#define MEXFILEWITHMEASURE_H

#include "mexfile.h"

class MEXFileWithMeasure : public MEXFile
{
    public:
        static const int NUM_MEASURE_ARGS; //ipars, dpars, target

        MEXFileWithMeasure(const std::string& name, const std::string& obj_fun);

        virtual std::string ObjectiveFunc() const override;

    protected:
        virtual void MakeHFile() override;
        virtual std::string Suffix() const override;
        virtual void WriteDataOut(std::ofstream& out, ds::PMODEL mi) override;
        virtual void WriteExtraFuncs(std::ofstream& out) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteModelLoopBegin(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream&) override;

    private:
        std::string MakeHName(const std::string& name) const;
        std::string MakeObjFunName(const std::string& obj_fun) const;

        static const std::string MAX_OBJ_VEC_LEN;

        const std::string _hFileName,
                _objectiveFun;
};

#endif // MEXFILEWITHMEASURE_H
