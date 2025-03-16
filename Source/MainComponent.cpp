#include "MainComponent.h"
#include <JuceHeader.h>

//==============================================================================
MainComponent::MainComponent()
    : openFileBrowserButton("Load a WAV file"),
    fileBrowserStatusMessage("No file loaded"),
    waveformDisplay(*audioFormatManager)
{
    addAndMakeVisible(openFileBrowserButton);
    openFileBrowserButton.addListener(this);
    
    audioFormatManager->registerBasicFormats();
    
    addAndMakeVisible(waveformDisplay);
    
    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(&sourcePlayer);
    sourcePlayer.setSource(&transportSource);
    setSize (600, 400);
}

MainComponent::~MainComponent()
{
    openFileBrowserButton.removeListener(this);
    sourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&sourcePlayer);
    transportSource.setSource(nullptr);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText(fileBrowserStatusMessage, getLocalBounds().removeFromBottom(40), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto area = getLocalBounds();
    // Status area at the bottom
    auto statusArea = area.removeFromBottom(40);
    // Button area at the bottom of the remaining space
    auto buttonArea = area.removeFromBottom(60);
    openFileBrowserButton.setBounds(buttonArea.getCentreX() - 100, buttonArea.getCentreY() - 20, 200, 40);
    
    // Use the rest of the space for the waveform display
    waveformDisplay.setBounds(area);
}


void MainComponent::buttonClicked(juce::Button *button) {
    if(button == &openFileBrowserButton) {
        loadWavFile();
    }
}

void MainComponent::loadWavFile() {
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
                
                transportSource.start();
                
                fileBrowserStatusMessage = "Playing " + file.getFileName();
                repaint();
            }
            else
            {
                fileBrowserStatusMessage = "Failed to load the file (unsupported format)";
                repaint();
            }
        }
        else {
            fileBrowserStatusMessage = "Failed to load the file";
            repaint();
        }
    });
    
}
