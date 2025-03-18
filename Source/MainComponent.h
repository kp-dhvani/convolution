#pragma once

#include <JuceHeader.h>

class AudioWaveFormComponent: public juce::Component, public juce::ChangeListener, public juce::Timer
{
public:
    AudioWaveFormComponent(juce::AudioFormatManager& formatManagerToUse): formatManager(&formatManagerToUse), thumbnailCache(5)
    {
        thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManagerToUse, thumbnailCache);
        thumbnail->addChangeListener(this);
        startTimer(40);
    }

    ~AudioWaveFormComponent() override
    {
        if(thumbnail)
        {
            thumbnail->removeChangeListener(this);
            stopTimer();
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
    
    void setTransportSource(juce::AudioTransportSource* source)
    {
        transportSource = source;
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        
        if(thumbnail->getTotalLength() > .0f)
        {
            juce::Rectangle<int> thumbArea(getLocalBounds());
            thumbnail->drawChannels(g, thumbArea, .0f, thumbnail->getTotalLength(), 1.0f);
            
            if(transportSource != nullptr)
            {
                g.setColour(juce::Colours::red);
                auto audioLength = thumbnail->getTotalLength();
                auto drawPosition = (currentPosition / audioLength) * thumbArea.getWidth();
                g.drawLine(drawPosition, 0.0f, drawPosition, (float)thumbArea.getHeight(), 2.0f);
            }
        }
        else
        {
            g.drawFittedText("No file loaded", getLocalBounds(), juce::Justification::centred, 1);
        }
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if(source == thumbnail.get())
        {
            repaint();
        }
    }
    
    void timerCallback() override
    {
        if(transportSource != nullptr)
        {
            currentPosition = transportSource->getCurrentPosition();
            repaint();
        }
    }
    
private:
    juce::AudioFormatManager* formatManager;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    juce::AudioThumbnailCache thumbnailCache { 100 };
    juce::AudioTransportSource* transportSource = nullptr;
    double currentPosition;
};


class ButtonGroupForWavFileProcessing : public juce::Component, private juce::Button::Listener
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void loadWavFileButtonClicked() = 0;
        virtual void playButtonClicked(bool shouldPlay) = 0;
        virtual void shouldLoopToggled(bool shouldLoop) = 0;
    };

    ButtonGroupForWavFileProcessing()
    {
        addAndMakeVisible(openFileBrowserButton);
        addAndMakeVisible(playWavFile);
        addAndMakeVisible(loopButton);
        
        openFileBrowserButton.addListener(this);
        playWavFile.addListener(this);
        loopButton.addListener(this);
    }
    
    ~ButtonGroupForWavFileProcessing()
    {
        openFileBrowserButton.removeListener(this);
        playWavFile.removeListener(this);
        loopButton.removeListener(this);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
            auto buttonHeight = bounds.getHeight() - 10; // slightly smaller than container height

        // make buttons take up approximately 25-30% of the width each
        // this leaves room for spacing between them
        int buttonWidth = bounds.getWidth() / 4;
    
        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
        flex.alignItems = juce::FlexBox::AlignItems::center;
    
        flex.items.add(juce::FlexItem(openFileBrowserButton).withWidth(buttonWidth).withHeight(buttonHeight));

        flex.items.add(juce::FlexItem(playWavFile).withWidth(buttonWidth).withHeight(buttonHeight));
                  
        flex.items.add(juce::FlexItem(loopButton).withWidth(buttonWidth).withHeight(buttonHeight));

        flex.performLayout(bounds);
    }
    void setListener(Listener* newListener) { listener = newListener; }
    void buttonClicked(juce::Button* button) override
    {
        if(listener == nullptr)
            return;
        if(button == &openFileBrowserButton)
        {
            listener->loadWavFileButtonClicked();
        }
        else if(button == &playWavFile)
        {
            listener->playButtonClicked(playWavFile.getToggleState());
        }
        else if(button == &loopButton)
        {
            listener->shouldLoopToggled(loopButton.getToggleState());
        }
    }
    
    void updatePlayButtonText(bool isPlaying)
    {
        playWavFile.setButtonText(isPlaying ? "Stop" : "Play");
    }
    
    juce::TextButton openFileBrowserButton { "Load a WAV file" };
    juce::TextButton playWavFile { "Play" };
    juce::ToggleButton loopButton { "Loop File" };
    
private:
    Listener* listener = nullptr;
    
    
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component, private juce::ComboBox::Listener, private ButtonGroupForWavFileProcessing::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void loadWavFile();
    void loadWavFileButtonClicked() override;
    void playButtonClicked(bool shouldPlay) override;
    void shouldLoopToggled(bool shouldLoop) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

private:
    //==============================================================================
    juce::String convolutionComboboxText;
    
    // AudioFormatManager → AudioFormatReader → AudioTransportSource → AudioSourcePlayer → AudioDeviceManager → Audio Hardware
    std::unique_ptr<juce::AudioFormatManager> audioFormatManager = std::make_unique<juce::AudioFormatManager>();
    std::unique_ptr<juce::AudioFormatReader> audioFormatReader;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::FileChooser> currentFileChooser;
    juce::AudioTransportSource transportSource;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer sourcePlayer;
    juce::ComboBox convolutionOptions;
    
    AudioWaveFormComponent waveformDisplay;
    ButtonGroupForWavFileProcessing waveFileHandlerButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
