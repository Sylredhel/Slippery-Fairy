#pragma once
#include <cmath>
#include <algorithm>

namespace sylredhel::slippery
{

enum class SlipMode : int
{
    OpAmpSid  = 0, // Real Op-Amp diff-pair slew ODE: dy/dt = S_pm * tanh(w_bw*(x-y)/S_pm) [RK2]
    AsymRC    = 1, // Asymmetric 1-pole RC charge/discharge impedance (pure +6dB/oct 2nd harmonic)
    VelQuad   = 2, // Velocity quadrature x * dx/dt = 0.5*A^2*w*sin(2wt) (ultra-pure 2nd, ~0% 3rd)
    DynSlip   = 3  // Hybrid Op-Amp Slew + Envelope-Modulated Dynamic Bias (tube/console bloom)
};

enum class AuditionMode : int
{
    Normal = 0,
    Delta  = 1 // Wet - Driven Dry (solo exact generated harmonics & slew rounding)
};

struct SlipParams
{
    SlipMode mode       = SlipMode::OpAmpSid;
    double   driveLin   = 1.0;    // Input drive linear gain (0 .. +24 dB)
    double   slewFreqHz = 3200.0; // Slew / velocity reference corner (300 Hz .. 20 kHz)
    double   asymmetry  = 0.65;   // -1.0 (slow rise / fast fall) .. 0.0 (sym) .. +1.0 (fast rise / slow fall)
    double   dynBias    = 0.0;    // 0.0 .. 1.0 envelope-modulated dynamic bias (2nd-harmonic bloom)
    double   airRecDb   = 0.0;    // -6.0 .. +12.0 dB post-slew air recovery shelf @ 4.5 kHz
};

/**
 * Stereo Velocity / Slew-Based Nonlinear Saturator Core.
 * Automatically adapts internal RK2 sub-stepping & filter coefficients across 1x..16x rates.
 */
class SlipperyEngine
{
public:
    void prepareForRate(double effectiveRate)
    {
        const double nextFs = std::max(effectiveRate, 8000.0);
        if (std::abs(nextFs - fs) < 1.0 && initialized)
            return;

        fs = nextFs;
        dt = 1.0 / fs;
        initialized = true;

        // When running at 1x (e.g. 44.1/48 kHz), use 2 internal RK2 substeps for OP-AMP SID
        // so small-signal open-loop bandwidth stays transparent above 20 kHz & unconditionally stable!
        sidSubSteps = (fs < 88000.0) ? 4 : ((fs < 176000.0) ? 2 : 1);
        sidSubDt = dt / static_cast<double>(sidSubSteps);

        constexpr double twoPi = 6.2831853071795864769;
        const double effSidRate = fs * static_cast<double>(sidSubSteps);
        wBw = twoPi * std::min(34000.0, 0.22 * effSidRate);

        // 10 Hz DC blocker removes static DC offset caused by asymmetric rise/fall
        dcAlpha = std::exp(-twoPi * 10.0 / fs);

        // Velocity differentiator band-limit LPF (~15 kHz) for Mode 2 (VelQuad)
        velLpAlpha = 1.0 - std::exp(-twoPi * std::min(15000.0, 0.34 * fs) / fs);

        // Fast dynamic-bias envelope follower (1.5 ms attack, 28 ms release)
        envAtkAlpha = 1.0 - std::exp(-1.0 / (0.0015 * fs));
        envRelAlpha = 1.0 - std::exp(-1.0 / (0.0280 * fs));

        // Slipping activity detector decay (~45 ms)
        slipDecay = std::exp(-1.0 / (0.045 * fs));

        // Invalidate caches so coefficients rebuild cleanly for the new sample rate
        const double prevSlew = (cachedSlewHz > 0.0) ? cachedSlewHz : 3200.0;
        const double prevAsym = (cachedAsym > -10.0) ? cachedAsym   : 0.65;
        const double prevAir  = (lastAirDb > -100.0) ? lastAirDb    : 0.0;
        cachedSlewHz = -1.0;
        cachedAsym   = -99.0;
        lastAirDb    = -999.0;
        updateSlewCoeffs(prevSlew, prevAsym);
        updateAirShelf(prevAir);
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            slewState[ch]  = 0.0;
            prevX[ch]      = 0.0;
            dxLpState[ch]  = 0.0;
            envState[ch]   = 0.0;
            dcState[ch]    = 0.0;
            dcPrevIn[ch]   = 0.0;
            shZ1[ch]       = 0.0;
            shZ2[ch]       = 0.0;
        }
        slipActivity = 0.0;
    }

