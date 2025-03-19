#pragma once

#include <JuceHeader.h>

class AudioWaveFormComponent: public juce::Component, public juce::ChangeListener, public juce::Timer
{
public:
    AudioWaveFormComponent(juce::AudioFormatManager& formatManagerToUse): formatManager(&formatManagerToUse), thumbnailCache(5)
    {
        // create two audio thumbnails - one for original audio, one for convolved
        originalThumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManagerToUse, thumbnailCache);
        originalThumbnail->addChangeListener(this);
       
        convolvedThumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManagerToUse, thumbnailCache);
        convolvedThumbnail->addChangeListener(this);
        startTimer(40);
    }

    ~AudioWaveFormComponent() override
    {
        if(originalThumbnail)
        {
            originalThumbnail->removeChangeListener(this);
        }
        if(convolvedThumbnail)
        {
            convolvedThumbnail->removeChangeListener(this);
        }
        stopTimer();
    }
    
    void setSource(const juce::File& file)
    {
        if(originalThumbnail)
        {
            originalThumbnail->setSource(new juce::FileInputSource(file));
            repaint();
        }
    }
    
    void setConvolvedSource(const juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        // create a temporary file to store the convolved audio
        juce::File tempFile = juce::File::createTempFile("convolved.wav");

        // create a writer for the file
        std::unique_ptr<juce::AudioFormatWriter> writer(
                    formatManager->findFormatForFileExtension("wav")->createWriterFor(
                        new juce::FileOutputStream(tempFile),
                        sampleRate,
                        buffer.getNumChannels(),
                        16, // bit depth
                        {}, // metadata
                        0 // quality option
                    )
                );
                
        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
            writer.reset();
        
            // set the convolved thumbnail source to the temporary file
            convolvedThumbnail->setSource(new juce::FileInputSource(tempFile));
            isShowingConvolved = true;
            repaint();
        }
    }
    void clearConvolvedSource()
    {
        isShowingConvolved = false;
        repaint();
    }
    
    void setTransportSource(juce::AudioTransportSource* source)
    {
        transportSource = source;
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);

        if(originalThumbnail->getTotalLength() > .0f)
        {
            juce::Rectangle<int> thumbArea(getLocalBounds());
            if (isShowingConvolved)
            {
                juce::Rectangle<int> topHalf = thumbArea.removeFromTop(thumbArea.getHeight() / 2);

                g.setColour(juce::Colours::lightblue);
                g.drawRect(topHalf, 1);
                g.setColour(juce::Colours::white);
                originalThumbnail->drawChannels(g, topHalf, 0.0f, originalThumbnail->getTotalLength(), 1.0f);
                
                // Label for original audio
                g.setColour(juce::Colours::white);
                g.drawText("Original", topHalf.reduced(5), juce::Justification::topLeft, false);
                
                // Bottom half for convolved audio
                g.setColour(juce::Colours::lightgreen);
                g.drawRect(thumbArea, 1);
                g.setColour(juce::Colours::white);
                convolvedThumbnail->drawChannels(g, thumbArea, 0.0f, convolvedThumbnail->getTotalLength(), 1.0f);
                
                // Label for convolved audio
                g.setColour(juce::Colours::white);
                g.drawText("Convolved", thumbArea.reduced(5), juce::Justification::topLeft, false);
                
                // Draw playhead in both sections
                if(transportSource != nullptr)
                {
                    g.setColour(juce::Colours::red);
                    auto audioLength = originalThumbnail->getTotalLength();
                    auto drawPosition = (currentPosition / audioLength) * topHalf.getWidth();
                    g.drawLine(drawPosition + topHalf.getX(), topHalf.getY(),
                               drawPosition + topHalf.getX(), topHalf.getBottom(), 2.0f);
                    
                    drawPosition = (currentPosition / convolvedThumbnail->getTotalLength()) * thumbArea.getWidth();
                    g.drawLine(drawPosition + thumbArea.getX(), thumbArea.getY(),
                               drawPosition + thumbArea.getX(), thumbArea.getBottom(), 2.0f);
                }
            }
            else
            {
                // Just draw the original audio
                g.setColour(juce::Colours::white);
                originalThumbnail->drawChannels(g, thumbArea, 0.0f, originalThumbnail->getTotalLength(), 1.0f);
                
                // Draw the playhead
                if(transportSource != nullptr)
                {
                    g.setColour(juce::Colours::red);
                    auto audioLength = originalThumbnail->getTotalLength();
                    auto drawPosition = (currentPosition / audioLength) * thumbArea.getWidth();
                    g.drawLine(drawPosition, 0.0f, drawPosition, (float)thumbArea.getHeight(), 2.0f);
                }
            }
        }
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if(source == originalThumbnail.get() || source == convolvedThumbnail.get())
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
    std::unique_ptr<juce::AudioThumbnail> originalThumbnail;
    std::unique_ptr<juce::AudioThumbnail> convolvedThumbnail;
    juce::AudioThumbnailCache thumbnailCache { 100 };
    juce::AudioTransportSource* transportSource = nullptr;
    double currentPosition = 0.0;
    juce::File currentFile;
    bool isShowingConvolved = false;
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

