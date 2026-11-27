#include "PluginEditor.h"

SlipperyFairyAudioProcessorEditor::SlipperyFairyAudioProcessorEditor(SlipperyFairyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), scopeView(p)
{
    setLookAndFeel(&laf);
    setSize(980, 624);

    addAndMakeVisible(scopeView);
    addAndMakeVisible(loopView);
    addAndMakeVisible(mascotView);

    // Header: Factory Preset Browser + Save/Load + Oversampling
    addAndMakeVisible(prevPresetBtn);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(nextPresetBtn);
    addAndMakeVisible(savePresetBtn);
    addAndMakeVisible(loadPresetBtn);

    const auto& factories = SlipperyPresetManager::getFactoryPresets();
    for (int i = 0; i < static_cast<int>(factories.size()); ++i)
        presetBox.addItem(factories[static_cast<size_t>(i)].name, i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);

    presetBox.onChange = [this]()
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0)
            loadFactoryPresetByIdx(idx);
    };
    prevPresetBtn.onClick = [this]()
    {
        const int n = static_cast<int>(SlipperyPresetManager::getFactoryPresets().size());
        loadFactoryPresetByIdx((currentFactoryPresetIdx - 1 + n) % n);
    };
    nextPresetBtn.onClick = [this]()
    {
        const int n = static_cast<int>(SlipperyPresetManager::getFactoryPresets().size());
        loadFactoryPresetByIdx((currentFactoryPresetIdx + 1) % n);
    };
    savePresetBtn.onClick = [this]() { openSavePresetDialog(); };
    loadPresetBtn.onClick = [this]() { openLoadPresetDialog(); };

    osBox.addItemList({ "OS: OFF (1x)", "OS: 2x FIR", "OS: 4x FIR", "OS: 8x FIR", "OS: 16x FIR" }, 1);
    addAndMakeVisible(osBox);

    // Card 1: Velocity Topology & Speed
    slipModeBox.addItemList({
        "OP-AMP SID (Asym Slew)",
        "ASYM RC (Charge/Fall)",
        "VEL QUAD (Pure 2nd)",
        "DYN SLIP (Slew+Bias)"
    }, 1);
    addAndMakeVisible(slipModeBox);

    setupKnob(driveKnob, " dB", 0xffc0842c, false,
              "INPUT DRIVE: Pushes signal velocity deeper into slew-rate limiting (Auto Gain keeps level steady).");
    setupKnob(slewFreqKnob, " Hz", 0xff2f6f99, false,
              "SLEW CEILING: Maximum clean waveform speed. Lower Hz = earlier transient & high-frequency slipping.");

    // Card 2: Asymmetry & Harmonic Bloom
    setupKnob(asymmetryKnob, " %", 0xff2f8578, true,
              "RISE / FALL ASYM: -100% Slow Rise | 0% Symmetric Odd | +100% Slow Fall (silky 2nd harmonics!).");
    setupKnob(dynBiasKnob, " %", 0xff6d5a9e, false,
              "DYNAMIC BIAS: Envelope-modulated bias shift that adds warm 2nd-harmonic bloom only on loud peaks.");
    setupKnob(airRecKnob, " dB", 0xff2f6f99, true,
              "AIR RECOVERY: Post-slew 4.5 kHz shelf to restore silky high-end air after heavy transient rounding.");

    // Card 3: Monitor & Output
    auditionBox.addItemList({ "NORMAL", "DELTA (HARMONICS)" }, 1);
    addAndMakeVisible(auditionBox);

    setupKnob(mixKnob, " %", 0xff2f6f99, false,
              "PARALLEL MIX: Phase-aligned blend between dry input and Slippery Fairy output.");
    setupKnob(outGainKnob, " dB", 0xff2f6f99, true,
              "OUTPUT GAIN: Final makeup level trim.");
    addAndMakeVisible(autoGainBtn);
    addAndMakeVisible(bypassBtn);

    // Register friendly 1-line hover tips
    registerTip(presetBox,     "FACTORY PRESETS: 12 curated starting points for mastering, vocals, drums, synths & bass.");
    registerTip(prevPresetBtn, "Previous factory preset.");
    registerTip(nextPresetBtn, "Next factory preset.");
    registerTip(savePresetBtn, "Save current settings as a .xml user preset.");
    registerTip(loadPresetBtn, "Load a saved .xml user preset from disk.");
    registerTip(osBox,         "OVERSAMPLING: Linear-phase equiripple anti-aliasing FIR (OFF / 2x / 4x / 8x / 16x). 8x recommended.");

    registerTip(slipModeBox,   "OP-AMP SID: level+freq slew | ASYM RC: +6dB/oct 2nd | VEL QUAD: pure x*dx/dt 2nd | DYN SLIP: slew+bias.");
    registerTip(auditionBox,   "NORMAL: Full output | DELTA: Solo exact generated even/odd harmonics & slew rounding.");
    registerTip(autoGainBtn,   "AUTO GAIN: Matches output loudness automatically as you sweep Input Drive.");
    registerTip(bypassBtn,     "BYPASS: Exact latency-compensated A/B bypass with zero timing shift.");
    registerTip(scopeView,     "SLIP SCOPE: Ochre = input waveform; Cerulean = velocity-slipped asymmetric output.");
    registerTip(loopView,      "HYSTERESIS LOOP: Static saturators draw a thin line; velocity slew opens a living asymmetric loop!");
    registerTip(mascotView,    "Slippery Fairy (Sylredhel): Leans & skids in the direction of your Rise/Fall Asymmetry!");

    auto& a = processorRef.apvts;
    attSlipMode     = std::make_unique<CAtt>(a, "slipMode",     slipModeBox);
    attOversampling = std::make_unique<CAtt>(a, "oversampling", osBox);
    attDrive        = std::make_unique<SAtt>(a, "driveDb",      driveKnob);
    attSlewFreq     = std::make_unique<SAtt>(a, "slewFreq",     slewFreqKnob);

    attAsymmetry    = std::make_unique<SAtt>(a, "asymmetryPct", asymmetryKnob);
    attDynBias      = std::make_unique<SAtt>(a, "dynBiasPct",   dynBiasKnob);
    attAirRec       = std::make_unique<SAtt>(a, "airRecDb",     airRecKnob);

    attAudition     = std::make_unique<CAtt>(a, "audition",     auditionBox);
    attMix          = std::make_unique<SAtt>(a, "mixPct",       mixKnob);
    attOutGain      = std::make_unique<SAtt>(a, "outGainDb",    outGainKnob);
    attAutoGain     = std::make_unique<BAtt>(a, "autoGain",     autoGainBtn);
    attBypass       = std::make_unique<BAtt>(a, "bypass",       bypassBtn);

    addMouseListener(this, true);
    startTimerHz(30);
}