    inline void processStereoSample(double& left, double& right, const SlipParams& p) noexcept
    {
        const double fSlew = std::clamp(p.slewFreqHz, 250.0, 22000.0);
        const double asym  = std::clamp(p.asymmetry, -1.0, 1.0);
        if (std::abs(fSlew - cachedSlewHz) > 0.25 || std::abs(asym - cachedAsym) > 1e-4)
            updateSlewCoeffs(fSlew, asym);

        if (std::abs(p.airRecDb - lastAirDb) > 0.02)
            updateAirShelf(p.airRecDb);

        const double inPair[2] = { left * p.driveLin, right * p.driveLin };
        double outPair[2] = { 0.0, 0.0 };

        const double dBias = std::clamp(p.dynBias, 0.0, 1.0);
        double peakSlipNow = 0.0;

        if (p.mode == SlipMode::OpAmpSid || p.mode == SlipMode::DynSlip)
        {
            auto evalDeriv = [this](double yVal, double xIn, double& slipFracOut) noexcept -> double
            {
                const double e = xIn - yVal;
                const double s = (e >= 0.0) ? sUp : sDn;
                const double invS = (e >= 0.0) ? invSUp : invSDn;
                const double u = (wBw * e) * invS;
                const double au = std::abs(u);
                const double th = std::tanh(u);
                slipFracOut = (au > 0.01) ? std::clamp(1.0 - std::abs(th) / au, 0.0, 1.0) : 0.0;
                return s * th;
            };

            for (int ch = 0; ch < 2; ++ch)
            {
                const double xi = inPair[ch];
                for (int sub = 0; sub < sidSubSteps; ++sub)
                {
                    double slip1 = 0.0, slip2 = 0.0;
                    const double k1  = evalDeriv(slewState[ch], xi, slip1);
                    const double mid = slewState[ch] + 0.5 * sidSubDt * k1;
                    const double k2  = evalDeriv(mid, xi, slip2);
                    slewState[ch] += sidSubDt * k2;
                    peakSlipNow = std::max(peakSlipNow, std::max(slip1, slip2));
                }
                if (!std::isfinite(slewState[ch]))
                    slewState[ch] = 0.0;

                outPair[ch] = slewState[ch];
            }
        }
        else if (p.mode == SlipMode::AsymRC)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const double d = inPair[ch] - slewState[ch];
                const double a = (d >= 0.0) ? aUp : aDn;
                slewState[ch] += a * d;
                outPair[ch] = slewState[ch];

                peakSlipNow = std::max(peakSlipNow, std::clamp(std::abs(d) * 1.8, 0.0, 1.0));
            }
        }
        else // SlipMode::VelQuad (x * dx/dt Pure Velocity 2nd-Harmonic Generator)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const double xi = inPair[ch];
                const double dx = (xi - prevX[ch]) * invWnVel;
                prevX[ch] = xi;

                dxLpState[ch] += velLpAlpha * (dx - dxLpState[ch]);
                const double dxBounded = softProtect(dxLpState[ch], 2.0, 4.0);
                const double harm2 = quadAmt * (xi * dxBounded);

                const double raw = xi + harm2;
                outPair[ch] = softProtect(raw, 1.5, 2.8);
                slewState[ch] = outPair[ch];

                peakSlipNow = std::max(peakSlipNow, std::clamp(std::abs(harm2) * 2.2, 0.0, 1.0));
            }
        }

        // Optional Dynamic Envelope-Modulated Bias Stage (active when dynBias > 0 OR in DynSlip mode)
        const double effDynBias = (p.mode == SlipMode::DynSlip)
            ? std::clamp(0.45 + 0.55 * dBias, 0.0, 1.0)
            : dBias;

        for (int ch = 0; ch < 2; ++ch)
        {
            double y = outPair[ch];

            const double absY = std::abs(y);
            const double eAlpha = (absY > envState[ch]) ? envAtkAlpha : envRelAlpha;
            envState[ch] += eAlpha * (absY - envState[ch]);

            if (effDynBias > 1e-4)
            {
                const double bSign = (asym >= -1e-4) ? 1.0 : -1.0;
                const double b = bSign * effDynBias * 0.62 * std::min(envState[ch], 1.8);
                constexpr double head = 1.65;
                y = head * (std::tanh((y + b) / head) - std::tanh(b / head));
            }

            // 10 Hz DC Blocker
            const double hp = dcAlpha * (dcState[ch] + y - dcPrevIn[ch]);
            dcPrevIn[ch] = y;
            dcState[ch]  = hp;

            // Optional Post-Slew Air Recovery High-Shelf (@ 4.5 kHz)
            double finalY = hp;
            if (airShelfActive)
            {
                finalY = shB0 * hp + shZ1[ch];
                shZ1[ch] = shB1 * hp - shA1 * finalY + shZ2[ch];
                shZ2[ch] = shB2 * hp - shA2 * finalY;
            }

            outPair[ch] = finalY;
        }

        if (peakSlipNow > slipActivity)
            slipActivity = peakSlipNow;
        else
            slipActivity *= slipDecay;

        left  = outPair[0];
        right = outPair[1];
    }

    double getSlipActivity() const noexcept { return slipActivity; }

