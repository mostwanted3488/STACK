#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr int pad = 24;
    constexpr int topPad = 26;
    constexpr int headerH = 40;
    constexpr int rowH = 36;
    constexpr int rowGap = 8;
    constexpr int colGap = 14;
    constexpr int footH = 50;
    constexpr int mixH = 24;
    constexpr int winW = 680;

    const juce::Colour ink { 0xff1e2024 };

    juce::Colour inkA (float a) { return ink.withAlpha (a); }
    juce::Colour white (float a) { return juce::Colours::white.withAlpha (a); }

    void drawGlass (juce::Graphics& g, juce::Rectangle<float> r, float corner, bool shadow = true)
    {
        juce::Path p;
        p.addRoundedRectangle (r, corner);

        if (shadow)
            juce::DropShadow (juce::Colour::fromFloatRGBA (0.24f, 0.25f, 0.28f, 0.2f), 10, { 0, 4 }).drawForPath (g, p);

        g.setGradientFill (juce::ColourGradient (white (0.62f), r.getX(), r.getY(),
                                                 white (0.16f), r.getRight(), r.getBottom(), false));
        g.fillPath (p);

        g.saveState();
        g.reduceClipRegion (p);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.0f, r.getY() + r.getHeight() * 0.55f,
                                                 juce::Colour::fromFloatRGBA (0.35f, 0.37f, 0.41f, 0.13f), 0.0f, r.getBottom(), false));
        g.fillRect (r);
        g.restoreState();

        g.setColour (white (0.95f));
        g.drawLine (r.getX() + corner * 0.7f, r.getY() + 1.0f, r.getRight() - corner * 0.7f, r.getY() + 1.0f, 1.0f);

        g.setColour (white (0.78f));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
    }

    void drawOrb (juce::Graphics& g, juce::Point<float> c, float radius, bool lit)
    {
        auto e = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c);

        if (lit)
        {
            g.setGradientFill (juce::ColourGradient (white (0.95f), c.x, c.y, white (0.0f), c.x + radius * 2.6f, c.y, true));
            g.fillEllipse (e.expanded (radius * 1.6f));

            g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x - radius * 0.3f, c.y - radius * 0.4f,
                                                     juce::Colour (0xffc4c8cf), c.x + radius, c.y + radius, true));
            g.fillEllipse (e);
            g.setColour (juce::Colours::white);
            g.drawEllipse (e, 1.0f);
        }
        else
        {
            g.setColour (white (0.45f));
            g.fillEllipse (e);
            g.setColour (white (0.95f));
            g.drawEllipse (e, 1.0f);
        }
    }
}

juce::Font jostFont (float height, float kerning)
{
    static juce::Typeface::Ptr face = juce::Typeface::createSystemTypefaceFor (BinaryData::Jost_ttf,
                                                                              (size_t) BinaryData::Jost_ttfSize);
    juce::Font f = face != nullptr ? juce::Font (juce::FontOptions (face).withHeight (height))
                                   : juce::Font (juce::FontOptions (height));
    return kerning != 0.0f ? f.withExtraKerningFactor (kerning) : f;
}

// ------------------------------------------------------- StackLookAndFeel

void StackLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    auto track = juce::Rectangle<float> ((float) x, (float) y + (float) height * 0.5f - 4.0f, (float) width, 8.0f);

    g.setColour (white (0.5f));
    g.fillRoundedRectangle (track, 4.0f);
    g.setColour (juce::Colour::fromFloatRGBA (0.24f, 0.25f, 0.28f, 0.22f));
    g.drawLine (track.getX() + 3.0f, track.getY() + 1.5f, track.getRight() - 3.0f, track.getY() + 1.5f, 1.0f);
    g.setColour (white (0.9f));
    g.drawRoundedRectangle (track.reduced (0.5f), 4.0f, 1.0f);

    const float r = 11.0f;
    juce::Point<float> c (juce::jlimit ((float) x + r, (float) (x + width) - r, sliderPos), track.getCentreY());
    auto e = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);

    juce::Path p;
    p.addEllipse (e);
    juce::DropShadow (juce::Colour::fromFloatRGBA (0.24f, 0.25f, 0.28f, 0.4f), 8, { 0, 3 }).drawForPath (g, p);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x - 4.0f, c.y - 5.0f,
                                             juce::Colour (0xffd9dce1), c.x + r, c.y + r, true));
    g.fillEllipse (e);
    g.setColour (juce::Colours::white);
    g.drawEllipse (e, 1.0f);
}

void StackLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                             bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (4.0f);
    if (down)
        r = r.reduced (0.5f);

    juce::Path p;
    p.addRoundedRectangle (r, r.getHeight() * 0.5f);
    juce::DropShadow (juce::Colour::fromFloatRGBA (0.24f, 0.25f, 0.28f, 0.24f), 8, { 0, 3 }).drawForPath (g, p);

    g.setGradientFill (juce::ColourGradient (white (highlighted ? 1.0f : 0.95f), r.getX(), r.getY(),
                                             white (highlighted ? 0.75f : 0.5f), r.getRight(), r.getBottom(), false));
    g.fillPath (p);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
}

void StackLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (jostFont (13.0f, 0.06f));
    g.setColour (inkA (0.85f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

juce::Font StackLookAndFeel::getLabelFont (juce::Label&)
{
    return jostFont (12.0f, 0.04f);
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
    const int h = getHeight();
    const bool has = proc.hasPlugin (index);
    const bool on = has && ! proc.isBypassed (index);

    g.setFont (jostFont (11.0f));
    g.setColour (inkA (0.35f));
    g.drawText (juce::String (index + 1), 10, 0, 20, h, juce::Justification::centredRight);

    if (has)
    {
        drawOrb (g, { 44.0f, (float) h * 0.5f }, 6.0f, on);

        g.setFont (jostFont (14.0f, 0.02f));
        g.setColour (on ? inkA (0.85f) : inkA (0.35f));
        g.drawText (proc.getSlotName (index), 62, 0, getWidth() - 62 - 34, h,
                    juce::Justification::centredLeft, true);

        g.setFont (jostFont (18.0f));
        g.setColour (inkA (0.3f));
        g.drawText (juce::String::fromUTF8 ("\xc3\x97"), getWidth() - 32, 0, 22, h, juce::Justification::centred);
    }
    else
    {
        g.setFont (jostFont (14.0f, 0.02f));
        g.setColour (inkA (0.32f));
        g.drawText ("Empty", 62, 0, getWidth() - 70, h, juce::Justification::centredLeft);
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

    if (e.x >= getWidth() - 36)
        proc.removePlugin (index);
    else if (e.x >= 30 && e.x <= 58)
        proc.setBypassed (index, ! proc.isBypassed (index));
    else if (onOpen)
        onOpen (index);
}

// -------------------------------------------------------------- StackEditor

StackEditor::StackEditor (StackProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    for (int i = 0; i < StackProcessor::numSlots; ++i)
    {
        auto* r = rows.add (new SlotRow (proc, i));
        r->onOpen = [this] (int s) { openSlotEditor (s); };
        r->onLoad = [this] (int s) { showLoadMenu (s); };
        addAndMakeVisible (r);
    }

    addButton.onClick = [this]
    {
        for (int i = 0; i < StackProcessor::numSlots; ++i)
            if (! proc.hasPlugin (i))
            {
                showLoadMenu (i);
                return;
            }

        infoText = "All 20 slots are full";
        repaint();
    };
    addAndMakeVisible (addButton);

    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
    mixSlider.setColour (juce::Slider::textBoxTextColourId, inkA (0.5f));
    mixSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (mixSlider);
    mixAttach = std::make_unique<juce::SliderParameterAttachment> (*proc.mixParam, mixSlider, nullptr);

    const int h = topPad + headerH + 10 * (rowH + rowGap) + 2 + footH + 16 + mixH + 22;
    setSize (winW, h);

    proc.addListener (this);
}

StackEditor::~StackEditor()
{
    proc.removeListener (this);
    windows.clear();
    setLookAndFeel (nullptr);
}

void StackEditor::slotsChanged()
{
    if (pendingSelect >= 0 && proc.hasPlugin (pendingSelect))
    {
        selected = pendingSelect;
        infoText = proc.getSlotName (selected) + " loaded";
        pendingSelect = -1;
    }

    if (selected >= 0 && ! proc.hasPlugin (selected))
        selected = -1;

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

    selected = slot;
    infoText = "Slot " + juce::String (slot + 1) + "  /  " + proc.getSlotName (slot);
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

    menu.addItem (100000, "Browse...");
    menu.addItem (100001, "Rescan VST3 folders");

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
        chooser = std::make_unique<juce::FileChooser> ("Choose a VST3 plugin",
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
    infoText = "Loading " + juce::File (path).getFileNameWithoutExtension() + "...";
    pendingSelect = slot;
    repaint();

    juce::WeakReference<StackEditor> weak (this);
    proc.loadFromPath (slot, path, [weak] (const juce::String& err)
    {
        if (weak != nullptr)
        {
            weak->pendingSelect = -1;
            weak->infoText = "Failed: " + err;
            weak->repaint();
        }
    });
}

juce::Rectangle<int> StackEditor::getFooterBounds() const
{
    const int y = topPad + headerH + 10 * (rowH + rowGap) + 2;
    return { pad, y, getWidth() - pad * 2, footH };
}

void StackEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    // argent brosse
    juce::ColourGradient bg (juce::Colour (0xfff4f5f7), 0.0f, 0.0f, juce::Colour (0xffb7bbc2), w, h, false);
    bg.addColour (0.38, juce::Colour (0xffc9ccd2));
    bg.addColour (0.62, juce::Colour (0xffeceef1));
    g.setGradientFill (bg);
    g.fillAll();

    auto blob = [&g] (float cx, float cy, float r, juce::Colour c)
    {
        g.setGradientFill (juce::ColourGradient (c, cx, cy, c.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    };
    blob (60.0f, 70.0f, 230.0f, white (0.85f));
    blob (w - 20.0f, h * 0.45f, 190.0f, juce::Colour::fromFloatRGBA (0.47f, 0.49f, 0.53f, 0.32f));
    blob (w * 0.5f, h + 10.0f, 260.0f, white (0.7f));

    // en-tete
    g.setColour (inkA (0.8f));
    g.setFont (jostFont (20.0f, 0.42f));
    g.drawText ("STACK", pad + 4, topPad, 200, headerH, juce::Justification::centredLeft);

    const float ms = proc.getSampleRate() > 0.0
                         ? (float) proc.getTotalLatencySamples() * 1000.0f / (float) proc.getSampleRate()
                         : 0.0f;
    g.setColour (inkA (0.5f));
    g.setFont (jostFont (13.0f, 0.06f));
    g.drawText (juce::String (proc.getActiveCount()) + " active  " + juce::String::fromUTF8 ("\xc2\xb7")
                    + "  " + juce::String (ms, 1) + " ms",
                getWidth() - pad - 4 - 260, topPad, 260, headerH, juce::Justification::centredRight);

    // slots
    for (int i = 0; i < rows.size(); ++i)
    {
        auto r = rows[i]->getBounds().toFloat();
        const float corner = r.getHeight() * 0.5f;

        if (proc.hasPlugin (i))
        {
            drawGlass (g, r, corner);

            if (i == selected)
            {
                g.setColour (white (0.6f));
                g.drawRoundedRectangle (r.expanded (2.0f), corner + 2.0f, 3.0f);
            }
        }
        else
        {
            juce::Path p, dashed;
            p.addRoundedRectangle (r.reduced (0.5f), corner);
            g.setColour (white (0.12f));
            g.fillPath (p);
            const float dashes[] = { 4.0f, 3.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dashes, 2);
            g.setColour (white (0.9f));
            g.fillPath (dashed);
        }
    }

    // pied
    auto foot = getFooterBounds().toFloat();
    drawGlass (g, foot, foot.getHeight() * 0.5f);

    g.setColour (inkA (0.55f));
    g.setFont (jostFont (13.0f, 0.03f));
    g.drawText (infoText, getFooterBounds().withTrimmedLeft (22).withTrimmedRight (176),
                juce::Justification::centredLeft, true);

    g.setColour (inkA (0.5f));
    g.setFont (jostFont (12.0f, 0.1f));
    g.drawText ("DRY / WET", pad + 6, foot.getBottom() + 16, 100, mixH, juce::Justification::centredLeft);
}

void StackEditor::resized()
{
    const int colW = (getWidth() - pad * 2 - colGap) / 2;
    const int top = topPad + headerH;

    for (int i = 0; i < rows.size(); ++i)
    {
        const int col = i / 10;
        const int r = i % 10;
        rows[i]->setBounds (pad + col * (colW + colGap), top + r * (rowH + rowGap), colW, rowH);
    }

    auto foot = getFooterBounds();
    addButton.setBounds (foot.getRight() - 10 - 158, foot.getCentreY() - 21, 158, 42);

    const int my = foot.getBottom() + 16;
    mixSlider.setBounds (pad + 6 + 100, my, getWidth() - pad * 2 - 6 - 100 - 6, mixH);
}
