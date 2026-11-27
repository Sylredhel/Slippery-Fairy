#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SlipperyEngine.h"
#include "DSP/FractionalDelayLine.h"
#include <array>
#include <atomic>
#include <memory>

template <int Capacity>
class LockFreeSlipScope
{
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    void reset() noexcept
    {
        inBuf.fill(0.0f);
        outBuf.fill(0.0f);
        writePos.store(0, std::memory_order_relaxed);
    }

    inline void pushPair(float xIn, float yOut) noexcept
    {
        const int idx = writePos.load(std::memory_order_relaxed);
        inBuf[static_cast<size_t>(idx)]  = xIn;
        outBuf[static_cast<size_t>(idx)] = yOut;
        writePos.store((idx + 1) & (Capacity - 1), std::memory_order_release);
    }

    void copyLatest(float* destIn, float* destOut, int count) const noexcept
    {
        const int end = writePos.load(std::memory_order_acquire);
        int pos = (end - count) & (Capacity - 1);
        for (int i = 0; i < count; ++i)
        {
            destIn[i]  = inBuf[static_cast<size_t>(pos)];
            destOut[i] = outBuf[static_cast<size_t>(pos)];
            pos = (pos + 1) & (Capacity - 1);
        }
    }

private:
    std::array<float, Capacity> inBuf {};
    std::array<float, Capacity> outBuf {};
    std::atomic<int> writePos { 0 };
};

class SlipperyFairyAudioProcessor : public juce::AudioProcessor,
                                    private juce::AsyncUpdater
{
public:
    static constexpr int kScopeCapacity = 4096;

    SlipperyFairyAudioProcessor();
    ~SlipperyFairyAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lock-free telemetry for UI oscilloscope, hysteresis loop & mascot animation
    double getCurrentSampleRate() const noexcept    { return currentFsAtomic.load(std::memory_order_relaxed); }
    int    getEngineLatencySamples() const noexcept { return latencyAtomic.load(std::memory_order_relaxed); }
    float  getSlipActivity() const noexcept         { return slipActivityAtomic.load(std::memory_order_relaxed); }
    const LockFreeSlipScope<kScopeCapacity>& getScope() const noexcept { return scope; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void handleAsyncUpdate() override;
    double computeLatencyForOs(int osIdx) const noexcept;

    // Pre-allocated Linear-Phase Equiripple Half-Band FIR Oversampler Bank:
    // index 0 = 2x, 1 = 4x, 2 = 8x, 3 = 16x (steep anti-imaging + anti-aliasing FIR)
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 4> osBank;

    sylredhel::slippery::SlipperyEngine      engine;
    sylredhel::slippery::FractionalDelayLine dryAlignDelay;

    juce::AudioBuffer<float> stereoWorkBuf;
    juce::AudioBuffer<float> dryAlignedBuf;

    // Smoothed continuous parameters
    juce::SmoothedValue<double> smDriveLin, smSlewFreq, smAsymmetry, smDynBias, smAirRecDb;
    juce::SmoothedValue<double> smMix, smOutGainLin, smAutoCompLin;

    // Cached atomic parameter pointers
    std::atomic<float>* pSlipMode     = nullptr;
    std::atomic<float>* pOversampling = nullptr;
    std::atomic<float>* pDriveDb      = nullptr;
    std::atomic<float>* pSlewFreq     = nullptr;
    std::atomic<float>* pAsymmetry    = nullptr;
    std::atomic<float>* pDynBias      = nullptr;
    std::atomic<float>* pAirRecDb     = nullptr;
    std::atomic<float>* pMixPct       = nullptr;
    std::atomic<float>* pOutGainDb    = nullptr;
    std::atomic<float>* pAutoGain     = nullptr;
    std::atomic<float>* pAudition     = nullptr;
    std::atomic<float>* pBypass       = nullptr;

    double hostFs              = 48000.0;
    double exactLatencySamples = 26.0;
    int    activeOsIdx         = 3; // Default 8x (0=OFF, 1=2x, 2=4x, 3=8x, 4=16x)

    std::atomic<double> currentFsAtomic      { 48000.0 };
    std::atomic<int>    latencyAtomic        { 26 };
    std::atomic<int>    pendingLatencyAtomic { 26 };
    std::atomic<float>  slipActivityAtomic   { 0.0f };
    LockFreeSlipScope<kScopeCapacity> scope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlipperyFairyAudioProcessor)
};