private:
    static inline double softProtect(double x, double knee, double ceilVal) noexcept
    {
        const double ax = std::abs(x);
        if (ax <= knee)
            return x;
        const double span = ceilVal - knee;
        const double y = knee + span * std::tanh((ax - knee) / span);
        return std::copysign(y, x);
    }

    void updateSlewCoeffs(double fSlew, double asym) noexcept
    {
        cachedSlewHz = fSlew;
        cachedAsym   = asym;

        constexpr double twoPi = 6.2831853071795864769;
        const double sBase = twoPi * fSlew;
        const double ratioSid = std::pow(2.0, asym * 1.6);
        sUp    = sBase * ratioSid;
        sDn    = sBase / ratioSid;
        invSUp = 1.0 / std::max(sUp, 1.0);
        invSDn = 1.0 / std::max(sDn, 1.0);

        const double ratioRc = std::pow(2.0, asym * 1.85);
        const double fUp = std::clamp(fSlew * ratioRc, 180.0, 0.42 * fs);
        const double fDn = std::clamp(fSlew / ratioRc, 180.0, 0.42 * fs);
        aUp = 1.0 - std::exp(-twoPi * fUp * dt);
        aDn = 1.0 - std::exp(-twoPi * fDn * dt);

        invWnVel = fs / (twoPi * fSlew);
        const double effAsym = (std::abs(asym) < 0.02) ? 0.02 : asym;
        quadAmt = 0.48 * effAsym;
    }

    void updateAirShelf(double gainDb) noexcept
    {
        lastAirDb = std::clamp(gainDb, -6.0, 12.0);
        airShelfActive = (std::abs(lastAirDb) > 0.02);
        if (!airShelfActive)
        {
            shB0 = 1.0; shB1 = 0.0; shB2 = 0.0; shA1 = 0.0; shA2 = 0.0;
            return;
        }

        const double A  = std::pow(10.0, lastAirDb / 40.0);
        const double w0 = 6.2831853071795864769 * std::min(4500.0, 0.30 * fs) / fs;
        const double cosW = std::cos(w0);
        const double sinW = std::sin(w0);
        const double alpha = (sinW * 0.5) * std::sqrt((A + 1.0 / A) * (1.0 / 0.72 - 1.0) + 2.0);
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;

        const double a0 = (A + 1.0) - (A - 1.0) * cosW + twoSqrtAAlpha;
        const double invA0 = 1.0 / a0;

        shB0 = (A * ((A + 1.0) + (A - 1.0) * cosW + twoSqrtAAlpha)) * invA0;
        shB1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW)) * invA0;
        shB2 = (A * ((A + 1.0) + (A - 1.0) * cosW - twoSqrtAAlpha)) * invA0;
        shA1 = (2.0 * ((A - 1.0) - (A + 1.0) * cosW)) * invA0;
        shA2 = ((A + 1.0) - (A - 1.0) * cosW - twoSqrtAAlpha) * invA0;
    }

    bool   initialized = false;
    double fs          = 384000.0;
    double dt          = 1.0 / 384000.0;
    int    sidSubSteps = 1;
    double sidSubDt    = 1.0 / 384000.0;
    double wBw         = 213628.0;

    double cachedSlewHz = -1.0;
    double cachedAsym   = -99.0;
    double sUp = 22000.0, sDn = 22000.0, invSUp = 1.0 / 22000.0, invSDn = 1.0 / 22000.0;
    double aUp = 0.1, aDn = 0.1;
    double invWnVel = 15.0;
    double quadAmt  = 0.3;

    double dcAlpha     = 0.9998;
    double velLpAlpha  = 0.22;
    double envAtkAlpha = 0.002;
    double envRelAlpha = 0.0001;
    double slipDecay   = 0.9999;

    double slewState[2] = { 0.0, 0.0 };
    double prevX[2]     = { 0.0, 0.0 };
    double dxLpState[2] = { 0.0, 0.0 };
    double envState[2]  = { 0.0, 0.0 };
    double dcState[2]   = { 0.0, 0.0 };
    double dcPrevIn[2]  = { 0.0, 0.0 };

    double lastAirDb = 0.0;
    bool   airShelfActive = false;
    double shB0 = 1.0, shB1 = 0.0, shB2 = 0.0, shA1 = 0.0, shA2 = 0.0;
    double shZ1[2] = { 0.0, 0.0 };
    double shZ2[2] = { 0.0, 0.0 };

    double slipActivity = 0.0;
};

} // namespace sylredhel::slippery
