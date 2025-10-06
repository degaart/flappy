#include "engine.hpp"

using namespace zorro;

void Sfx::setFreq(int freq)
{
    CHECK(_sndBuf->SetFrequency(freq));
}

void Sfx::play()
{
    REPORT(_sndBuf->Play(0, 0, 0));
}

void Sfx::stop()
{
    REPORT(_sndBuf->Stop());
}

const char* Sfx::tag() const
{
    return _tag.c_str();
}

