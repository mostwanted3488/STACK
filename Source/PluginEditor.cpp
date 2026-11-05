#include "PluginEditor.h"

namespace
{
    const juce::Colour colBg      { 0xff0e0e0e };
    const juce::Colour colRow     { 0xff171717 };
    const juce::Colour colBorder  { 0xff262626 };
    const juce::Colour colText    { 0xffececec };
    const juce::Colour colMuted   { 0xff8a8a8a };
    const juce::Colour colDim     { 0xff6d6d6d };
    const juce::Colour colBright  { 0xfff2f2f2 };

    constexpr int pad = 14;
    constexpr int rowH = 34;
    constexpr int rowGap = 4;
    constexpr int colGap = 12;
    constexpr int headerH = 34;
    constexpr int labelH = 18;
    constexpr int footH = 40;
    constexpr int mixH = 24;
}

// ------------------------------------------------------------ PluginWindow

PluginWindow::PluginWindow (juce::AudioPluginInstance& plugin, const juce::String& title, std::function<void()> onClose)
    : juce::DocumentWindow (title, juce::Colours::black, juce::DocumentWindow::closeButton),
      closeCallback (std::move (onClose))
{
    setUsingNativeTitleBar (true);

    bool resizable = false;
    if (auto* ed = plugin.createEditorIfNeeded())
    {
        resizable = ed->isResizable();
        setContentOwned (ed, true);
    }
    else
    {
        setContentOwned (new juce::GenericAudioProcessorEditor (plugin), true);
    }

    setResizable (resizable, false);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
    toFront (true);
}

void PluginWindow::closeButtonPressed()
{
    if (closeCallback)
        closeCallback();
}

// ----------------------------------------------------------------- SlotRow

void SlotRow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    const bool has = proc.hasPlugin (index);
    const bool on = has && ! proc.isBypassed (index);

    if (has)
    {
        g.setColour (colRow);
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (colBorder);
        g.drawRoundedRectangle (b, 6.0f, 1.0f);
    }
    else
    {
        juce::Path p, dashed;
        p.addRoundedRectangle (b, 6.0f);
        const float dashes[] = { 3.0f, 3.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dashes, 2);
        g.setColour (juce::Colour (0xff3a3a3a));
        g.fillPath (dashed);
    }

    const int h = getHeight();

    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (colDim);
    g.drawText (juce::String (index + 1), 4, 0, 18, h, juce::Justification::centredRight);

    if (has)
    {
        auto led = juce::Rectangle<float> (30.0f, (float) h * 0.5f - 5.0f, 10.0f, 10.0f);
        g.setColour (on ? colBright : juce::Colour (0xff2c2c2c));
        g.fillEllipse (led);
        g.setColour (on ? colBright : juce::Colour (0xff4a4a4a));
        g.drawEllipse (led, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.setColour (on ? colText : colDim);
        g.drawText (proc.getSlotName (index), 48, 0, getWidth() - 48 - 28, h,
                    juce::Justification::centredLeft, true);

        g.setFont (juce::Font (juce::FontOptions (14.0f)));
        g.setColour (colDim);
        g.drawText ("x", getWidth() - 26, 0, 20, h, juce::Justification::centred);
    }
    else
    {
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.setColour (juce::Colour (0xff5c5c5c));
        g.drawText ("Vide", 48, 0, getWidth() - 56, h, juce::Justification::centredLeft);
    }
}

void SlotRow::mouseDown (const juce::MouseEvent& e)
{
    if (! proc.hasPlugin (index))
    {
        if (onLoad)
            onLoad (index);
        return;
    }

    if (e.x >= getWidth() - 28)
        proc.removePlugin (index);
    else if (e.x >= 24 && e.x <= 46)
        proc.setBypassed (index, ! proc.isBypassed (index));
    else if (onOpen)
        onOpen (index);
}

// -------------------------------------------------------------- StackEditor

StackEditor::StackEditor (StackProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < StackProcessor::numSlots; ++i)
    {
        auto* r = rows.add (new SlotRow (proc, i));
        r->onOpen = [this] (int s) { openSlotEditor (s); };
        r->onLoad = [this] (int s) { showLoadMenu (s); };
        addAndMakeVisible (r);
    }

    addButton.setColour (juce::TextButton::buttonColourId, colBright);
    addButton.setColour (juce::TextButton::textColourOffId, colBg);
    addButton.onClick = [this]
    {
        for (int i = 0; i < StackProcessor::numSlots; ++i)
            if (! proc.hasPlugin (i))
            {
                showLoadMenu (i);
                return;
            }

        infoText = "Les 20 slots sont pleins.";
        repaint();
    };
    addAndMakeVisible (addButton);

    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
    mixSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xffbdbdbd));
    mixSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2a2a2a));
    mixSlider.setColour (juce::Slider::thumbColourId, colBright);
    mixSlider.setColour (juce::Slider::textBoxTextColourId, colMuted);
    mixSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (mixSlider);
    mixAttach = std::make_unique<juce::SliderParameterAttachment> (*proc.mixParam, mixSlider, nullptr);

    mixLabel.setColour (juce::Label::textColourId, colMuted);
    mixLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (mixLabel);

    const int w = pad * 2 + 2 * 320 + colGap;
    const int h = pad + headerH + labelH + 10 * (rowH + rowGap) + 6 + footH + 6 + mixH + pad;
    setSize (w, h);

    proc.addListener (this);
}

StackEditor::~StackEditor()
{
    proc.removeListener (this);
    windows.clear();
}

void StackEditor::slotsChanged()
{
    repaint();
}

void StackEditor::slotWillBeRemoved (int slot)
{
    windows.erase (slot);
}