SlipperyFairyAudioProcessorEditor::~SlipperyFairyAudioProcessorEditor()
{
    stopTimer();
    removeMouseListener(this);
    setLookAndFeel(nullptr);
}

void SlipperyFairyAudioProcessorEditor::setupKnob(juce::Slider& s, const juce::String& suffix,
                                                  uint32_t accentHex, bool bipolar, const juce::String& tip)
{
    using namespace sylredhel::sketch;
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 16);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::textBoxTextColourId,       kGraphite);
    s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxBackgroundColourId, kPaperBot.withAlpha(0.65f));
    s.getProperties().set(LookAndFeelSketchbook::kAccentProp,  static_cast<int>(accentHex));
    s.getProperties().set(LookAndFeelSketchbook::kBipolarProp, bipolar);
    s.setRotaryParameters(juce::degreesToRadians(220.0f), juce::degreesToRadians(500.0f), true);
    addAndMakeVisible(s);
    registerTip(s, tip);
}

void SlipperyFairyAudioProcessorEditor::registerTip(juce::Component& c, const juce::String& tip)
{
    c.getProperties().set("hoverTip", tip);
}

void SlipperyFairyAudioProcessorEditor::mouseEnter(const juce::MouseEvent& e)
{
    auto* comp = e.eventComponent;
    while (comp != nullptr && comp != this)
    {
        if (comp->getProperties().contains("hoverTip"))
        {
            hoverTipText = comp->getProperties()["hoverTip"].toString();
            repaint(0, getHeight() - 32, getWidth(), 32);
            return;
        }
        comp = comp->getParentComponent();
    }
}

