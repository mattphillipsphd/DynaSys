#include "log.h"

const int Log::MAXLEN = 1024 * 64;

Log::Log()
{
}

void Log::AddExcept(const std::string& except)
{
    std::unique_lock<std::mutex> lock(_mutex);
    std::string mesg = "Exception: " + except;
    std::cerr << mesg << std::endl;
    Add(mesg);
    lock.unlock();
    emit OpenGui();
}

void Log::AddMesg(const std::string& mesg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Add(mesg);
}
void Log::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _buffer.clear();
}
void Log::SendBuffer()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& it : _buffer)
        emit SendMesg(it.first.c_str(), it.second);
}
void Log::Write(std::ofstream& out) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& it : _buffer)
        out << it.first;
}

void Log::Add(const std::string& mesg)
{
    std::thread::id tid = std::this_thread::get_id();
    QColor color = ds::ThreadColor(tid);
    _buffer.push_back( std::make_pair(mesg, color) );
#ifdef QT_DEBUG
    qDebug() << mesg.c_str() << color;
#endif
    emit SendMesg(mesg.c_str(), color);
    if (_buffer.size() > MAXLEN) _buffer.clear();
}
