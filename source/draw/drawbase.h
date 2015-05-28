#ifndef DRAWBASE_H
#define DRAWBASE_H

#include <chrono>
#include <mutex>

#include <QFile>
#include <QTime>

#include <qwt_scale_div.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_renderer.h>
#include <qwt_symbol.h>

#include "../globals/globals.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"
#include "../gui/dsplot.h"
#include "../memrep/modelmgr.h"
#include "../memrep/parsermgr.h"

typedef std::vector< std::deque<double> > DataVec;
typedef std::map<std::string, std::string> MapStr;
typedef std::map<std::string, const void*> MapSCV;
typedef std::map<std::string, void*> MapSV;
class DrawBase : public QObject
{
    Q_OBJECT
    
    friend class DrawMgr;

    public:
        enum DRAW_TYPE
        {
            NULLCLINE,
            SINGLE,
            TIME_PLOT,
            USER_NULLCLINE,
            VARIABLE_VIEW,
            VECTOR_FIELD
        };
        enum DRAW_STATE
        {
            STOPPED,
            DRAWING,
            PAUSED
        };

        static const int MAX_BUF_SIZE,
                        TP_WINDOW_LENGTH;
        static const std::string EMPTY_STRING;

        struct Packet
        {
            static const uchar PP_READ, TP_READ;
            Packet(int num_samps, int num_diffs, int num_vars) : num_samples(num_samps),
                ip(new double[num_samps]),
                  diffs(MakeDVec(num_diffs, num_samps)),
                  vars(MakeDVec(num_vars, num_samps)),
                  read_flag(0)
            {}
            Packet(const Packet& pack) : num_samples(pack.num_samples),
                ip(new double[pack.num_samples]),
                  diffs(MakeDVec(pack.diffs.size(), pack.num_samples)),
                  vars(MakeDVec(pack.vars.size(), pack.num_samples)),
                  read_flag(0)
            {
                CopyDVec(diffs, pack.diffs);
                CopyDVec(vars, pack.vars);
                memcpy(ip, pack.ip, num_samples*sizeof(double));
            }
            ~Packet()
            {
                delete[] ip;
                for (auto it: diffs) delete[] it;
                for (auto it: vars) delete[] it;
            }
            const int num_samples;
            double* const ip;
            const std::vector<double*> diffs, vars;
            uchar read_flag;

        private:
            void CopyDVec(const std::vector<double*>& dest, const std::vector<double*>& source)
            {
                const size_t num_elts = source.size();
                for (size_t i=0; i<num_elts; ++i)
                    memcpy(dest[i], source.at(i), num_samples*sizeof(double));
            }
            const std::vector<double*> MakeDVec(size_t num_elts, size_t num_samps)
            {
                std::vector<double*> vec = std::vector<double*>(num_elts);
                for (auto& it : vec) it = new double[num_samps];
                return vec;
            }
        };

        static DrawBase* Create(DRAW_TYPE draw_type, DSPlot* plot);

        virtual ~DrawBase();

        virtual void MakePlotItems() = 0;
        void QuickEval(const std::string&);

        void SetDeleteOnFinish(bool b) { _deleteOnFinish = b; }
        void SetNeedRecompute(bool need_update_parser);
        virtual void SetOpaqueSpec(const std::string& key, const void* value);
        virtual void SetNonConstOpaqueSpec(const std::string& key, void* value);
        void SetSpec(const std::string& key, const std::string& value);
        void SetSpec(const std::string& key, double value);
        void SetSpec(const std::string& key, int value);
        void SetSpecs(const MapStr& specs);

        const void* ConstData() const { return _data; }
        virtual void* DataCopy() const;
        bool DeleteOnFinish() const { return _deleteOnFinish; }
        const ParserMgr& GetParserMgr(size_t i) const { return _parserMgrs.at(i); }
        bool IsSpec(const std::string& key) const;
        long long int IterCt() const;
        long long int IterMax() const { return _iterMax; }
        void* NonConstOpaqueSpec(const std::string& key);
        size_t NumPlotItems() const;
        size_t NumParserMgrs() const;
        const void* OpaqueSpec(const std::string& key) const;
        const DSPlot* Plot() const { return _plot; }
        virtual int SamplesShown() const { return 128 * 1024; }
        virtual int SleepMs() const { return 50; }
        const std::string& Spec(const std::string& key) const;
        bool Spec_tob(const std::string& key) const;
        double Spec_tod(const std::string& key) const;
        int Spec_toi(const std::string& key) const;
        const MapStr& Specs() const;
        DRAW_TYPE Type() const { return _drawType; }

    signals:
        void ComputeComplete(int num_iters);
        void DFlag(); //Only to be used in derived classes
        void Error() const;
        void Flag1();
        void Flag2();
        void Flag3();
        void Flag_i(int);
        void Flag_d(double);
        void Flag_pv(void*);
        void ReadyToDelete();

    protected slots:
        void IterCompleted(int num_iters);

    protected:
        DrawBase(DSPlot* plot);

        void AddPlotItem(QwtPlotItem* plot_item);
        virtual void ClearData() {}
        void ClearPlotItems();
        virtual void ComputeData() = 0;
        void FreezeNonUser();
        virtual void Initialize() = 0;
        void InitParserMgrs(size_t num);
        std::mutex& Mutex() { return _mutex; }
        void RecomputeIfNeeded();
        void RemovePlotItem(const QwtPlotItem* item);
        void ReservePlotItems(size_t num);

        void SetData(void* data);
        void SetDrawState(DRAW_STATE draw_state) { _drawState = draw_state; }

        void* Data() { return _data; }
        DRAW_STATE DrawState() const { return _drawState; }
        ParserMgr& GetParserMgr(size_t i) { return _parserMgrs[i]; }
        std::mutex& Mutex() const { return _mutex; }
        bool NeedNewStep();
        bool NeedRecompute() const { return _needRecompute; }
        QwtPlotItem* PlotItem(size_t i) { return _plotItems[i]; }
        int RemainingSleepMs() const;

        InputMgr* const _inputMgr;
        Log* const _log;
        ModelMgr* const _modelMgr;

    private:
        void DetachItems();
        void ResetIterCt() { _iterCt = 0; }
        void SetIterMax(long long int iter_max) { _iterMax = iter_max; }

        void* _data;
        bool _deleteOnFinish;
        DRAW_STATE _drawState;
        DRAW_TYPE _drawType;
        const std::thread::id _guiTid;
        long long int _iterCt, _iterMax;
        std::chrono::time_point<std::chrono::system_clock> _lastStep;
        mutable std::mutex _mutex;
        bool _needRecompute;
        MapSCV _opaqueSpecs; //Try to use as little as possible obviously!
        MapSV _nonConstOpaqueSpecs; //Try to use as little as possible obviously!
        std::vector<ParserMgr> _parserMgrs;
        DSPlot* _plot;
        std::vector<QwtPlotItem*> _plotItems;
        MapStr _specs;
};

#endif // DRAWBASE_H
