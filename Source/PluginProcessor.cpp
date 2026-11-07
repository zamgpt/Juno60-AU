#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
class JunoSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class JunoVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<JunoSound*> (sound) != nullptr;
    }

    void prepare (double sr)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 2048;
        spec.numChannels = 1;
        filter.prepare (spec);
        filter.reset();
        ampEnvelope.setSampleRate (sr);
    }

    void setParameters (float saw, float pulse, float sub, float noise,
                        float pw, float cutoff, float resonance, float envAmount,
                        float attack, float decay, float sustain, float release)
    {
        sawLevel = saw;
        pulseLevel = pulse;
        subLevel = sub;
        noiseLevel = noise;
        pulseWidth = pw;
        baseCutoff = cutoff;
        filterResonance = resonance;
        filterEnvAmount = envAmount;
        ampEnvelope.setParameters ({ attack, decay, sustain, release });
        filter.setResonance (juce::jlimit (0.1f, 1.0f, 0.1f + resonance * 0.88f));
    }

    void startNote (int note, float velocity, juce::SynthesiserSound*, int) override
    {
        frequency = juce::MidiMessage::getMidiNoteInHertz (note);
        noteVelocity = velocity;
        phase = 0.0;
        ampEnvelope.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        ampEnvelope.noteOff();
        if (! allowTailOff)
            clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int start, int count) override
    {
        if (frequency <= 0.0 || getSampleRate() <= 0.0)
            return;

        const double phaseIncrement = frequency / getSampleRate();
        for (int i = 0; i < count; ++i)
        {
            if (! ampEnvelope.isActive())
            {
                clearCurrentNote();
                break;
            }

            const float env = ampEnvelope.getNextSample();
            const float saw = 2.0f * static_cast<float> (phase) - 1.0f;
            const float pulse = phase < pulseWidth ? 1.0f : -1.0f;
            const float sub = phase < 0.5 ? 1.0f : -1.0f;
            const float noise = random.nextFloat() * 2.0f - 1.0f;

            float source = saw * sawLevel + pulse * pulseLevel
                         + sub * subLevel + noise * noiseLevel;
            source *= env * noteVelocity * 0.22f;

            const float modulatedCutoff = juce::jlimit (
                30.0f, static_cast<float> (getSampleRate() * 0.44),
                baseCutoff * std::pow (2.0f, env * filterEnvAmount * 4.0f));
            filter.setCutoffFrequency (modulatedCutoff);
            const float filtered = filter.processSample (0, source);

            phase += phaseIncrement;
            if (phase >= 1.0)
                phase -= 1.0;

            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, start + i, filtered);
        }
    }

private:
    double frequency = 440.0;
    double phase = 0.0;
    float noteVelocity = 0.8f;
    float sawLevel = 0.8f, pulseLevel = 0.0f, subLevel = 0.0f, noiseLevel = 0.0f;
    float pulseWidth = 0.5f, baseCutoff = 12000.0f, filterResonance = 0.1f, filterEnvAmount = 0.0f;
    juce::ADSR ampEnvelope;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Random random;
};
}

Juno60AudioProcessor::Juno60AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // The original Juno-60 is a six-voice instrument.
    for (int i = 0; i < 6; ++i)
        synth.addVoice (new JunoVoice());
    synth.addSound (new JunoSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout Juno60AudioProcessor::createParameterLayout()
{
    using P = juce::AudioProcessorValueTreeState;
    P::ParameterLayout p;
    p.add (std::make_unique<juce::AudioParameterFloat> ("SAW", "Saw", 0.0f, 1.0f, 0.8f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("PULSE", "Pulse", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("SUB", "Sub", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("NOISE", "Noise", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("PW", "Pulse Width", 0.05f, 0.95f, 0.5f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("CUTOFF", "VCF Cutoff",
        juce::NormalisableRange<float> (30.0f, 18000.0f, 0.0f, 0.25f), 12000.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("RESONANCE", "VCF Resonance", 0.0f, 1.0f, 0.12f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("ENV_MOD", "VCF Env Amount", 0.0f, 1.0f, 0.25f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("ATTACK", "Attack", 0.001f, 5.0f, 0.01f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("DECAY", "Decay", 0.01f, 8.0f, 0.8f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain", 0.0f, 1.0f, 0.8f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release", 0.01f, 8.0f, 0.25f));
    p.add (std::make_unique<juce::AudioParameterFloat> ("CHORUS", "Chorus", 0.0f, 1.0f, 0.0f));
    return p;
}

void Juno60AudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    synth.setCurrentPlaybackSampleRate (sr);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<JunoVoice*> (synth.getVoice (i)))
            voice->prepare (sr);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 2;
    chorus.prepare (spec);
    chorus.reset();
    chorus.setRate (0.25f);
    chorus.setDepth (0.35f);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);
}

bool Juno60AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void Juno60AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto value = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<JunoVoice*> (synth.getVoice (i)))
            voice->setParameters (value ("SAW"), value ("PULSE"), value ("SUB"), value ("NOISE"),
                value ("PW"), value ("CUTOFF"), value ("RESONANCE"), value ("ENV_MOD"),
                value ("ATTACK"), value ("DECAY"), value ("SUSTAIN"), value ("RELEASE"));

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() == 2)
    {
        chorus.setMix (value ("CHORUS"));
        juce::dsp::AudioBlock<float> block (buffer);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
}

juce::AudioProcessorEditor* Juno60AudioProcessor::createEditor()
{
    return new Juno60AudioProcessorEditor (*this);
}

void Juno60AudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void Juno60AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Juno60AudioProcessor();
}
