#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

SlipperyFairyAudioProcessor::SlipperyFairyAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SlipperyFairyV2Params", createParameterLayout())
{
    for (size_t i = 0; i < 4; ++i)
    {
        const size_t order = i + 1; // 1->2x, 2->4x, 3->8x, 4->16x
        osBank[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, order, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true);
    }

    pSlipMode     = apvts.getRawParameterValue("slipMode");
    pOversampling = apvts.getRawParameterValue("oversampling");
    pDriveDb      = apvts.getRawParameterValue("driveDb");
    pSlewFreq     = apvts.getRawParameterValue("slewFreq");
    pAsymmetry    = apvts.getRawParameterValue("asymmetryPct");
    pDynBias      = apvts.getRawParameterValue("dynBiasPct");
    pAirRecDb     = apvts.getRawParameterValue("airRecDb");
    pMixPct       = apvts.getRawParameterValue("mixPct");
    pOutGainDb    = apvts.getRawParameterValue("outGainDb");
    pAutoGain     = apvts.getRawParameterValue("autoGain");
    pAudition     = apvts.getRawParameterValue("audition");
    pBypass       = apvts.getRawParameterValue("bypass");
}

SlipperyFairyAudioProcessor::~SlipperyFairyAudioProcessor()
{
    cancelPendingUpdate();
}

juce::AudioProcessorValueTreeState::ParameterLayout SlipperyFairyAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. Velocity / Slew Topology, Oversampling & Speed
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "slipMode", 1 }, "Slip Topology",
        juce::StringArray {
            "OP-AMP SID (Asym Slew)",
            "ASYM RC (Charge/Fall)",
            "VEL QUAD (Pure 2nd)",
            "DYN SLIP (Slew+Bias)"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "oversampling", 1 }, "Oversampling",
        juce::StringArray { "OFF (1x)", "2x FIR", "4x FIR", "8x FIR", "16x FIR" }, 3));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "driveDb", 1 }, "Input Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "slewFreq", 1 }, "Slew Ceiling",
        juce::NormalisableRange<float>(300.0f, 20000.0f, 1.0f, 0.32f), 3200.0f));

    // 2. Asymmetry & Harmonic Color
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "asymmetryPct", 1 }, "Rise/Fall Asymmetry",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 65.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dynBiasPct", 1 }, "Dynamic Bias",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airRecDb", 1 }, "Air Recovery",
        juce::NormalisableRange<float>(-6.0f, 12.0f, 0.1f), 0.0f));

    // 3. Output & Monitor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mixPct", 1 }, "Parallel Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outGainDb", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "autoGain", 1 }, "Auto Gain", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "audition", 1 }, "Audition",
        juce::StringArray { "NORMAL", "DELTA (HARMONICS)" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

double SlipperyFairyAudioProcessor::computeLatencyForOs(int osIdx) const noexcept
{
    if (osIdx >= 1 && osIdx <= 4)
        return static_cast<double>(osBank[static_cast<size_t>(osIdx - 1)]->getLatencyInSamples());
    return 0.0;
}

void SlipperyFairyAudioProcessor::handleAsyncUpdate()
{
    setLatencySamples(pendingLatencyAtomic.load(std::memory_order_relaxed));
}

void SlipperyFairyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostFs = std::max(sampleRate, 8000.0);
    currentFsAtomic.store(hostFs, std::memory_order_relaxed);

    const int maxBlock = std::max(4096, samplesPerBlock);
    stereoWorkBuf.setSize(2, maxBlock, false, true, true);
    dryAlignedBuf.setSize(2, maxBlock, false, true, true);

    for (size_t i = 0; i < 4; ++i)
    {
        osBank[i]->initProcessing(static_cast<size_t>(maxBlock));
        osBank[i]->reset();
    }

    activeOsIdx = juce::jlimit(0, 4, static_cast<int>(std::round(pOversampling->load())));
    const double factor = (activeOsIdx == 0) ? 1.0 : static_cast<double>(1 << activeOsIdx);
    engine.prepareForRate(hostFs * factor);
    engine.reset();

    exactLatencySamples = computeLatencyForOs(activeOsIdx);
    const int reportedLat = static_cast<int>(std::round(exactLatencySamples));
    latencyAtomic.store(reportedLat, std::memory_order_relaxed);
    pendingLatencyAtomic.store(reportedLat, std::memory_order_relaxed);
    setLatencySamples(reportedLat);

    const int maxLat = static_cast<int>(std::ceil(computeLatencyForOs(4))) + 64;
    dryAlignDelay.prepare(maxLat);
    scope.reset();

    const double rampSec = 0.015;
    smDriveLin.reset(hostFs, rampSec);
    smSlewFreq.reset(hostFs, rampSec);
    smAsymmetry.reset(hostFs, rampSec);
    smDynBias.reset(hostFs, rampSec);
    smAirRecDb.reset(hostFs, rampSec);
    smMix.reset(hostFs, rampSec);
    smOutGainLin.reset(hostFs, rampSec);
    smAutoCompLin.reset(hostFs, rampSec);

    const double initDriveDb = pDriveDb->load();
    const bool initAuto = pAutoGain->load() > 0.5f;
    smDriveLin.setCurrentAndTargetValue(std::pow(10.0, initDriveDb * 0.05));
    smAutoCompLin.setCurrentAndTargetValue(initAuto ? std::pow(10.0, -initDriveDb * 0.05) : 1.0);
    smSlewFreq.setCurrentAndTargetValue(pSlewFreq->load());
    smAsymmetry.setCurrentAndTargetValue(pAsymmetry->load() * 0.01);
    smDynBias.setCurrentAndTargetValue(pDynBias->load() * 0.01);
    smAirRecDb.setCurrentAndTargetValue(pAirRecDb->load());
    smMix.setCurrentAndTargetValue(pMixPct->load() * 0.01);
    smOutGainLin.setCurrentAndTargetValue(std::pow(10.0, pOutGainDb->load() * 0.05));
}