void SlipperyFairyAudioProcessorEditor::mouseExit(const juce::MouseEvent&)
{
    hoverTipText = "Hover over any control for a quick guide  |  Tip: Sweep Rise/Fall Asymmetry while watching the Hysteresis Loop!";
    repaint(0, getHeight() - 32, getWidth(), 32);
}

void SlipperyFairyAudioProcessorEditor::loadFactoryPresetByIdx(int idx)
{
    const auto& list = SlipperyPresetManager::getFactoryPresets();
    if (idx < 0 || idx >= static_cast<int>(list.size()))
        return;

    currentFactoryPresetIdx = idx;
    presetBox.setSelectedId(idx + 1, juce::dontSendNotification);
    SlipperyPresetManager::applyFactoryPreset(processorRef.apvts, idx);
    hoverTipText = "Preset: " + list[static_cast<size_t>(idx)].tip;
    repaint(0, getHeight() - 32, getWidth(), 32);
}

void SlipperyFairyAudioProcessorEditor::openSavePresetDialog()
{
    auto startDir = SlipperyPresetManager::getUserPresetDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Slippery Fairy Preset", startDir.getChildFile("MySlipperyPreset.xml"), "*.xml");

    constexpr int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f != juce::File {})
        {
            auto target = f.hasFileExtension(".xml") ? f : f.withFileExtension(".xml");
            if (SlipperyPresetManager::saveUserPreset(processorRef.apvts, target))
            {
                hoverTipText = "Saved preset: " + target.getFileName();
                repaint(0, getHeight() - 32, getWidth(), 32);
            }
        }
    });
}

void SlipperyFairyAudioProcessorEditor::openLoadPresetDialog()
{
    auto startDir = SlipperyPresetManager::getUserPresetDirectory();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Slippery Fairy Preset", startDir, "*.xml");

    constexpr int flags = juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile())
        {
            if (SlipperyPresetManager::loadUserPreset(processorRef.apvts, f))
            {
                presetBox.setText(f.getFileNameWithoutExtension(), juce::dontSendNotification);
                hoverTipText = "Loaded user preset: " + f.getFileName();
                repaint(0, getHeight() - 32, getWidth(), 32);
            }
        }
    });
}

void SlipperyFairyAudioProcessorEditor::timerCallback()
{
    scopeView.updateFrame(slipModeBox.getSelectedItemIndex(), osBox.getSelectedItemIndex());
    loopView.updateFromTraces(scopeView.getInTrace(), scopeView.getOutTrace());
    mascotView.updateActivity(processorRef.getSlipActivity(),
                              static_cast<float>(asymmetryKnob.getValue()));
}

void SlipperyFairyAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace sylredhel::sketch;
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    // 1. Warm Cream Sketchbook Paper Backdrop
    juce::ColourGradient paper(kPaperTop, 0.0f, 0.0f, kPaperBot, 0.0f, h, false);
    g.setGradientFill(paper);
    g.fillAll();

    // Subtle sketchbook ruled lines
    g.setColour(kGridLine.withAlpha(0.35f));
    for (float y = 58.0f; y < h - 30.0f; y += 26.0f)
        g.drawHorizontalLine(static_cast<int>(y), 24.0f, w - 12.0f);

    // Sketchbook spiral binding holes along left margin
    g.setColour(kGraphiteMid.withAlpha(0.20f));
    g.drawVerticalLine(20, 0.0f, h);
    for (float y = 26.0f; y < h - 18.0f; y += 34.0f)
    {
        g.setColour(kGraphite.withAlpha(0.18f));
        g.fillEllipse(6.0f, y, 8.0f, 8.0f);
        g.setColour(kGraphiteMid.withAlpha(0.45f));
        g.drawEllipse(6.0f, y, 8.0f, 8.0f, 1.0f);
        g.drawLine(0.0f, y + 4.0f, 9.0f, y + 4.0f, 1.4f);
    }

    // 2. Top Sketchbook Header Strip
    g.setColour(kCardFill.withAlpha(0.85f));
    g.fillRect(21.0f, 0.0f, w - 21.0f, 52.0f);
    g.setColour(kGraphiteMid.withAlpha(0.55f));
    g.drawHorizontalLine(52, 21.0f, w);
    g.setColour(kGraphiteLt.withAlpha(0.30f));
    g.drawHorizontalLine(54, 21.0f, w);

    // Hand-drawn little watercolor droplet emblem
    juce::Path drop;
    drop.startNewSubPath(42.0f, 11.0f);
    drop.quadraticTo(53.0f, 25.0f, 42.0f, 38.0f);
    drop.quadraticTo(31.0f, 25.0f, 42.0f, 11.0f);
    drop.closeSubPath();
    g.setColour(kCerulean.withAlpha(0.24f));
    g.fillPath(drop);
    g.setColour(kGraphite);
    g.strokePath(drop, juce::PathStrokeType(1.5f));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(39.0f, 21.0f, 38.0f, 27.0f, 1.4f);

    g.setColour(kGraphiteMid);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::italic)));
    g.drawText("Sylredhel", 64, 7, 150, 14, juce::Justification::centredLeft);

    g.setColour(kGraphite);
    g.setFont(juce::Font(juce::FontOptions(19.5f, juce::Font::bold)));
    g.drawText("SLIPPERY FAIRY", 63, 21, 195, 24, juce::Justification::centredLeft);

    g.setColour(kCerulean);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("v2", 229, 26, 26, 16, juce::Justification::centredLeft);

    // 3. Three Sketchbook Control Cards
    auto drawCard = [&g](juce::Rectangle<float> r, const juce::String& title, juce::Colour accent)
    {
        g.setColour(kCardFill.withAlpha(0.92f));
        g.fillRoundedRectangle(r, 6.0f);
        drawSketchBox(g, r, 6.0f, kGraphiteMid);

        g.setColour(accent.withAlpha(0.30f));
        g.fillRoundedRectangle(r.getX() + 12.0f, r.getY() + 21.0f, 142.0f, 5.0f, 2.5f);

        g.setColour(kGraphite);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(title, static_cast<int>(r.getX() + 14), static_cast<int>(r.getY() + 8),
                   static_cast<int>(r.getWidth() - 28), 16, juce::Justification::centredLeft);
    };

    drawCard({ 32.0f,  284.0f, 276.0f, 296.0f }, "1. VELOCITY & SPEED",     kCerulean);
    drawCard({ 320.0f, 284.0f, 404.0f, 296.0f }, "2. ASYMMETRY & BLOOM",    kSageTeal);
    drawCard({ 736.0f, 284.0f, 228.0f, 296.0f }, "3. MONITOR & OUTPUT",     kOchre);

    auto lbl = [&g](const juce::String& txt, int x, int y, int width, juce::Colour col = kGraphiteMid)
    {
        g.setColour(col);
        g.setFont(juce::Font(juce::FontOptions(9.2f, juce::Font::bold)));
        g.drawText(txt, x, y, width, 13, juce::Justification::centred);
    };

    // Card 1 Labels
    lbl("SLIP TOPOLOGY",  50, 318, 240, kCerulean);
    lbl("INPUT DRIVE",    52, 398, 110, kOchre.darker(0.20f));
    lbl("SLEW CEILING",  174, 398, 114, kCerulean);

    g.setColour(kGraphiteMid);
    g.setFont(juce::Font(juce::FontOptions(8.7f, juce::Font::italic)));
    g.drawFittedText("Lower Slew Ceiling = earlier high-freq & transient slipping.",
                     44, 534, 252, 30, juce::Justification::centred, 2);

    // Card 2 Labels
    lbl("RISE / FALL ASYM", 340, 344, 114, kSageTeal.darker(0.15f));
    lbl("DYNAMIC BIAS",     464, 344, 114, kLavender);
    lbl("AIR RECOVERY",     588, 344, 114, kCerulean);

    g.setColour(kGraphiteMid);
    g.setFont(juce::Font(juce::FontOptions(8.7f, juce::Font::italic)));
    g.drawFittedText("-100% Slow Rise   <   0% Odd Sym   >   +100% Slow Fall (Even 2nd)\nOP-AMP SID: level+freq  |  ASYM RC: +6dB/oct  |  VEL QUAD: pure x*dx/dt 2nd",
                     334, 502, 376, 56, juce::Justification::centred, 3);

    // Card 3 Labels
    lbl("AUDITION",     754, 318, 192, kOchre.darker(0.20f));
    lbl("PARALLEL MIX", 750, 380,  96);
    lbl("OUTPUT GAIN",  852, 380,  96);

    // 4. Bottom 1-Line Hover Guide Bar
    const auto bar = juce::Rectangle<float>(32.0f, h - 32.0f, w - 48.0f, 24.0f);
    g.setColour(kCardFill.withAlpha(0.90f));
    g.fillRoundedRectangle(bar, 4.0f);
    drawSketchBox(g, bar, 4.0f, kGraphiteLt);

    g.setColour(kCerulean);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("GUIDE:", static_cast<int>(bar.getX() + 10), static_cast<int>(bar.getY()), 48, 24,
               juce::Justification::centredLeft);

    g.setColour(kGraphite);
    g.setFont(juce::Font(juce::FontOptions(10.2f, juce::Font::plain)));
    g.drawText(hoverTipText, static_cast<int>(bar.getX() + 58), static_cast<int>(bar.getY()),
               static_cast<int>(bar.getWidth() - 68), 24, juce::Justification::centredLeft, true);
}

