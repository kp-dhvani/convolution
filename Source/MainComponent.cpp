#include "MainComponent.h"
#include <JuceHeader.h>

//==============================================================================
MainComponent::MainComponent():
    convolutionComboboxText("Convolute with"),
    waveformDisplay(*audioFormatManager)
    
{
    addAndMakeVisible(waveFileHandlerButtons);
    waveFileHandlerButtons.setListener(this);
    
    audioFormatManager->registerBasicFormats();
    
    waveformDisplay.setTransportSource(&transportSource);
    addAndMakeVisible(waveformDisplay); // adds the component to the parent's hierarchy - a must for rendering anything
    
    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(&transportSource);
    
    convolutionOptions.addItem("No convolution", 1);
    convolutionOptions.addItem("convolution option 1", 2);
    convolutionOptions.addItem("convolution option 2", 3);
    convolutionOptions.addItem("convolution option 3", 4);
    convolutionOptions.addItem("convolution option 4", 5);
    convolutionOptions.setSelectedItemIndex(0);
    
    addAndMakeVisible(convolutionOptions);
    convolutionOptions.addListener(this);
    setSize(600, 400);
}

MainComponent::~MainComponent()
{
    sourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&sourcePlayer);
    transportSource.setSource(nullptr);
}

// used only for graphics only and not layout
// use resized for layout
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
}

void MainComponent::resized()
{

    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::column;
    flex.justifyContent = juce::FlexBox::JustifyContent::center;
    flex.alignItems = juce::FlexBox::AlignItems::center;
    flex.flexWrap = juce::FlexBox::Wrap::noWrap;


    flex.items.add(juce::FlexItem(waveformDisplay).withFlex(4.0f).withWidth(getWidth() * 0.9f).withHeight(getHeight() * 0.6f));

    // add some spacing
    flex.items.add(juce::FlexItem().withHeight(20));

    flex.items.add(juce::FlexItem(waveFileHandlerButtons).withFlex(1.0f).withWidth(getWidth()).withHeight(50));

    // add some more spacing
    flex.items.add(juce::FlexItem().withHeight(20));
    
    flex.items.add(juce::FlexItem(convolutionOptions).withFlex(1.0f).withWidth(getWidth() * 0.5f).withHeight(30));

    flex.performLayout(getLocalBounds().reduced(10));
}


void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if(comboBoxThatHasChanged == &convolutionOptions)
    {
        int selectedId = convolutionOptions.getSelectedId();
            switch (selectedId)
            {
                case 1:
                    break;
                case 2:
                    break;
            }
    }
}

void MainComponent::loadWavFileButtonClicked()
{
    loadWavFile();
}

void MainComponent::playButtonClicked(bool shouldPlay)
{
    if(transportSource.isPlaying())
    {
        transportSource.stop();
    }
    else
    {
        transportSource.start();
    }
    waveFileHandlerButtons.updatePlayButtonText(shouldPlay);
}

void MainComponent::shouldLoopToggled(bool shouldLoop)
{
    if(readerSource != nullptr)
    {
        readerSource->setLooping(shouldLoop);
    }
}

void MainComponent::loadWavFile()
{
    currentFileChooser = std::make_unique<juce::FileChooser>("Select a WAV file...", juce::File{}, "*.wav");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    
    currentFileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if(file.exists()) {
            // create a reader for the file
            std::unique_ptr<juce::AudioFormatReader> reader(audioFormatManager->createReaderFor(file));
            if(reader != nullptr) {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader.get(), false);
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                readerSource = std::move(newSource);
                audioFormatReader.reset(reader.release());
                
                waveformDisplay.setSource(file);
                if(waveFileHandlerButtons.loopButton.getToggleState())
                {
                    readerSource->setLooping(true);
                    transportSource.start();
                    waveFileHandlerButtons.updatePlayButtonText(true);
                }
                repaint();
            }
        }
    });
    
}