class Convolution
{
public:
    Convolution()
    {
        audioFormatManagerForIR->registerBasicFormats();
    }
    /**
        when you use smart pointers for your member variables
        the default destructor will properly clean up those resources by automatically
        calling the destructors of all member variables when a Convolution object is destroyed
     */
    ~Convolution() = default;
    
    void loadImpulseResponse(const juce::File& impulseResponseFile) // can be used later for loading custom IR samples
    {
        audioFormatReaderForIR.reset(audioFormatManagerForIR->createReaderFor(impulseResponseFile));
        if(audioFormatReaderForIR != nullptr)
        {
            const int numberOfChannels = audioFormatReaderForIR->numChannels;
            const int numberOfSamples = static_cast<int>(audioFormatReaderForIR->lengthInSamples);
            
            impulseResponseBuffer.setSize(numberOfChannels, numberOfSamples);
            
            audioFormatReaderForIR->read(&impulseResponseBuffer, 0, numberOfSamples, 0, true, true);
            
            readerSourceForIR = std::make_unique<juce::AudioFormatReaderSource>(audioFormatReaderForIR.get(), false);
            
            impulseResponseLength = numberOfSamples;
        }
    }
        
    /**
        in c++ const void* data represents a pointer to data of an unspecified type
        the juce BinaryData system itself uses void* for all its resources.
    */
    void loadImpulseResponseFromBinaryDataInAssets(const void* data, size_t dataSize)
    {
        auto inputStream = std::make_unique<juce::MemoryInputStream>(data, dataSize, false);
            
        audioFormatReaderForIR.reset(audioFormatManagerForIR->createReaderFor(std::move(inputStream)));
        if(audioFormatReaderForIR != nullptr)
        {
            const int numberOfChannels = audioFormatReaderForIR->numChannels;
            const int numberOfSamples = static_cast<int>(audioFormatReaderForIR->lengthInSamples);
            impulseResponseBuffer.setSize(numberOfChannels, numberOfSamples);
                
            audioFormatReaderForIR->read(&impulseResponseBuffer, 0, numberOfSamples, 0, true, true);
                
            readerSourceForIR = std::make_unique<juce::AudioFormatReaderSource>(audioFormatReaderForIR.get(), false);
                
            impulseResponseLength = numberOfSamples;
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer)
    {
        if(impulseResponseLength == 0)
            return;
        
        const int numberOfChannels = buffer.getNumChannels();
        const int numberOfSamples = buffer.getNumSamples();
        
        // create a temporary buffer for the convolution output
        juce::AudioBuffer<float> tempBuffer(numberOfChannels, numberOfSamples);
        tempBuffer.clear();
        
        // normalization factor to prevent clipping
        const float normalizationFactor = 0.1f;
        
        // use a shorter portion of the impulse response for real-time processing
        // This is a compromise for educational purposes
        const int usableIRLength = juce::jmin(4096, impulseResponseLength);
        
        // making sure the outout buffer is large enough
        if(outputBuffer.getNumChannels() < numberOfChannels || outputBuffer.getNumSamples() < numberOfSamples + impulseResponseLength - 1)
        {
            outputBuffer.setSize(numberOfChannels, numberOfSamples + impulseResponseLength - 1);
        }
        outputBuffer.clear();

        for(int channel = 0; channel < numberOfChannels; ++channel)
        {
            const float* inputData = buffer.getReadPointer(channel);
            float* outputData = outputBuffer.getWritePointer(channel);
            
            const int impulseResponseChannel = std::min(channel, impulseResponseBuffer.getNumChannels() - 1);
            const float* impulseResponseData = impulseResponseBuffer.getReadPointer(impulseResponseChannel);
            
            // main algo implement time domain convolution
            // y[n] = sum(x[k] * h[n-k])
            for(int n = 0; n < numberOfSamples + impulseResponseLength - 1; ++n)
            {
                for(int k = 0; k < usableIRLength; ++k)
                {
                    // making sure we are not accessing out of bounds on input
                    if(n - k >= 0 && n - k < numberOfSamples)
                    {
                        outputData[n] += inputData[n - k] * impulseResponseData[k] * normalizationFactor;
                    }
                    else
                    {
                        // for simplicity, we're ignoring this in this example
                        break;
                    }
                }
            }
            buffer.copyFrom(channel, 0, outputBuffer, channel, 0, numberOfSamples);
            if (numberOfChannels > 1)
                buffer.copyFrom(1, 0, tempBuffer, 1, 0, numberOfSamples);
        }
        
    }

    int getImpulseResponseLength() const { return impulseResponseLength; }

    float getFirstSampleValue() const
    {
        if (impulseResponseBuffer.getNumSamples() > 0)
            return impulseResponseBuffer.getSample(0, 0);
        return 0.0f;
    }
    
    private:
        std::unique_ptr<juce::AudioFormatManager> audioFormatManagerForIR = std::make_unique<juce::AudioFormatManager>();
        std::unique_ptr<juce::AudioFormatReader> audioFormatReaderForIR;
        std::unique_ptr<juce::AudioFormatReaderSource> readerSourceForIR;
        juce::AudioBuffer<float> impulseResponseBuffer;
        juce::AudioBuffer<float> outputBuffer;
        int impulseResponseLength = 0;
};

class ConvolutionProcessor : public juce::AudioProcessor
{
public:
    ConvolutionProcessor(Convolution& convolutionToUse) : convolution(convolutionToUse)
    {
        audioFormatManager.registerBasicFormats();
    }
    
