#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Juno60AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit Juno60AudioProcessorEditor (Juno60AudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    Juno60AudioProcessor& audioProcessor;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<Attachment>> attachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Juno60AudioProcessorEditor)
};
