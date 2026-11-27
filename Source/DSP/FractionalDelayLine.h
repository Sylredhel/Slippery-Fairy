#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace sylredhel::slippery
{

class FractionalDelayLine
{
public:
    void prepare(int maxDelaySamples)
    {
        int sz = 16;
        while (sz < maxDelaySamples + 16)
            sz <<= 1;
        bufSize = sz;
        bufMask = sz - 1;
        for (int ch = 0; ch < 2; ++ch)
            buf[ch].assign(static_cast<size_t>(bufSize), 0.0);
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            std::fill(buf[ch].begin(), buf[ch].end(), 0.0);
        writePos = 0;
    }

    inline void pushAndRead(double inL, double inR, double delaySamples,
                            double& outL, double& outR) noexcept
    {
        buf[0][static_cast<size_t>(writePos)] = inL;
        buf[1][static_cast<size_t>(writePos)] = inR;

        if (delaySamples < 1.0)
        {
            outL = inL;
            outR = inR;
            writePos = (writePos + 1) & bufMask;
            return;
        }

        const double d = std::clamp(delaySamples, 1.0, static_cast<double>(bufSize - 4));
        const double readPos = static_cast<double>(writePos) - d;
        const int i1 = static_cast<int>(std::floor(readPos));
        const double f = readPos - static_cast<double>(i1);

        const int idx0 = (i1 - 1) & bufMask;
        const int idx1 = (i1)     & bufMask;
        const int idx2 = (i1 + 1) & bufMask;
        const int idx3 = (i1 + 2) & bufMask;

        const double c0 = -f * (f - 1.0) * (f - 2.0) * (1.0 / 6.0);
        const double c1 =  (f + 1.0) * (f - 1.0) * (f - 2.0) * 0.5;
        const double c2 = -(f + 1.0) * f * (f - 2.0) * 0.5;
        const double c3 =  (f + 1.0) * f * (f - 1.0) * (1.0 / 6.0);

        outL = c0 * buf[0][static_cast<size_t>(idx0)] + c1 * buf[0][static_cast<size_t>(idx1)]
             + c2 * buf[0][static_cast<size_t>(idx2)] + c3 * buf[0][static_cast<size_t>(idx3)];
        outR = c0 * buf[1][static_cast<size_t>(idx0)] + c1 * buf[1][static_cast<size_t>(idx1)]
             + c2 * buf[1][static_cast<size_t>(idx2)] + c3 * buf[1][static_cast<size_t>(idx3)];

        writePos = (writePos + 1) & bufMask;
    }

private:
    int bufSize = 512;
    int bufMask = 511;
    int writePos = 0;
    std::vector<double> buf[2];
};

} // namespace sylredhel::slippery