void SlipperyFairyAudioProcessorEditor::resized()
{
    // Header: Preset Bar + Oversampling Selector
    prevPresetBtn.setBounds(274, 13,  26, 26);
    presetBox.setBounds(304,     13, 250, 26);
    nextPresetBtn.setBounds(558, 13,  26, 26);
    savePresetBtn.setBounds(592, 13,  56, 26);
    loadPresetBtn.setBounds(652, 13,  56, 26);
    osBox.setBounds(818,         13, 146, 26);

    // Upper Visual Row: Slip Scope + Hysteresis Loop + Slippery Fairy Mascot
    scopeView.setBounds(32,   64, 514, 208);
    loopView.setBounds(558,   64, 224, 208);
    mascotView.setBounds(794, 64, 170, 208);

    // Card 1: Velocity & Speed
    slipModeBox.setBounds(50,   336, 240, 28);
    driveKnob.setBounds(56,     416, 102, 110);
    slewFreqKnob.setBounds(178, 416, 102, 110);

    // Card 2: Asymmetry & Harmonic Bloom
    asymmetryKnob.setBounds(342, 366, 110, 120);
    dynBiasKnob.setBounds(466,   366, 110, 120);
    airRecKnob.setBounds(590,    366, 110, 120);

    // Card 3: Monitor & Output
    auditionBox.setBounds(754, 334, 192, 28);
    mixKnob.setBounds(752,     398,  92, 100);
    outGainKnob.setBounds(854, 398,  92, 100);
    autoGainBtn.setBounds(752, 518,  94,  34);
    bypassBtn.setBounds(852,   518,  94,  34);
}
