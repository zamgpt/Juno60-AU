#include "PluginEditor.h"

Juno60AudioProcessorEditor::Juno60AudioProcessorEditor (Juno60AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1080, 570);
    const std::array<std::pair<const char*, const char*>, 13> defs {{
        { "SAW", "SAW" }, { "PULSE", "PULSE" }, { "SUB", "SUB" }, { "NOISE", "NOISE" },
        { "PW", "PULSE WIDTH" }, { "CUTOFF", "VCF CUTOFF" }, { "RESONANCE", "RESONANCE" },
        { "ENV_MOD", "ENV MOD" }, { "ATTACK", "ATTACK" }, { "DECAY", "DECAY" },
        { "SUSTAIN", "SUSTAIN" }, { "RELEASE", "RELEASE" }, { "CHORUS", "CHORUS" }
    }};

    for (const auto& item : defs)
    {
        auto* slider = sliders.add (new juce::Slider());
        slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
        slider->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff287eaa));
        slider->setColour (juce::Slider::thumbColourId, juce::Colour (0xff20242a));
        addAndMakeVisible (slider);

        auto* label = labels.add (new juce::Label());
        label->setText (item.second, juce::dontSendNotification);
        label->setJustificationType (juce::Justification::centred);
        label->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        addAndMakeVisible (label);
        attachments.push_back (std::make_unique<Attachment> (processor.parameters, item.first, *slider));
    }
}

void Juno60AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffcbd1d7));
    g.setColour (juce::Colour (0xffe9ecef));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f), 8.0f);
    g.setColour (juce::Colour (0xff24384a));
    g.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold)));
    g.drawText ("JUNO-60", 28, 18, 260, 44, juce::Justification::left);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("SIX-VOICE PROGRAMMABLE POLYPHONIC SYNTHESIZER  |  DEVELOPMENT BUILD",
                300, 28, 700, 26, juce::Justification::right);
    g.setColour (juce::Colour (0xff287eaa));
    g.fillRect (10, 76, getWidth() - 20, 5);
    g.setColour (juce::Colour (0xff66727e));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("DCO", 28, 98, 330, 20, juce::Justification::left);
    g.drawText ("VCF / VCA", 390, 98, 330, 20, juce::Justification::left);
    g.drawText ("ENVELOPE / CHORUS", 752, 98, 300, 20, juce::Justification::left);
}

void Juno60AudioProcessorEditor::resized()
{
    const int x0 = 20, y = 145, sliderW = 78, sliderH = 175, gap = 4;
    for (int i = 0; i < sliders.size(); ++i)
    {
        const int x = x0 + i * (sliderW + gap);
        sliders[i]->setBounds (x, y, sliderW, sliderH);
        labels[i]->setBounds (x - 2, y - 28, sliderW + 4, 24);
    }
}
