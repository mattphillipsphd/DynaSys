#include "mrunckwm.h"

MRunCKWM::MRunCKWM(const std::string& name) : MRunCudaKernel(name)
{
}

void MRunCKWM::WriteCuCall(std::ofstream& out) const
{
    out <<
           "num_iters = num_records / tau;\n"
           "if isempty(fis)\n"
           "    num_intervals = 1;\n"
           "    input_data = [];\n"
           "    sput = [];\n"
           "    input_len = 0;\n"
           "    interval_len = num_iters;\n"
           "else\n"
           "    num_intervals = fis(1).NumIntervals;\n"
           "    input_len = length(fis(1).Data);\n"
           "    input_data = zeros(input_len, num_fis);\n"
           "    sput = zeros(num_fis,1);\n"
           "    for i=1:num_fis\n"
           "        input_data(:,i) = fis(i).Data;\n"
           "        sput(i) = fis(i).Sput;\n"
           "    end\n"
           "    interval_len = fis(1).IntervalLen*ones(num_intervals,1);\n"
           "end\n"
           "if isempty(target), target = zeros(num_iters,1); end\n"
           "mipars = int32( zeros(6+length(user_mipars), 1) );\n"
           "mipars(1) = num_iters;\n"
           "mipars(2) = num_intervals;\n"
           "mipars(3) = num_iters / num_intervals;\n"
           "mipars(4) = 1.0 / tau;\n"
           "mipars(5) = tau * num_iters / num_intervals;\n"
           "mipars(6) = length(target);\n"
           "for i=1:length(user_mipars), mipars(6+i) = int32( user_mipars(i) ); end\n"
           "mdpars = user_mdpars;\n"
           "data = NaN(num_tests,1);\n"
           "num_chunks = ceil( num_tests / MAX_CHUNK_SIZE );\n"
           "for i=1:num_chunks\n"
           "    if num_chunks>1\n"
           "        disp(['   Starting chunk ' num2str(i) ' of ' num2str(num_chunks) '...']);\n"
           "        chunk_start = toc;\n"
           "    end\n"
           "    idxs = (i-1)*MAX_CHUNK_SIZE+1 : min( [i*MAX_CHUNK_SIZE num_tests] );\n"
           "    out_mat = zeros( block_size_x * k.GridSize(1), k.GridSize(2) );\n"
           "%keyboard;\n"
           "    d = gather( feval(k, ...\n"
           "        input_data, input_len, sput, ...\n"
           "        input_mat(:,idxs), num_inputs, numel(idxs), ...\n"
           "        target, interval_len, num_intervals, ...\n"
           "        mipars, mdpars, out_mat) );\n"
           "    data(idxs) = d( 1:numel(idxs) );\n"
           "    if num_chunks>1\n"
           "        disp(['    ...Finished chunk ' num2str(i) ' of ' num2str(num_chunks) ...\n"
           "                ' in ' num2str(round(toc-chunk_start)) ' seconds.']);\n"
           "    end\n"
           "end\n"
           "\n";
}
void MRunCKWM::WriteChunkSize(std::ofstream& out) const
{
    out <<
           "MAX_CHUNK_SIZE = 1024 * 1024;\n"
           "chunk_size = min([MAX_CHUNK_SIZE num_tests]);\n";
}
void MRunCKWM::WriteDefaultPars(std::ofstream& out) const
{
    MRunCudaKernel::WriteDefaultPars(out);
    out <<
           "if nargin<5, target = []; end\n"
           "if nargin<6, user_mipars = []; end\n"
           "if nargin<7, user_mdpars = []; end\n"
           "\n";
}
void MRunCKWM::WriteDefsCall(std::ofstream& out) const
{
    std::string name_defs = ds::StripPath( NameDefs() );
    if (name_defs.find_last_of('.') != std::string::npos)
        name_defs.erase(name_defs.find_last_of('.'));
    out <<
           "xInfo = " + name_defs + ";\n"
           "inputs = xInfo.inputs;\n"
           "tau = xInfo.tau;\n"
           "\n";
}
void MRunCKWM::WriteHeader(std::ofstream& out) const
{
    std::string name_run = ds::StripPath( NameRun() );
    name_run.erase(name_run.find_last_of('.'));
    out << "function [data, k] = " + name_run
           + "(num_records, save_mod_n, par_mat, ...\n"
           "            fis, target, user_mipars, user_mdpars, k)\n";
}
