#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct SlipperyPreset
{
    juce::String name;
    juce::String tip;
    int   slipMode;      // 0=OP-AMP SID, 1=ASYM RC, 2=VEL QUAD, 3=DYN SLIP
    int   oversampling;  // 0=OFF, 1=2x, 2=4x, 3=8x, 4=16x
    float driveDb;
    float slewFreq;
    float asymmetryPct;  // -100 .. +100
    float dynBiasPct;    // 0 .. 100
    float airRecDb;      // -6 .. +12
    float mixPct;
    float outGainDb;
    bool  autoGain;
};

class SlipperyPresetManager
{
public:
    static const std::vector<SlipperyPreset>& getFactoryPresets()
    {
        static const std::vector<SlipperyPreset> presets = {
            { "01. Init (Silky Op-Amp)",
              "Balanced starting point: gentle asymmetric Op-Amp slew with auto-gain.",
              0, 3,  6.0f, 3200.0f,  65.0f,  0.0f,  0.0f, 100.0f, 0.0f, true  },

            { "02. Master - Subtle 2nd Sheen",
              "Pure velocity quadrature (x*dx/dt): 3rd-free 2nd harmonic gloss on upper mids & highs.",
              2, 3,  4.0f, 4500.0f,  45.0f,  0.0f,  0.0f, 100.0f, 0.0f, true  },

            { "03. Master - Console Warmth",
              "Hybrid DYN SLIP with gentle dynamic bias & high slew ceiling for mix cohesion.",
              3, 3,  4.5f, 5800.0f,  55.0f, 25.0f,  0.8f, 100.0f, 0.0f, true  },

            { "04. Vocal - De-Harsh Silk",
              "Rounds sharp vocal sibilance & condenser harshness naturally, then restores airy gloss.",
              0, 3,  7.5f, 2600.0f,  75.0f, 10.0f,  2.2f, 100.0f, 0.0f, true  },

            { "05. Vocal - Warm Tube Bloom",
              "Dynamic grid-bias bloom: stays clean on whispers and glows with 2nd harmonics on belts.",
              3, 3,  8.0f, 3800.0f,  60.0f, 55.0f,  1.2f, 100.0f, 0.0f, true  },

            { "06. Drum Bus - Analog Glue",
              "Asymmetric RC charge/fall drag softens brittle drum edges while keeping kick punch tight.",
              1, 3,  6.0f, 6200.0f,  70.0f, 15.0f,  1.5f, 100.0f, 0.0f, true  },

            { "07. Cymbals / OH - Ice Polish",
              "Classic Op-Amp SID rounding of harsh digital cymbal splash + silky air recovery.",
              0, 3,  9.0f, 2100.0f,  80.0f,  0.0f,  3.5f, 100.0f, 0.0f, true  },

            { "08. Acoustic - Pick Softener",
              "Gently tames spiky acoustic guitar or pluck attack spikes without dulling the strings.",
              0, 3,  6.5f, 2900.0f, -55.0f,  0.0f,  1.5f,  85.0f, 0.0f, true  },

            { "09. Synth Lead - Liquid Saw",
              "Heavy asymmetric slew tilts edgy digital saw waves into velvety analog curves.",
              0, 4, 13.0f, 1450.0f,  90.0f, 35.0f,  2.0f, 100.0f, 0.0f, true  },

            { "10. Bass - Warm Even Drive",
              "Deep DYN SLIP + dynamic bias for lush, glowing 2nd-harmonic bass weight.",
              3, 3, 10.5f, 2200.0f,  70.0f, 75.0f,  0.0f, 100.0f, 0.0f, true  },

            { "11. Piano / Keys - Velvet Hammer",
              "Reverse asymmetry (-65% slow rise / fast fall) cushions sharp hammer strikes smoothly.",
              1, 3,  5.5f, 5200.0f, -65.0f, 20.0f,  1.0f,  90.0f, 0.0f, true  },

            { "12. Parallel - Pure 2nd Sparkle",
              "Hard-driven VEL QUAD blended at 35% parallel for an ultra-pure even-harmonic exciter.",
              2, 4, 12.0f, 2800.0f,  85.0f,  0.0f,  1.5f,  35.0f, 0.0f, true  }
        };
        return presets;
    }

    static void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
    {
        const auto& list = getFactoryPresets();
        if (index < 0 || index >= static_cast<int>(list.size()))
            return;

        const auto& p = list[static_cast<size_t>(index)];
        auto setParam = [&](const juce::String& id, float plainVal)
        {
            if (auto* param = apvts.getParameter(id))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(plainVal));
                param->endChangeGesture();
            }
        };

        setParam("slipMode",     static_cast<float>(p.slipMode));
        setParam("oversampling", static_cast<float>(p.oversampling));
        setParam("driveDb",      p.driveDb);
        setParam("slewFreq",     p.slewFreq);
        setParam("asymmetryPct", p.asymmetryPct);
        setParam("dynBiasPct",   p.dynBiasPct);
        setParam("airRecDb",     p.airRecDb);
        setParam("mixPct",       p.mixPct);
        setParam("outGainDb",    p.outGainDb);
        setParam("autoGain",     p.autoGain ? 1.0f : 0.0f);
        setParam("audition",     0.0f);
        setParam("bypass",       0.0f);
    }

    static juce::File getUserPresetDirectory()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("Sylredhel")
                       .getChildFile("Slippery Fairy")
                       .getChildFile("Presets");
        if (!dir.exists())
            dir.createDirectory();
        return dir;
    }

    static bool saveUserPreset(juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml == nullptr)
            return false;
        return xml->writeTo(file);
    }

    static bool loadUserPreset(juce::AudioProcessorValueTreeState& apvts, const juce::File& file)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            return true;
        }
        return false;
    }
};
