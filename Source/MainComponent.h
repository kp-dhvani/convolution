#pragma once

#include <JuceHeader.h>

class AudioWaveFormComponent: public juce::Component, public juce::ChangeListener
{
public:
    AudioWaveFormComponent(juce::AudioFormatManager& formatManagerToUse): formatManager(&formatManagerToUse), thumbnailCache(5)
    {
        thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManagerToUse, thumbnailCache);
        thumbnail->addChangeListener(this);
    }

    ~AudioWaveFormComponent() override
    {
        if(thumbnail)
        {
            thumbnail->removeChangeListener(this);
            repaint();
        }
    }
    
    void setSource(const juce::File& file)
    {
        if(thumbnail)
        {
            thumbnail->setSource(new juce::FileInputSource(file));
            repaint();
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        
        if(thumbnail->getTotalLength() > .0f)
        {
            juce::Rectangle<int> thumbArea(getLocalBounds());
            thumbnail->drawChannels(g, thumbArea, .0f, thumbnail->getTotalLength(), 1.0f);
        }
        else
        {
            g.drawFittedText("no file loaded", getLocalBounds(), juce::Justification::centred, 1);
        }
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if(source == thumbnail.get())
        {
            repaint();
        }
    }
    
private:
    juce::AudioFormatManager* formatManager;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    juce::AudioThumbnailCache thumbnailCache { 100 };
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component, private juce::Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void buttonClicked(juce::Button* button) override;

private:
    //==============================================================================
    juce::TextButton openFileBrowserButton;
    juce::String fileBrowserStatusMessage;
    
    // AudioFormatManager → AudioFormatReader → AudioTransportSource → AudioSourcePlayer → AudioDeviceManager → Audio Hardware
    std::unique_ptr<juce::AudioFormatManager> audioFormatManager = std::make_unique<juce::AudioFormatManager>();
    std::unique_ptr<juce::AudioFormatReader> audioFormatReader;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::FileChooser> currentFileChooser;
    juce::AudioTransportSource transportSource;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer sourcePlayer;
    
    AudioWaveFormComponent waveformDisplay;
    
    void loadWavFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