void SlipperyFairyAudioProcessor::releaseResources()
{
    cancelPendingUpdate();
    for (size_t i = 0; i < 4; ++i)
        osBank[i]->reset();
    engine.reset();
    dryAlignDelay.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SlipperyFairyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto outSet = layouts.getMainOutputChannelSet();
    const auto inSet  = layouts.getMainInputChannelSet();
    if (outSet != juce::AudioChannelSet::mono() && outSet != juce::AudioChannelSet::stereo())
        return false;
    return outSet == inSet;
}
#endif

void SlipperyFairyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInCh    = getTotalNumInputChannels();
    const int numOutCh   = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = numInCh; i < numOutCh; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples <= 0 || numInCh <= 0)
        return;

    if (stereoWorkBuf.getNumSamples() < numSamples)
    {
        stereoWorkBuf.setSize(2, numSamples, false, true, true);
        dryAlignedBuf.setSize(2, numSamples, false, true, true);
    }

    // Live RT-safe Oversampling switch (OFF / 2x / 4x / 8x / 16x)
    const int reqOsIdx = juce::jlimit(0, 4, static_cast<int>(std::round(pOversampling->load())));
    if (reqOsIdx != activeOsIdx)
    {
        activeOsIdx = reqOsIdx;
        if (activeOsIdx >= 1)
            osBank[static_cast<size_t>(activeOsIdx - 1)]->reset();

        const double factor = (activeOsIdx == 0) ? 1.0 : static_cast<double>(1 << activeOsIdx);
        engine.prepareForRate(hostFs * factor);

        exactLatencySamples = computeLatencyForOs(activeOsIdx);
        const int newLat = static_cast<int>(std::round(exactLatencySamples));
        latencyAtomic.store(newLat, std::memory_order_relaxed);
        pendingLatencyAtomic.store(newLat, std::memory_order_relaxed);
        triggerAsyncUpdate();
    }

    const auto mode     = static_cast<sylredhel::slippery::SlipMode>(static_cast<int>(std::round(pSlipMode->load())));
    const auto audition = static_cast<sylredhel::slippery::AuditionMode>(static_cast<int>(std::round(pAudition->load())));
    const bool autoGain = pAutoGain->load() > 0.5f;
    const bool bypassed = pBypass->load() > 0.5f;

    const double driveDb = pDriveDb->load();
    smDriveLin.setTargetValue(std::pow(10.0, driveDb * 0.05));
    smAutoCompLin.setTargetValue(autoGain ? std::pow(10.0, -driveDb * 0.05) : 1.0);
    smSlewFreq.setTargetValue(pSlewFreq->load());
    smAsymmetry.setTargetValue(pAsymmetry->load() * 0.01);
    smDynBias.setTargetValue(pDynBias->load() * 0.01);
    smAirRecDb.setTargetValue(pAirRecDb->load());
    smMix.setTargetValue(pMixPct->load() * 0.01);
    smOutGainLin.setTargetValue(std::pow(10.0, pOutGainDb->load() * 0.05));

    const bool isMono = (numInCh == 1);
    float* ioL = buffer.getWritePointer(0);
    float* ioR = isMono ? nullptr : buffer.getWritePointer(1);

    float* workL = stereoWorkBuf.getWritePointer(0);
    float* workR = stereoWorkBuf.getWritePointer(1);
    float* dryL  = dryAlignedBuf.getWritePointer(0);
    float* dryR  = dryAlignedBuf.getWritePointer(1);

    // 1. Copy input to work buffer & push latency-aligned dry copy
    for (int i = 0; i < numSamples; ++i)
    {
        const double inL = static_cast<double>(ioL[i]);
        const double inR = isMono ? inL : static_cast<double>(ioR[i]);
        workL[i] = static_cast<float>(inL);
        workR[i] = static_cast<float>(inR);

        double aL = 0.0, aR = 0.0;
        dryAlignDelay.pushAndRead(inL, inR, exactLatencySamples, aL, aR);
        dryL[i] = static_cast<float>(aL);
        dryR[i] = static_cast<float>(aR);
    }

    if (bypassed)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ioL[i] = dryL[i];
            if (!isMono) ioR[i] = dryR[i];
            const float m = 0.5f * (dryL[i] + dryR[i]);
            scope.pushPair(m, m);
        }
        slipActivityAtomic.store(0.0f, std::memory_order_relaxed);
        return;
    }

    // 2. Run Oversampled (or 1x) Velocity / Slew Nonlinear Engine
    sylredhel::slippery::SlipParams sp;
    sp.mode = mode;

    auto runEngineLoop = [&](float* chL, float* chR, size_t count, size_t stepDiv)
    {
        for (size_t k = 0; k < count; ++k)
        {
            if (k % stepDiv == 0)
            {
                sp.driveLin   = smDriveLin.getNextValue();
                sp.slewFreqHz = smSlewFreq.getNextValue();
                sp.asymmetry  = smAsymmetry.getNextValue();
                sp.dynBias    = smDynBias.getNextValue();
                sp.airRecDb   = smAirRecDb.getNextValue();
            }

            double l = static_cast<double>(chL[k]);
            double r = static_cast<double>(chR[k]);
            engine.processStereoSample(l, r, sp);
            chL[k] = static_cast<float>(l);
            chR[k] = static_cast<float>(r);
        }
    };

    if (activeOsIdx == 0)
    {
        runEngineLoop(workL, workR, static_cast<size_t>(numSamples), 1);
    }
    else
    {
        auto& os = *osBank[static_cast<size_t>(activeOsIdx - 1)];
        auto block2  = juce::dsp::AudioBlock<float>(stereoWorkBuf).getSubBlock(0, static_cast<size_t>(numSamples));
        auto osBlock = os.processSamplesUp(block2);
        runEngineLoop(osBlock.getChannelPointer(0), osBlock.getChannelPointer(1),
                      osBlock.getNumSamples(), os.getOversamplingFactor());
        os.processSamplesDown(block2);
    }

    // 3. Auto-Gain Compensation, Audition Routing, Parallel Mix & Scope Telemetry
    const double curDrive = smDriveLin.getTargetValue();
    const double invDrv   = 1.0 / std::max(curDrive, 1e-6);

    for (int i = 0; i < numSamples; ++i)
    {
        const double comp = smAutoCompLin.getNextValue();
        const double mix  = smMix.getNextValue();
        const double outG = smOutGainLin.getNextValue();

        const double dL = static_cast<double>(dryL[i]);
        const double dR = static_cast<double>(dryR[i]);

        const double wetRawL  = static_cast<double>(workL[i]);
        const double wetRawR  = static_cast<double>(workR[i]);
        const double wetCompL = wetRawL * comp;
        const double wetCompR = wetRawR * comp;

        double yL = 0.0, yR = 0.0;
        if (audition == sylredhel::slippery::AuditionMode::Delta)
        {
            yL = (wetRawL * invDrv) - dL;
            yR = (wetRawR * invDrv) - dR;
        }
        else
        {
            yL = (1.0 - mix) * dL + mix * wetCompL;
            yR = (1.0 - mix) * dR + mix * wetCompR;
        }

        ioL[i] = static_cast<float>(yL * outG);
        if (!isMono)
            ioR[i] = static_cast<float>(yR * outG);

        const float xScope = static_cast<float>(0.5 * (dL + dR));
        const float yScope = static_cast<float>(0.5 * (wetRawL + wetRawR) * invDrv);
        scope.pushPair(xScope, yScope);
    }

    slipActivityAtomic.store(static_cast<float>(engine.getSlipActivity()), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SlipperyFairyAudioProcessor::createEditor()
{
    return new SlipperyFairyAudioProcessorEditor(*this);
}

void SlipperyFairyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SlipperyFairyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlipperyFairyAudioProcessor();
}