void StackEditor::openSlotEditor (int slot)
{
    auto* plugin = proc.getPlugin (slot);
    if (plugin == nullptr)
        return;

    infoText = "Slot " + juce::String (slot + 1) + " : " + proc.getSlotName (slot);
    repaint();

    auto it = windows.find (slot);
    if (it != windows.end())
    {
        it->second->toFront (true);
        return;
    }

    juce::WeakReference<StackEditor> weak (this);
    windows[slot] = std::make_unique<PluginWindow> (*plugin, proc.getSlotName (slot), [weak, slot]
    {
        juce::MessageManager::callAsync ([weak, slot]
        {
            if (weak != nullptr)
                weak->windows.erase (slot);
        });
    });
}

void StackEditor::showLoadMenu (int slot)
{
    if (paths.isEmpty())
        paths = proc.scanVst3Paths();

    std::map<juce::String, juce::PopupMenu> groups;
    for (int i = 0; i < paths.size(); ++i)
    {
        auto name = juce::File (paths[i]).getFileNameWithoutExtension();
        auto key = name.substring (0, 1).toUpperCase();
        if (! juce::CharacterFunctions::isLetter (key[0]))
            key = "#";
        groups[key].addItem (i + 1, name);
    }

    juce::PopupMenu menu;
    for (auto& kv : groups)
        menu.addSubMenu (kv.first, kv.second);

    if (! groups.empty())
        menu.addSeparator();

    menu.addItem (100000, "Parcourir...");
    menu.addItem (100001, "Rescanner les dossiers VST3");

    juce::WeakReference<StackEditor> weak (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (rows[slot]),
                        [weak, slot] (int result)
                        {
                            if (weak != nullptr && result != 0)
                                weak->menuResult (slot, result);
                        });
}

void StackEditor::menuResult (int slot, int result)
{
    if (result == 100001)
    {
        paths = proc.scanVst3Paths();
        showLoadMenu (slot);
        return;
    }

    if (result == 100000)
    {
        chooser = std::make_unique<juce::FileChooser> ("Choisis un plugin VST3",
                                                       juce::File ("C:\\Program Files\\Common Files\\VST3"),
                                                       "*.vst3");
        juce::WeakReference<StackEditor> weak (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [weak, slot] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (weak != nullptr && f != juce::File())
                                      weak->loadPath (slot, f.getFullPathName());
                              });
        return;
    }

    if (juce::isPositiveAndBelow (result - 1, paths.size()))
        loadPath (slot, paths[result - 1]);
}

void StackEditor::loadPath (int slot, const juce::String& path)
{
    infoText = "Chargement de " + juce::File (path).getFileNameWithoutExtension() + "...";
    repaint();

    juce::WeakReference<StackEditor> weak (this);
    proc.loadFromPath (slot, path, [weak] (const juce::String& err)
    {
        if (weak != nullptr)
        {
            weak->infoText = "Echec : " + err;
            weak->repaint();
        }
    });
}

void StackEditor::paint (juce::Graphics& g)
{
    g.fillAll (colBg);

    g.setColour (colBright);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)).withExtraKerningFactor (0.2f));
    g.drawText ("STACK", pad, pad, 110, headerH, juce::Justification::centredLeft);

    g.setColour (colDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)).withExtraKerningFactor (0.12f));
    g.drawText ("20 SLOTS DANS 1", pad + 112, pad, 200, headerH, juce::Justification::centredLeft);

    const float ms = proc.getSampleRate() > 0.0
                         ? (float) proc.getTotalLatencySamples() * 1000.0f / (float) proc.getSampleRate()
                         : 0.0f;
    g.setColour (colMuted);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Actifs " + juce::String (proc.getActiveCount()) + "   Latence " + juce::String (ms, 1) + " ms",
                getWidth() - pad - 260, pad, 260, headerH, juce::Justification::centredRight);

    const int colW = (getWidth() - pad * 2 - colGap) / 2;
    g.setColour (colDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)).withExtraKerningFactor (0.12f));
    g.drawText ("SLOTS 1 A 10", pad, pad + headerH, colW, labelH, juce::Justification::centredLeft);
    g.drawText ("SLOTS 11 A 20", pad + colW + colGap, pad + headerH, colW, labelH, juce::Justification::centredLeft);

    auto foot = getFooterBounds();
    g.setColour (colRow);
    g.fillRoundedRectangle (foot.toFloat(), 6.0f);
    g.setColour (colBorder);
    g.drawRoundedRectangle (foot.toFloat().reduced (0.5f), 6.0f, 1.0f);

    g.setColour (colMuted);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (infoText, foot.reduced (12, 0).withTrimmedRight (150), juce::Justification::centredLeft, true);
}

juce::Rectangle<int> StackEditor::getFooterBounds() const
{
    const int y = pad + headerH + labelH + 10 * (rowH + rowGap) + 6;
    return { pad, y, getWidth() - pad * 2, footH };
}

void StackEditor::resized()
{
    const int colW = (getWidth() - pad * 2 - colGap) / 2;
    const int top = pad + headerH + labelH;

    for (int i = 0; i < rows.size(); ++i)
    {
        const int col = i / 10;
        const int r = i % 10;
        rows[i]->setBounds (pad + col * (colW + colGap), top + r * (rowH + rowGap), colW, rowH);
    }

    auto foot = getFooterBounds();
    addButton.setBounds (foot.getRight() - 12 - 130, foot.getY() + 8, 130, foot.getHeight() - 16);

    const int my = foot.getBottom() + 6;
    mixLabel.setBounds (pad, my, 120, mixH);
    mixSlider.setBounds (pad + 124, my, getWidth() - pad * 2 - 124, mixH);
}
