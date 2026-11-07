#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
class JunoSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Polynomial band-limiting correction for discontinuous oscillator waveforms.
// This reduces the most obvious aliasing without adding per-voice oversampling.
float polyBlep (double phase, double increment) noexcept
{
    if (increment <= 0.0)
        return 0.0f;

    if (phase < increment)
    {
        const double x = phase / increment;
        return static_cast<float> (x + x - x * x - 1.0);
    }

    if (phase > 1.0 - increment)
    {
        const double x = (phase - 1.0) / increment;
        return static_cast<float> (x * x + x + x + 1.0);
    }

    return 0.0f;
}

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
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
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
        filterEnvAmount = envAmount;
        // JUCE ADSR parameters must not be changed while a note is active.
        // Apply the latest values at the next note-on instead.
        ampParameters = { attack, decay, sustain, release };
        filter.setResonance (0.70710678f + resonance * 4.0f);
    }

    void startNote (int note, float velocity, juce::SynthesiserSound*, int) override
    {
        frequency = juce::MidiMessage::getMidiNoteInHertz (note);
        noteVelocity = velocity;
        phase = 0.0;
        subPhase = 0.0;
        ampEnvelope.reset();
        ampEnvelope.setParameters (ampParameters);
        ampEnvelope.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            ampEnvelope.noteOff();
        else
        {
            ampEnvelope.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int start, int count) override
    {
        const double sr = getSampleRate();
        if (frequency <= 0.0 || sr <= 0.0)
            return;

        const double phaseIncrement = frequency / sr;
        const double subIncrement = phaseIncrement * 0.5;

        for (int i = 0; i < count; ++i)
        {
            if (! ampEnvelope.isActive())
            {
                clearCurrentNote();
                break;
            }

            const float env = ampEnvelope.getNextSample();
            const float saw = static_cast<float> (2.0 * phase - 1.0)
                            - polyBlep (phase, phaseIncrement);

            float pulse = phase < pulseWidth ? 1.0f : -1.0f;
            pulse += polyBlep (phase, phaseIncrement);
            double fallingEdge = phase - pulseWidth;
            if (fallingEdge < 0.0)
                fallingEdge += 1.0;
            pulse -= polyBlep (fallingEdge, phaseIncrement);

            float sub = subPhase < 0.5 ? 1.0f : -1.0f;
            sub += polyBlep (subPhase, subIncrement);
            double subFallingEdge = subPhase - 0.5;
            if (subFallingEdge < 0.0)
                subFallingEdge += 1.0;
            sub -= polyBlep (subFallingEdge, subIncrement);

            const float noise = random.nextFloat() * 2.0f - 1.0f;
            float source = saw * sawLevel + pulse * pulseLevel
                         + sub * subLevel + noise * noiseLevel;
            source *= env * noteVelocity * 0.22f;

            const float modulatedCutoff = juce::jlimit (
                30.0f, static_cast<float> (sr * 0.44),
                baseCutoff * std::pow (2.0f, env * filterEnvAmount * 4.0f));
            filter.setCutoffFrequency (modulatedCutoff);
            const float filtered = filter.processSample (0, source);

            phase += phaseIncrement;
            if (phase >= 1.0)
                phase -= 1.0;

            subPhase += subIncrement;
            if (subPhase >= 1.0)
                subPhase -= 1.0;

            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, start + i, filtered);
        }
    }

private:
    double frequency = 440.0;
    double phase = 0.0;
    double subPhase = 0.0;
    float noteVelocity = 0.8f;
    float sawLevel = 0.8f, pulseLevel = 0.0f, subLevel = 0.0f, noiseLevel = 0.0f;
    float pulseWidth = 0.5f, baseCutoff = 12000.0f, filterEnvAmount = 0.0f;
    juce::ADSR ampEnvelope;
    juce::ADSR::Parameters ampParameters { 0.01f, 0.8f, 0.8f, 0.25f };
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Random random;
};
}

Juno60AudioProcessor::Juno60AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
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
