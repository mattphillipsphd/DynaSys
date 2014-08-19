#include "cudakernelwithmeasure.h"

const std::string CudaKernelWithMeasure::MAX_OBJ_VEC_LEN = "256";

CudaKernelWithMeasure::CudaKernelWithMeasure(const std::string& name, const std::string& obj_fun)
    : CudaKernel(name), _hFileName( MakeHName(name) ), _objectiveFun(obj_fun)
{
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

void CudaKernelWithMeasure::WriteCuCall(std::ofstream& out)
{
    out <<
           "num_iters = round( num_records/model_step );\n"
           "num_intervals = length(target);\n"
           "out_mat = zeros(num_tests,1);\n"
           "data = gather( feval(k, fi.Data, length(fi.Data), fi.Sput, ...\n"
           "    input_mat, num_inputs, num_tests, ...\n"
           "    target, fi.IntervalLen*ones(num_intervals,1), num_intervals, ...\n"
           "    out_mat) );\n"
           "\n";
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

void CudaKernelWithMeasure::WriteIncludes(std::ofstream &out)
{
    out << "//Begin CudaKernelWithMeasure::WriteIncludes\n";
    out <<
           "#include \"math.h\"\n"
           "#include \"" + _objectiveFun + "\"\n";
    out << "//End CudaKernelWithMeasure::WriteIncludes\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteMainBegin(std::ofstream& out)
{
    out <<
           "__global__ void Entry(\n"
           "        const double* input, const int input_len, const int sput, \n"
           "        const double* par_mat, const int num_pars, const int num_tests,\n"
           "        const double* target, const int* int_lens, const int num_intervals,\n"
           "        double* out_mat)\n"
           "{\n";
}

void CudaKernelWithMeasure::WriteMainEnd(std::ofstream& out)
{
    out <<
           "    double sse = 0, rmse;\n"
           "    for (int i=0; i<num_intervals; ++i)\n"
           "    {\n"
           "        const double yhat_i = yhat[i], tg_i = target[i];\n"
           "        sse += (yhat_i - tg_i)*(yhat_i - tg_i);\n"
           "        //sse +=  2*(yhat_i>tg_i ? yhat_i : tg_i) / (yhat_i + tg_i) - 1;\n"
           "    }\n"
           "    rmse = sqrt(sse/num_intervals);\n"
           "    \n"
           "    out_mat[idx] = rmse;\n"
           "}\n";
}

void CudaKernelWithMeasure::WriteModelLoopBegin(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteModelLoopBegin\n";

    out <<
           "    const int iters_per_interval = num_iters / num_intervals;\n"
           "    double yhat[" + MAX_OBJ_VEC_LEN + "];\n"
           "    memset(yhat, 0, sizeof(double)*" + MAX_OBJ_VEC_LEN + ");\n";

    out << "//End CudaKernelWithMeasure::WriteModelLoopBegin\n";
    out << "\n";
    CFileBase::WriteModelLoopBegin(out);
}

void CudaKernelWithMeasure::WriteMRunHeader(std::ofstream& out)
{
    std::string name_run = ds::StripPath( NameMRun() );
    name_run.erase(name_run.find_last_of('.'));
    out << "function [data, k] = " + name_run
           + "(num_records, save_mod_n, par_mat, is_test, fi, target, k)\n";
}

void CudaKernelWithMeasure::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    std::string num_out = std::to_string(num_vars+num_diffs);
    out <<
           "    const int num_records = num_iters / save_mod_n;\n"
           "    double out[" + num_out + "];\n";
    out << "//End CudaKernel::WriteOutputHeader\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteSaveBlockEnd\n";
    std::string obj_fun = ds::StripPath(_objectiveFun);
    const size_t pos = obj_fun.find_last_of('.');
    obj_fun.erase(pos);

    out << "            " + obj_fun + "(i, out, yhat, &iters_per_interval, 0);\n";
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
