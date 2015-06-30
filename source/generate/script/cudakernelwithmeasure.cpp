#include "cudakernelwithmeasure.h"

const std::string CudaKernelWithMeasure::MAX_OBJ_VEC_LEN = "16";

CudaKernelWithMeasure::CudaKernelWithMeasure(const std::string& name, const std::string& obj_fun)
    : CudaKernel(name, MatlabBase::CUDA_WM ), _hFileName( MakeHName(name) ), _objectiveFun(obj_fun)
{
}

std::string CudaKernelWithMeasure::ObjectiveFunc() const
{
    std::string obj_fun = ds::StripPath(_objectiveFun);
    const size_t pos = obj_fun.find_last_of('.');
    obj_fun.erase(pos);
    return obj_fun;
}

void CudaKernelWithMeasure::MakeHFile()
{
    std::ofstream hout;
    hout.open(_hFileName);

    const ParamModelBase* variables = _modelMgr->Model(ds::VAR),
            * diffs = _modelMgr->Model(ds::DIFF);
    const size_t num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();

    hout << "//Auto-generated with DynaSys " + ds::VERSION_STR + "\n\n";

    for (size_t i=0; i<num_vars; ++i)
    {
        const std::string key = variables->ShortKey(i) + "_OUTIDX_";
        std::string idx;
        std::transform(key.cbegin(), key.cend(), std::back_inserter(idx), ::toupper);
        hout << "#define " + idx + " " + std::to_string(i) + "\n";
    }
    for (size_t i=0; i<num_diffs; ++i)
    {
        const std::string key = diffs->ShortKey(i) + "_OUTIDX_";
        std::string idx;
        std::transform(key.cbegin(), key.cend(), std::back_inserter(idx), ::toupper);
        hout << "#define " + idx + " " + std::to_string(i+num_vars) + "\n";
    }
}

std::string CudaKernelWithMeasure::Suffix() const
{
    return "_cm";
}

void CudaKernelWithMeasure::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CudaKernelWithMeasure::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++ ] = " + model->ShortKey(i) + ";\n";
    out << "//End CudaKernelWithMeasure::WriteDataOut\n";
    out << "    \n";
}

void CudaKernelWithMeasure::WriteExtraFuncs(std::ofstream& out)
{
    std::ifstream ofun;
    ofun.open(_objectiveFun);

    out << "//Begin CudaKernelWithMeasure::WriteExtraFuncs\n";
    std::string line;
    std::getline(ofun, line);
    while (!ofun.eof())
    {
        out << line << "\n";
        std::getline(ofun, line);
    }
    out << "//End CudaKernelWithMeasure::WriteExtraFuncs\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteIncludes(std::ofstream &out)
{
    CudaKernel::WriteIncludes(out);
    out << "//Begin CudaKernelWithMeasure::WriteIncludes\n";
    out <<"#include \"" + _hFileName + "\"\n";
    out << "//End CudaKernelWithMeasure::WriteIncludes\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteMainBegin(std::ofstream& out)
{
    out <<
           "__global__ void Entry(\n"
           "        const double* input, const int input_len, const int* sput, \n"
           "        const double* par_mat, const int num_pars, const int num_tests,\n"
           "        const double* target, const int* int_lens, const int num_intervals,\n"
           "        const int* mipars,  const double* mdpars, double* out_mat)\n"
           "{\n";
}

void CudaKernelWithMeasure::WriteMainEnd(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteMainEnd\n";
    out <<
           "    double yhat[MAX_YHAT_LEN];\n"
           "    out_mat[idx] =  MeasureRMSE(target, mipars, mdpars, &mstate, yhat); //RMSE\n"
           "    Delete" + ObjectiveFunc() + "(&mstate);\n"
           "}\n";
    out << "//End CudaKernelWithMeasure::WriteMainEnd\n";
}

void CudaKernelWithMeasure::WriteModelLoopBegin(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteModelLoopBegin\n";

    out <<
           "    MState mstate;\n"
           "    mstate.idx = idx;\n"
           "    Init" + ObjectiveFunc() + "(&mstate, mipars, mdpars);\n";

    out << "//End CudaKernelWithMeasure::WriteModelLoopBegin\n";
    out << "\n";
    CFileBase::WriteModelLoopBegin(out);
}

void CudaKernelWithMeasure::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    const std::string num_out = std::to_string(num_vars+num_diffs);
    out << "    double out[" + num_out + "];\n";
    out << "//End CudaKernelWithMeasure::WriteOutputHeader\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteSaveBlockEnd\n";

    out << "            " + ObjectiveFunc()
           + "(i, out, mipars, mdpars, &mstate);\n";
    out << "//End CudaKernelWithMeasure::WriteSaveBlockEnd\n";
}

std::string CudaKernelWithMeasure::MakeHName(const std::string& name) const
{
    std::string out = name;
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += ".h";
    return out;
}
std::string CudaKernelWithMeasure::MakeObjFunName(const std::string& obj_fun) const
{
    std::string out(obj_fun);
    const size_t pos = out.find_last_of('.');
    if (pos==std::string::npos || out.substr(pos)!=".cu")
        throw std::runtime_error("CudaKernelWithMeasure::MakeObjFunName: Objective function must have .cu extension.");
    return out;
}
