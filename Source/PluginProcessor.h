#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <functional>

class StackProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSlots = 20;

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void slotsChanged() {}
        virtual void slotWillBeRemoved (int) {}
    };

    StackProcessor();
    ~StackProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "STACK"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Slot management (message thread)
    bool hasPlugin (int slot) const;
    juce::String getSlotName (int slot) const;
    bool isBypassed (int slot) const;
    void setBypassed (int slot, bool shouldBypass);
    juce::AudioPluginInstance* getPlugin (int slot) const;
    void loadFromPath (int slot, const juce::String& path, std::function<void (const juce::String&)> onError);
    void removePlugin (int slot);
    juce::StringArray scanVst3Paths();
    int getActiveCount() const;
    int getTotalLatencySamples() const { return totalLatency.load(); }

    void addListener (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    juce::AudioParameterFloat* mixParam = nullptr;

private:
    struct Slot
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::PluginDescription desc;
        bool bypass = false;
    };

    void loadInstance (int slot, const juce::PluginDescription& desc, const juce::String& stateBase64,
                       bool bypass, std::function<void (const juce::String&)> onError);
    void updateLatency();

    juce::AudioPluginFormatManager formatManager;
    std::array<Slot, numSlots> slots;
    juce::CriticalSection lock;
    juce::ListenerList<Listener> listeners;

    double currentSR = 44100.0;
    int currentBlock = 512;
    std::atomic<int> totalLatency { 0 };

    juce::AudioBuffer<float> work;
    juce::AudioBuffer<float> dryBuffer;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 192000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackProcessor)
};
