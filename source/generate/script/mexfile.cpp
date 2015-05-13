#include "mexfile.h"

MEXFile::MEXFile(const std::string& name, FILE_TYPE file_type)
#ifdef Q_OS_WIN
    : MatlabBase(name, ".cpp", file_type)
#else
    : MatlabBase(name, ".c", file_type)
#endif
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MEXFile::MEXFile", std::this_thread::get_id());
#endif
}

std::string MEXFile::Suffix() const
{
    return "_m";
}

void MEXFile::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin MEXFile::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End MEXFile::WriteDataOut\n";
    out << "    \n";
}
void MEXFile::WriteIncludes(std::ofstream& out)
{
    out << "//Begin MEXFile::WriteIncludes\n";
    out <<
           "#include \"math.h\"\n"
           "#include \"mex.h\"\n"
           "#include \"matrix.h\"\n";
    out << "//End MEXFile::WriteIncludes\n";
    out << "\n";
}
void MEXFile::WriteInitArgs(std::ofstream& out)
{
    out << "//Begin MEXFile::WriteInitArgs\n";
    const ParamModelBase* inputs = _modelMgr->Model(ds::INP),
            * init_conds = _modelMgr->Model(ds::INIT);
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( *mxGetPr(prhs[0]) / tau + 0.5),\n"
           "            save_mod_n = (int)*mxGetPr(prhs[1]);\n"
           "\n";

    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = *mxGetPr(prhs["
               + std::to_string(i+MFileBase::NUM_AUTO_ARGS) + "]);\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = *mxGetPr(prhs["
               + std::to_string(i+num_inputs+MFileBase::NUM_AUTO_ARGS) + "]);\n";
    out << "//End MEXFile::WriteInitArgs\n";
    out << "\n";
}
void MEXFile::WriteLoadInput(std::ofstream& out)
{
    out << "//Begin MEXFile::WriteLoadInput\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::VAR);
    const size_t num_inputs = _modelMgr->Model(ds::INP)->NumPars(),
            num_ics = _modelMgr->Model(ds::INIT)->NumPars(),
            num_vars = variables->NumPars();
    size_t input_ct = num_inputs+num_ics+MFileBase::NUM_AUTO_ARGS;
    for (size_t i=0, ct=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        const std::string& value = variables->Value(i);
        if (Input::Type(value) != Input::INPUT_FILE) continue;

        const std::string var = variables->Key(i),
                inputv = "input_" + var,
                sputv = "sput_" + var, //samples per unit time
                samps_ct = "ct_" + var;
        if (ct++!=0) out << "\n";
        out << "    const double* " + inputv + " = mxGetPr(prhs[" + std::to_string(input_ct++) + "]);\n";
        out << "    int " + sputv + " = (int)*mxGetPr(prhs[" + std::to_string(input_ct++) + "]);\n";
        out <<
               "    int " + samps_ct + " = 0;\n"
               "    " + sputv + " = (int)(1.0/(tau * (double)" + sputv + ") + 0.5);\n";
    }
    out << "//End MEXFile::WriteLoadInput\n";
    out << "\n";
}
void MEXFile::WriteMainBegin(std::ofstream& out)
{
    out <<
           "void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )\n"
           "{\n"
           "    if (nlhs>1)\n"
           "    {\n"
           "        mexErrMsgIdAndTxt( \"MATLAB:test_dsmodel:maxlhs\", \"Too many output arguments.\");\n"
           "    }\n";
}
void MEXFile::WriteMainEnd(std::ofstream& out)
{
    out << "}\n";
}
void MEXFile::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin MEXFile::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    const std::string num_out = std::to_string(num_vars+num_diffs);
    out <<
           "    const int num_fields = " + num_out + ",\n"
           "            num_records = num_iters / save_mod_n;\n"
           "    plhs[0] = mxCreateDoubleMatrix((mwSize)num_records, (mwSize)num_fields, mxREAL);\n"
           "    double* out = mxGetPr(plhs[0]);\n"
           "    int row_ct = 0;\n";
    out << "//End MEXFile::WriteOutputHeader\n";
    out << "\n";
}
void MEXFile::WriteSaveBlockBegin(std::ofstream& out)
{
    out << "            int col = 0;\n";
    out << "\n";
}
void MEXFile::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "            ++row_ct;\n";
    out << "\n";
}
