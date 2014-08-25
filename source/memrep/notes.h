#ifndef NOTES_H
#define NOTES_H

#include "../globals/globals.h"

class Notes
{
    public:
        Notes();

        void Read(std::istream& in);
        void Write(std::ostream& out) const;

        void SetText(const std::string& text) { _text = text; }

        const std::string& Text() const { return _text; }

    private:
        std::string _text;
};

#endif // NOTES_H
