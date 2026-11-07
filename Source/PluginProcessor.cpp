#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
class JunoSound final : public juce::SynthesiserSound {
public:
 bool appliesToNote(int) override { return true; }
 bool appliesToChannel(int) override { return true; }
};
class JunoVoice final : public juce::SynthesiserVoice {
public:
 bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<JunoSound*>(s) != nullptr; }
 void startNote(int note, float vel, juce::SynthesiserSound*, int) override {
   frequency = juce::MidiMessage::getMidiNoteInHertz(note); velocity = vel;
   phase = 0.0; env = 0.0f; releasing = false;
 }
 void stopNote(float, bool allowTail) override { releasing = true; if (!allowTail) clearCurrentNote(); }
 void pitchWheelMoved(int) override {}
 void controllerMoved(int, int) override {}
 void renderNextBlock(juce::AudioBuffer<float>& out, int start, int count) override {
   if (frequency <= 0.0) return;
   const double inc = frequency / getSampleRate();
   for (int i=0;i<count;++i) {
     env = releasing ? env * releaseCoef : 1.0f - (1.0f-env)*attackCoef;
     if (releasing && env < 0.0001f) { clearCurrentNote(); break; }
     float saw = 2.0f * static_cast<float>(phase) - 1.0f;
     float pulse = phase < pulseWidth ? 1.0f : -1.0f;
     float sub = phase < 0.5 ? 1.0f : -1.0f;
     float x = (saw * sawLevel + pulse * pulseLevel + sub * subLevel) * env * velocity * 0.18f;
     phase += inc; if (phase >= 1.0) phase -= 1.0;
     for (int ch=0; ch<out.getNumChannels(); ++ch) out.addSample(ch,start+i,x);
   }
 }
 void setParameters(float saw, float pulse, float sub, float pw, float attack, float release) {
   sawLevel=saw; pulseLevel=pulse; subLevel=sub; pulseWidth=pw;
   attackCoef=std::exp(-1.0f/(std::max(0.001f,attack)*static_cast<float>(getSampleRate())));
   releaseCoef=std::exp(-1.0f/(std::max(0.01f,release)*static_cast<float>(getSampleRate())));
 }
private:
 double frequency=440.0, phase=0.0;
 float velocity=0.8f, env=0.0f, sawLevel=0.8f, pulseLevel=0.0f, subLevel=0.0f, pulseWidth=0.5f;
 float attackCoef=0.999f, releaseCoef=0.999f;
 bool releasing=false;
};
}
Juno60AudioProcessor::Juno60AudioProcessor()
 : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
   parameters(*this,nullptr,"PARAMETERS",createParameterLayout()) {
 for(int i=0;i<8;++i) synth.addVoice(new JunoVoice());
 synth.addSound(new JunoSound());
}
juce::AudioProcessorValueTreeState::ParameterLayout Juno60AudioProcessor::createParameterLayout() {
 using P=juce::AudioProcessorValueTreeState;
 P::ParameterLayout p;
 p.add(std::make_unique<juce::AudioParameterFloat>("SAW","Saw",0.0f,1.0f,0.8f));
 p.add(std::make_unique<juce::AudioParameterFloat>("PULSE","Pulse",0.0f,1.0f,0.0f));
 p.add(std::make_unique<juce::AudioParameterFloat>("SUB","Sub",0.0f,1.0f,0.0f));
 p.add(std::make_unique<juce::AudioParameterFloat>("PW","Pulse Width",0.05f,0.95f,0.5f));
 p.add(std::make_unique<juce::AudioParameterFloat>("ATTACK","Attack",0.001f,5.0f,0.01f));
 p.add(std::make_unique<juce::AudioParameterFloat>("RELEASE","Release",0.01f,8.0f,0.25f));
 return p;
}
void Juno60AudioProcessor::prepareToPlay(double sr,int) {
 sampleRate=sr; synth.setCurrentPlaybackSampleRate(sr);
}
bool Juno60AudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
 auto o=l.getMainOutputChannelSet(); return o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo();
}
void Juno60AudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {
 juce::ScopedNoDenormals noDenormals; b.clear();
 for(int i=0;i<synth.getNumVoices();++i)
   if(auto* v=dynamic_cast<JunoVoice*>(synth.getVoice(i)))
     v->setParameters(parameters.getRawParameterValue("SAW")->load(),parameters.getRawParameterValue("PULSE")->load(),
       parameters.getRawParameterValue("SUB")->load(),parameters.getRawParameterValue("PW")->load(),
       parameters.getRawParameterValue("ATTACK")->load(),parameters.getRawParameterValue("RELEASE")->load());
 synth.renderNextBlock(b,m,0,b.getNumSamples());
}
juce::AudioProcessorEditor* Juno60AudioProcessor::createEditor(){return new Juno60AudioProcessorEditor(*this);}
void Juno60AudioProcessor::getStateInformation(juce::MemoryBlock& d){
 if(auto xml=parameters.copyState().createXml()) copyXmlToBinary(*xml,d);
}
void Juno60AudioProcessor::setStateInformation(const void* d,int n){
 if(auto xml=getXmlFromBinary(d,n)) if(xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Juno60AudioProcessor();}