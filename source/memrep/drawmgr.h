#ifndef DRAWMGR_H
#define DRAWMGR_H

#include "../draw/drawbase.h"
#include "../globals/globals.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"

#include <QDebug>

//#define DEBUG_DM_FUNC

class DrawMgr : public QObject
{
    Q_OBJECT

    public:
        static DrawMgr* Instance();
    
        virtual ~DrawMgr() override;
        
        void AddObject(DrawBase* object);
        void ClearObjects();
        void Pause();
        void QuickEval(const std::string& exprn);
        void Resume();
        void Start();
        void Start(DrawBase::DRAW_TYPE draw_type, int iter_max = -1);
        void Stop();
        void StopAndRemove(DrawBase::DRAW_TYPE draw_type);

        void SetGlobalSpec(const std::string& key, const std::string& value);
        void SetGlobalSpec(const std::string& key, int value);
        void SetNeedRecompute();

        DrawBase::DRAW_STATE DrawState() const;
        DrawBase* GetObject(DrawBase::DRAW_TYPE draw_type);
        DrawBase* GetObject(size_t idx);
        int NumDrawObjects() const;
    
    signals:
        void Error();

    private:
        DrawMgr();
#ifdef __GNUG__
        DrawMgr(const DrawMgr&) = delete;
        DrawMgr& operator=(const DrawMgr&) = delete;
        DrawMgr* operator*(DrawMgr*) = delete;
        const DrawMgr* operator*(const DrawMgr*) = delete;
#endif
        void BroadcastDrawState();
        void StartThread(DrawBase* obj, int iter_max = -1);
        void StartThreads();

        static DrawMgr* _instance;
        
        volatile DrawBase::DRAW_STATE _drawState;
        Log* const _log;
        std::vector<DrawBase*> _objects;        
};

#endif // DRAWMGR_H
