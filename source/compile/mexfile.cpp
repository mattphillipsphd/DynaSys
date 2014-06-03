#include "mexfile.h"

MEXFile::MEXFile(const std::string& name) : CFileBase(name), _nameM(MakeMName(name))
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MEXFile::MEXFile", std::this_thread::get_id());
#endif
}

void MEXFile::MakeMFile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MEXFile::MakeMFile", std::this_thread::get_id());
#endif
    std::ofstream mout;
    mout.open(_nameM);

    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS),
            * init_conds = parser_mgr.Model(ds::INIT_CONDS),
            * variables = parser_mgr.Model(ds::VARIABLES),
            * diffs = parser_mgr.Model(ds::DIFFERENTIALS);
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars(),
            num_columns = num_inputs+num_ics,
            num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();

    std::string name_m = ds::StripPath(_nameM);
    name_m.erase(name_m.find_last_of('.'));

    mout <<
            "function [input_names, inputs, output_names, data] = "
                    + name_m + "(num_records, save_mod_n)\n"
            "\n";

    //All the input parameter names
    mout <<
           "input_names = cell(" + std::to_string(num_columns+2) + ",1);\n"
           "input_names{1} = 'num_records';\n"
           "input_names{2} = 'save_mod_n';\n";
    size_t inn_ct = 3;
    for (size_t i=0; i<num_inputs; ++i, ++inn_ct)
        mout << "input_names{" + std::to_string(inn_ct) + "} = '" + inputs->Key(i) + "';\n";
    for (size_t i=0; i<num_ics; ++i, ++inn_ct)
        mout << "input_names{" + std::to_string(inn_ct) + "} = '" + init_conds->ShortKey(i) + "0';\n";
    mout << "\n";

    //All the inputs
    mout <<
           "inputs = cell(" + std::to_string(num_columns+2) + ",1);\n"
           "inputs{1} = num_records;\n"
           "inputs{2} = save_mod_n;\n";
    size_t in_ct = 3;
    for (size_t i=0; i<num_inputs; ++i, ++in_ct)
        mout << "inputs{" + std::to_string(in_ct) + "} = " + inputs->Value(i)
                + ";\t%" + inputs->Key(i) + "\n";
    for (size_t i=0; i<num_ics; ++i, ++in_ct)
        mout << "inputs{" + std::to_string(in_ct) + "} = " + init_conds->Value(i)
                + ";\t%" + init_conds->Key(i) + "\n";
    mout << "\n";

    //The column names for the output matrix
    mout << "output_names = cell(" + std::to_string(num_vars+num_diffs) + ",1);\n";
    size_t out_ct = 1;
    for (size_t i=0; i<num_vars; ++i, ++out_ct)
        mout << "output_names{" + std::to_string(out_ct) + "} = '" + variables->Key(i) + "';\n";
    for (size_t i=0; i<num_diffs; ++i, ++out_ct)
        mout << "output_names{" + std::to_string(out_ct) + "} = '" + diffs->ShortKey(i) + "';\n";
    mout << "\n";


    name_m.erase( name_m.find_last_of("_m")-1 );
    mout <<
            "data = " + name_m + "(inputs{:});\n"
            "\n"
            "end\n";

    mout.close();
}

void MEXFile::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin WriteDataOut\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End WriteDataOut\n";
    out << "    \n";
}
void MEXFile::WriteIncludes(std::ofstream& out)
{
    out << "//Begin WriteIncludes\n";
    out <<
           "#include \"math.h\"\n"
           "#include \"mex.h\"\n"
           "#include \"matrix.h\"\n";
    out << "//End WriteIncludes\n";
    out << "\n";
}
void MEXFile::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( *mxGetPr(prhs[0]) / tau ),\n"
           "            save_mod_n = (int)*mxGetPr(prhs[1]);\n"
           "\n";

    const int NUM_AUTO_ARGS = 2;
    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = *mxGetPr(prhs["
               + std::to_string(i+NUM_AUTO_ARGS) + "]);\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = *mxGetPr(prhs["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "]);\n";
    out << "//End WriteInitArgs\n";
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
    out <<
           "    mexPrintf(\"" + ds::StripPath(Name()) + " exited without crashing.\\n\");\n"
           "}\n";
}
void MEXFile::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin WriteOutputHeader\n";
    const size_t num_fields = variables->NumPars()+diffs->NumPars();
    out <<
           "    const int num_fields = " + std::to_string(num_fields) + ",\n"
           "            num_records = num_iters / save_mod_n;\n"
           "    plhs[0] = mxCreateDoubleMatrix((mwSize)num_records, (mwSize)num_fields, mxREAL);\n"
           "    double* out = mxGetPr(plhs[0]);\n"
           "    int row_ct = 0;\n";
    out << "//End WriteOutputHeader\n";
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

std::string MEXFile::MakeMName(const std::string& name) const
{
    std::string out(name);
    out.erase( name.find_last_of('.') );
    out += "_m.m";
    return out;
}
