#pragma once

#include "PluginProcessor.h"
#include <map>

juce::Font jostFont (float height, float kerning = 0.0f);

class StackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
};

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
    StackLookAndFeel laf;

    juce::OwnedArray<SlotRow> rows;
    std::map<int, std::unique_ptr<PluginWindow>> windows;
    juce::StringArray paths;
    juce::String infoText { "Tap an empty slot to load a plugin" };
    int selected = -1;
    int pendingSelect = -1;

    juce::TextButton addButton { "Add" };
    juce::Slider mixSlider;
    std::unique_ptr<juce::SliderParameterAttachment> mixAttach;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_WEAK_REFERENCEABLE (StackEditor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackEditor)
};
