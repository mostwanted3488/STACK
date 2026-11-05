#pragma once

#include "PluginProcessor.h"
#include <map>

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow (juce::AudioPluginInstance& plugin, const juce::String& title, std::function<void()> onClose);
    void closeButtonPressed() override;

private:
    std::function<void()> closeCallback;
};

class SlotRow : public juce::Component
{
public:
    SlotRow (StackProcessor& p, int idx) : proc (p), index (idx) {}

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    std::function<void (int)> onOpen, onLoad;

private:
    StackProcessor& proc;
    int index;
};

class StackEditor : public juce::AudioProcessorEditor,
                    private StackProcessor::Listener
{
public:
    explicit StackEditor (StackProcessor&);
    ~StackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void slotsChanged() override;
    void slotWillBeRemoved (int slot) override;

    void openSlotEditor (int slot);
    void showLoadMenu (int slot);
    void menuResult (int slot, int result);
    void loadPath (int slot, const juce::String& path);
    juce::Rectangle<int> getFooterBounds() const;

    StackProcessor& proc;

    juce::OwnedArray<SlotRow> rows;
    std::map<int, std::unique_ptr<PluginWindow>> windows;
    juce::StringArray paths;
    juce::String infoText { "Clique un slot vide pour charger un plugin VST3." };

    juce::TextButton addButton { "Ajouter un plugin" };
    juce::Slider mixSlider;
    juce::Label mixLabel { {}, "Dry / Wet global" };
    std::unique_ptr<juce::SliderParameterAttachment> mixAttach;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_WEAK_REFERENCEABLE (StackEditor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackEditor)
};
