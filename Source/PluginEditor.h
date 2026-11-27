#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "LookAndFeelSketchbook.h"

class SlipperyFairyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit SlipperyFairyAudioProcessorEditor(SlipperyFairyAudioProcessor&);
    ~SlipperyFairyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& s, const juce::String& suffix, uint32_t accentHex,
                   bool bipolar, const juce::String& tip);
    void registerTip(juce::Component& c, const juce::String& tip);
    void loadFactoryPresetByIdx(int idx);
    void openSavePresetDialog();
    void openLoadPresetDialog();

    SlipperyFairyAudioProcessor& processorRef;
    LookAndFeelSketchbook laf;
    SlipOscilloscopeView  scopeView;
    HysteresisLoopView    loopView;
    SlipperyFairyMascot   mascotView;

    // Header: Preset Browser + Oversampling Selector
    juce::TextButton prevPresetBtn { "<" };
    juce::ComboBox   presetBox;
    juce::TextButton nextPresetBtn { ">" };
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton loadPresetBtn { "LOAD" };
    juce::ComboBox   osBox;

    // Card 1: Velocity Topology & Speed
    juce::ComboBox slipModeBox;
    juce::Slider   driveKnob, slewFreqKnob;

    // Card 2: Asymmetry & Harmonic Bloom
    juce::Slider asymmetryKnob, dynBiasKnob, airRecKnob;

    // Card 3: Monitor & Output
    juce::ComboBox     auditionBox;
    juce::Slider       mixKnob, outGainKnob;
    juce::ToggleButton autoGainBtn { "AUTO GAIN" };
    juce::ToggleButton bypassBtn   { "BYPASS" };

    // Bottom 1-Line Hover Guide
    juce::String hoverTipText { "Hover over any control for a quick guide, or pick a factory preset above." };
    int currentFactoryPresetIdx = 0;
    std::unique_ptr<juce::FileChooser> fileChooser;

    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<CAtt> attSlipMode, attOversampling, attAudition;
    std::unique_ptr<SAtt> attDrive, attSlewFreq, attAsymmetry, attDynBias, attAirRec, attMix, attOutGain;
    std::unique_ptr<BAtt> attAutoGain, attBypass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlipperyFairyAudioProcessorEditor)
};
