#ifndef DRAWBASE_H
#define DRAWBASE_H

#include <chrono>
#include <mutex>

#include <QFile>

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
class DrawBase : public QObject
{
    Q_OBJECT
    
    friend class DrawMgr;

    public:
        enum DRAW_TYPE
        {
            NULL_CLINE,
            PHASE_PLOT,
            TIME_PLOT,
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

        static DrawBase* Create(DRAW_TYPE draw_type, DSPlot* plot);

        virtual ~DrawBase();

        virtual void MakePlotItems() = 0;
        void QuickEval(const std::string&);

        void SetDeleteOnFinish(bool b) { _deleteOnFinish = b; }
        void SetNeedRecompute(bool need_update_parser);
        void SetOpaqueSpec(const std::string& key, const void* value);
        void SetSpec(const std::string& key, const std::string& value);
        void SetSpec(const std::string& key, double value);
        void SetSpec(const std::string& key, int value);
        void SetSpecs(const MapStr& specs);

        const void* ConstData() const { return _data; }
        virtual void* DataCopy() const;
        bool DeleteOnFinish() const { return _deleteOnFinish; }
        const ParserMgr& GetParserMgr(size_t i) const { return _parserMgrs.at(i); }
        bool IsSpec(const std::string& key) const;
        long long int IterCt() const { return _iterCt; }
        long long int IterMax() const { return _iterMax; }
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
        void Error() const;
        void Flag1();
        void Flag2();
        void Flag3();
        void ReadyToDelete();

    protected slots:
        void IterCompleted(int num_iters);

    protected:
        DrawBase(DSPlot* plot);

        void AddPlotItem(QwtPlotItem* plot_item);
        void ClearPlotItems();
        virtual void ComputeData() = 0;
        void FreezeNonUser();
        virtual void Initialize() = 0;
        void InitParserMgrs(size_t num);
        std::recursive_mutex& Mutex() { return _mutex; }
        void RecomputeIfNeeded();
        void ReservePlotItems(size_t num);

        void SetData(void* data) { _data = data; }
        void SetDrawState(DRAW_STATE draw_state) { _drawState = draw_state; }

        void* Data() { return _data; }
        DRAW_STATE DrawState() const { return _drawState; }
        ParserMgr& GetParserMgr(size_t i) { return _parserMgrs[i]; }
        std::recursive_mutex& Mutex() const { return _mutex; }
        bool NeedNewStep();
        bool NeedRecompute() const { return _needRecompute; }
        QwtPlotItem* PlotItem(size_t i) { return _plotItems[i]; }
        int RemainingSleepMs() const;

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
        long long int _iterCt, _iterMax;
        std::chrono::time_point<std::chrono::system_clock> _lastStep;
        mutable std::recursive_mutex _mutex;
        bool _needRecompute;
        MapSCV _opaqueSpecs; //Try to use as little as possible obviously!
        std::vector<ParserMgr> _parserMgrs;
        DSPlot* _plot;
        std::vector<QwtPlotItem*> _plotItems;
        MapStr _specs;
};

#endif // DRAWBASE_H
