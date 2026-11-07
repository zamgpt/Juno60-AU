#include "PluginEditor.h"
Juno60AudioProcessorEditor::Juno60AudioProcessorEditor(Juno60AudioProcessor& p):AudioProcessorEditor(&p),processor(p){
 setSize(900,420);
 const std::array<std::pair<const char*,const char*>,6> defs{{
  {"SAW","SAW"}, {"PULSE","PULSE"}, {"SUB","SUB"}, {"PW","PULSE WIDTH"}, {"ATTACK","ATTACK"}, {"RELEASE","RELEASE"}}};
 for(auto [id,name]:defs){
  auto* s=sliders.add(new juce::Slider()); s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,70,20); addAndMakeVisible(s);
  auto* l=labels.add(new juce::Label()); l->setText(name,juce::dontSendNotification);
  l->setJustificationType(juce::Justification::centred); addAndMakeVisible(l);
  attachments.push_back(std::make_unique<Attachment>(processor.parameters,id,*s));
 }
}
void Juno60AudioProcessorEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xffd8dce0)); g.setColour(juce::Colour(0xfff3f4f5));
 g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(10),8);
 g.setColour(juce::Colour(0xff24384a)); g.setFont(juce::Font(juce::FontOptions(30.0f,juce::Font::bold)));
 g.drawText("JUNO-60",28,20,260,44,juce::Justification::left);
 g.setColour(juce::Colour(0xff287eaa)); g.fillRect(10,76,getWidth()-20,5);
}
void Juno60AudioProcessorEditor::resized(){
 const int w=132; for(int i=0;i<sliders.size();++i){
  int x=24+i*145; sliders[i]->setBounds(x,130,w,180); labels[i]->setBounds(x,105,w,24);
 }
}