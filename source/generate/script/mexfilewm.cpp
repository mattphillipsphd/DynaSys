#include "mexfilewm.h"

const int MEXFileWithMeasure::NUM_MEASURE_ARGS = 3;

MEXFileWithMeasure::MEXFileWithMeasure(const std::string& name, const std::string& obj_fun)
    : MEXFile(name, MatlabBase::MEX_WM), _hFileName( MakeHName(name) ), _objectiveFun(obj_fun)
{
}

std::string MEXFileWithMeasure::ObjectiveFunc() const
{
    std::string obj_fun = ds::StripPath(_objectiveFun);
    const size_t pos = obj_fun.find_last_of('.');
    obj_fun.erase(pos);
    return obj_fun;
}

void MEXFileWithMeasure::MakeHFile()
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

std::string MEXFileWithMeasure::Suffix() const
{
    return "_mm";
}

void MEXFileWithMeasure::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin MEXFileWithMeasure::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++ ] = " + model->ShortKey(i) + ";\n";
    out << "//End MEXFileWithMeasure::WriteDataOut\n";
    out << "    \n";
}

void MEXFileWithMeasure::WriteExtraFuncs(std::ofstream& out)
{
    std::ifstream ofun;
    ofun.open(_objectiveFun);

    out << "//Begin MEXFileWithMeasure::WriteExtraFuncs\n";
    std::string line;
    std::getline(ofun, line);
    const std::string dev_str = "__device__ ",
            glob_str = "__global__ ";
    while (!ofun.eof())
    {
        const size_t dpos = line.find(dev_str);
        if (dpos!=std::string::npos) line.erase(dpos, dev_str.length());
        const size_t gpos = line.find(glob_str);
        if (gpos!=std::string::npos) line.erase(gpos, glob_str.length());
        out << line << "\n";
        std::getline(ofun, line);
    }
    out << "//End MEXFileWithMeasure::WriteExtraFuncs\n";
    out << "\n";
}

void MEXFileWithMeasure::WriteIncludes(std::ofstream &out)
{
    MEXFile::WriteIncludes(out);
    out << "//Begin MEXFileWithMeasure::WriteIncludes\n";
    out << "#include \"stdio.h\"\n";
    out << "#include \"string.h\"\n";
    out << "#include \"" + _hFileName + "\"\n";
    out << "//End MEXFileWithMeasure::WriteIncludes\n";
    out << "\n";
}

void MEXFileWithMeasure::WriteInitArgs(std::ofstream& out)
{
    MEXFile::WriteInitArgs(out);
    out << "//Begin MEXFileWithMeasure::WriteInitArgs\n";
    const size_t num_inputs = _modelMgr->Model(ds::INP)->NumPars(),
            num_ics = _modelMgr->Model(ds::INIT)->NumPars(),
            num_input_files = static_cast<const VariableModel*>(
                _modelMgr->Model(ds::VAR))->TypeCount(Input::INPUT_FILE);
    const size_t num_args = MFileBase::NUM_AUTO_ARGS + num_inputs + num_ics + 2*num_input_files;

    out <<
            "const int* ipars = reinterpret_cast<const int*>( mxGetPr(prhs["
                        + std::to_string(num_args) + "]) );\n"
            "const double* dpars = mxGetPr(prhs[" + std::to_string(num_args+1) + "]),\n"
            "           * target = mxGetPr(prhs[" + std::to_string(num_args+2) + "]);\n";
    out << "//End MEXFileWithMeasure::WriteInitArgs\n";
    out << "\n";
}

void MEXFileWithMeasure::WriteMainBegin(std::ofstream& out)
{

    const size_t num_inputs = _modelMgr->Model(ds::INP)->NumPars(),
            num_ics = _modelMgr->Model(ds::INIT)->NumPars(),
            num_input_files = static_cast<const VariableModel*>(
                _modelMgr->Model(ds::VAR))->TypeCount(Input::INPUT_FILE);
    const std::string num_args = std::to_string(
                MFileBase::NUM_AUTO_ARGS + num_inputs + 2*num_input_files + num_ics + NUM_MEASURE_ARGS
                );
    out <<
           "void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )\n"
           "{\n"
           "    if (nlhs>2)\n"
           "    {\n"
           "        mexErrMsgIdAndTxt( \"MATLAB:test_dsmodel:maxlhs\", \"Too many output arguments.\");\n"
           "    }\n"
           "    if (nrhs!=" + num_args + ")\n"
           "    {\n"
           "        mexErrMsgIdAndTxt( \"MATLAB:test_dsmodel:maxrhs\", \"Incorrect number of input arguments.\");\n"
           "    }\n";
}

void MEXFileWithMeasure::WriteMainEnd(std::ofstream& out)
{
    out << "//Begin MEXFileWithMeasure::WriteMainEnd\n";
    out <<
           "    *rmse =  MeasureRMSE(target, ipars, dpars, &mstate, yhat); //RMSE\n"
           "    Delete" + ObjectiveFunc() + "(&mstate);\n"
           "}\n";
    out << "//End MEXFileWithMeasure::WriteMainEnd\n";
}

void MEXFileWithMeasure::WriteModelLoopBegin(std::ofstream& out)
{
    out << "//Begin MEXFileWithMeasure::WriteModelLoopBegin\n";

    out <<
           "    MState mstate;\n"
           "    mstate.idx = -1;\n"
           "    Init" + ObjectiveFunc() + "(&mstate, ipars, dpars);\n";

    out << "//End MEXFileWithMeasure::WriteModelLoopBegin\n";
    out << "\n";
    CFileBase::WriteModelLoopBegin(out);
}

void MEXFileWithMeasure::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin MEXFileWithMeasure::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    const std::string num_out = std::to_string(num_vars+num_diffs);
    out <<
           "    const int num_fields = " + num_out + ",\n"
           "            num_records = num_iters / save_mod_n;\n"
           "    plhs[0] = mxCreateDoubleMatrix((mwSize)1, (mwSize)1, mxREAL);\n"
           "    plhs[1] = mxCreateDoubleMatrix((mwSize)1, (mwSize)m_targ_len, mxREAL);\n"
           "    double* const rmse = mxGetPr(plhs[0]);\n"
           "    double* const yhat = mxGetPr(plhs[1]);\n"
           "    double out[" + num_out + "];\n";
    out << "//End MEXFileWithMeasure::WriteOutputHeader\n";
    out << "\n";
}

void MEXFileWithMeasure::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "//Begin MEXFileWithMeasure::WriteSaveBlockEnd\n";

    out << "            " + ObjectiveFunc()
           + "(i, out, ipars, dpars, &mstate);\n";
    out << "//End MEXFileWithMeasure::WriteSaveBlockEnd\n";
}

std::string MEXFileWithMeasure::MakeHName(const std::string& name) const
{
    std::string out = name;
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += ".h";
    return out;
}
std::string MEXFileWithMeasure::MakeObjFunName(const std::string& obj_fun) const
{
    return obj_fun;
}