    void prepareToPlay(double sampleRate, int maximumSamplesPerBlock) override
    {
        if(audioSource != nullptr)
        {
            audioSource->prepareToPlay(maximumSamplesPerBlock, sampleRate);
        }
    }
    
    void releaseResources() override
    {
        if(audioSource != nullptr)
        {
            audioSource->releaseResources();
        }
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        buffer.clear();
        if(audioSource != nullptr)
        {
            // AudioSourceChannelInfo object pointing to our buffer
            juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
            
            audioSource->getNextAudioBlock(info);
            
            // thread-safe check using atomic variable
            const bool shouldConvolve = isConvolutionEnabled.load();
            
            if(shouldConvolve)
            {
                // make sure the convolution doesn't change the buffer dimensions
                convolution.process(buffer);
            }
        }
        juce::ignoreUnused(midiMessages);
    }
    
    void createConvolvedPreview(const juce::File& sourceFile, std::function<void(const juce::AudioBuffer<float>&, double)> callback)
    {
        // Load the source file
        std::unique_ptr<juce::AudioFormatReader> reader(audioFormatManager.createReaderFor(sourceFile));
        
        if (reader != nullptr)
        {
            // Create a buffer large enough for the whole file
            juce::AudioBuffer<float> fileBuffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
            
            // Read the entire file into the buffer
            reader->read(&fileBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
            
            // Apply convolution if enabled
            if (isConvolutionEnabled)
            {
                convolution.process(fileBuffer);
            }
            
            // Call the callback with the processed buffer
            callback(fileBuffer, reader->sampleRate);
        }
    }
    
    // required overrides for AudioProcessor
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Convolution Processor"; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    void setConvolutionEnabled(bool shouldBeEnabled) { isConvolutionEnabled.store(shouldBeEnabled); }
    void setAudioSource(juce::AudioSource* source) { audioSource = source; }

private:
    Convolution& convolution;
    juce::AudioSource* audioSource = nullptr;
    std::atomic<bool> isConvolutionEnabled { false };
    juce::AudioFormatManager audioFormatManager;
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
    juce::AudioProcessorPlayer processorPlayer;
    juce::ComboBox convolutionOptions;
    
    AudioWaveFormComponent waveformDisplay;
    ButtonGroupForWavFileProcessing waveFileHandlerButtons;
    Convolution timeDomainConvolution;
    std::unique_ptr<ConvolutionProcessor> convolutionProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
