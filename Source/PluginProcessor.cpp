#include "PluginProcessor.h"
#include "PluginEditor.h"

StackProcessor::StackProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.addDefaultFormats();
    addParameter (mixParam = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 },
                                                            "Dry/Wet", 0.0f, 1.0f, 1.0f));
}

StackProcessor::~StackProcessor()
{
    const juce::ScopedLock sl (lock);
    for (auto& s : slots)
    {
        if (s.plugin != nullptr)
            s.plugin->releaseResources();
        s.plugin.reset();
    }
}

void StackProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSR = sampleRate;
    currentBlock = samplesPerBlock;

    work.setSize (8, samplesPerBlock);
    dryBuffer.setSize (2, samplesPerBlock);
    dryDelay.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    dryDelay.reset();

    const juce::ScopedLock sl (lock);
    for (auto& s : slots)
        if (s.plugin != nullptr)
        {
            s.plugin->setPlayConfigDetails (2, 2, sampleRate, samplesPerBlock);
            s.plugin->prepareToPlay (sampleRate, samplesPerBlock);
        }
}

void StackProcessor::releaseResources()
{
    const juce::ScopedLock sl (lock);
    for (auto& s : slots)
        if (s.plugin != nullptr)
            s.plugin->releaseResources();
}

bool StackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void StackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (2, buffer.getNumChannels());

    for (int c = chans; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    if (chans < 2 || n > work.getNumSamples())
        return;

    const float mix = mixParam->get();
    const bool needDry = mix < 0.999f;

    const juce::ScopedLock sl (lock);

    if (needDry)
        for (int c = 0; c < 2; ++c)
            dryBuffer.copyFrom (c, 0, buffer, c, 0, n);

    juce::MidiBuffer emptyMidi;

    for (auto& s : slots)
    {
        if (s.plugin == nullptr || s.bypass)
            continue;

        for (int c = 0; c < 2; ++c)
            work.copyFrom (c, 0, buffer, c, 0, n);

        juce::AudioBuffer<float> view (work.getArrayOfWritePointers(), 2, n);
        emptyMidi.clear();
        s.plugin->processBlock (view, emptyMidi);

        for (int c = 0; c < 2; ++c)
            buffer.copyFrom (c, 0, work, c, 0, n);
    }

    if (needDry)
    {
        const float delay = (float) juce::jlimit (0, 191999, totalLatency.load());
        dryDelay.setDelay (delay);

        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
                dryDelay.pushSample (c, dryBuffer.getSample (c, i));

            for (int c = 0; c < 2; ++c)
            {
                const float d = dryDelay.popSample (c, delay, true);
                const float w = buffer.getSample (c, i);
                buffer.setSample (c, i, d * (1.0f - mix) + w * mix);
            }
        }
    }
}

juce::AudioProcessorEditor* StackProcessor::createEditor()
{
    return new StackEditor (*this);
}

// ---------------------------------------------------------------- slots

bool StackProcessor::hasPlugin (int slot) const
{
    return juce::isPositiveAndBelow (slot, numSlots) && slots[(size_t) slot].plugin != nullptr;
}

juce::String StackProcessor::getSlotName (int slot) const
{
    return hasPlugin (slot) ? slots[(size_t) slot].desc.name : juce::String();
}

bool StackProcessor::isBypassed (int slot) const
{
    return juce::isPositiveAndBelow (slot, numSlots) && slots[(size_t) slot].bypass;
}

void StackProcessor::setBypassed (int slot, bool shouldBypass)
{
    if (! hasPlugin (slot))
        return;

    {
        const juce::ScopedLock sl (lock);
        slots[(size_t) slot].bypass = shouldBypass;
    }
    updateLatency();
    listeners.call (&Listener::slotsChanged);
}

juce::AudioPluginInstance* StackProcessor::getPlugin (int slot) const
{
    return juce::isPositiveAndBelow (slot, numSlots) ? slots[(size_t) slot].plugin.get() : nullptr;
}

int StackProcessor::getActiveCount() const
{
    int n = 0;
    for (auto& s : slots)
        if (s.plugin != nullptr && ! s.bypass)
            ++n;
    return n;
}

juce::StringArray StackProcessor::scanVst3Paths()
{
    juce::StringArray result;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* f = formatManager.getFormat (i);
        if (f->getName() == "VST3")
            result.addArray (f->searchPathsForPlugins (f->getDefaultLocationsToSearch(), true, false));
    }

    juce::StringArray filtered;
    for (auto& p : result)
        if (juce::File (p).getFileNameWithoutExtension() != "STACK")
            filtered.addIfNotAlreadyThere (p);

    filtered.sort (true);
    return filtered;
}

void StackProcessor::loadFromPath (int slot, const juce::String& path,
                                   std::function<void (const juce::String&)> onError)
{
    if (! juce::isPositiveAndBelow (slot, numSlots))
        return;

    juce::OwnedArray<juce::PluginDescription> found;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* f = formatManager.getFormat (i);
        if (f->fileMightContainThisPluginType (path))
        {
            f->findAllTypesForFile (found, path);
            if (found.size() > 0)
                break;
        }
    }

    if (found.isEmpty())
    {
        if (onError)
            onError ("plugin not found or could not be loaded");
        return;
    }

    loadInstance (slot, *found[0], {}, false, std::move (onError));
}

void StackProcessor::loadInstance (int slot, const juce::PluginDescription& desc,
                                   const juce::String& stateBase64, bool bypass,
                                   std::function<void (const juce::String&)> onError)
{
    formatManager.createPluginInstanceAsync (
        desc, currentSR, currentBlock,
        [this, slot, desc, stateBase64, bypass, onError] (std::unique_ptr<juce::AudioPluginInstance> inst,
                                                         const juce::String& error)
        {
            if (inst == nullptr)
            {
                if (onError)
                    onError (error);
                return;
            }

            auto layout = inst->getBusesLayout();
            if (layout.inputBuses.size() > 0)
                layout.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();
            if (layout.outputBuses.size() > 0)
                layout.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
            for (int i = 1; i < layout.inputBuses.size(); ++i)
                layout.inputBuses.getReference (i) = juce::AudioChannelSet::disabled();
            for (int i = 1; i < layout.outputBuses.size(); ++i)
                layout.outputBuses.getReference (i) = juce::AudioChannelSet::disabled();
            inst->setBusesLayout (layout);

            inst->setPlayConfigDetails (2, 2, currentSR, currentBlock);
            inst->prepareToPlay (currentSR, currentBlock);

            if (stateBase64.isNotEmpty())
            {
                juce::MemoryBlock mb;
                if (mb.fromBase64Encoding (stateBase64))
                    inst->setStateInformation (mb.getData(), (int) mb.getSize());
            }

            listeners.call (&Listener::slotWillBeRemoved, slot);

            std::unique_ptr<juce::AudioPluginInstance> old;
            {
                const juce::ScopedLock sl (lock);
                old = std::move (slots[(size_t) slot].plugin);
                slots[(size_t) slot].plugin = std::move (inst);
                slots[(size_t) slot].desc = desc;
                slots[(size_t) slot].bypass = bypass;
            }

            if (old != nullptr)
                old->releaseResources();
            old.reset();

            updateLatency();
            listeners.call (&Listener::slotsChanged);
        });
}

void StackProcessor::removePlugin (int slot)
{
    if (! hasPlugin (slot))
        return;

    listeners.call (&Listener::slotWillBeRemoved, slot);

    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (lock);
        old = std::move (slots[(size_t) slot].plugin);
        slots[(size_t) slot].desc = {};
        slots[(size_t) slot].bypass = false;
    }

    if (old != nullptr)
        old->releaseResources();
    old.reset();

    updateLatency();
    listeners.call (&Listener::slotsChanged);
}

void StackProcessor::updateLatency()
{
    int total = 0;
    {
        const juce::ScopedLock sl (lock);
        for (auto& s : slots)
            if (s.plugin != nullptr && ! s.bypass)
                total += s.plugin->getLatencySamples();
    }

    totalLatency.store (total);
    setLatencySamples (total);
}

// ---------------------------------------------------------------- state

void StackProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("STACKSTATE");
    xml.setAttribute ("mix", (double) mixParam->get());

    const juce::ScopedLock sl (lock);
    for (int i = 0; i < numSlots; ++i)
    {
        auto& s = slots[(size_t) i];
        if (s.plugin == nullptr)
            continue;

        auto* e = xml.createNewChildElement ("SLOT");
        e->setAttribute ("index", i);
        e->setAttribute ("bypass", s.bypass);

        if (auto d = s.desc.createXml())
            e->addChildElement (d.release());

        juce::MemoryBlock mb;
        s.plugin->getStateInformation (mb);
        e->setAttribute ("state", mb.toBase64Encoding());
    }

    copyXmlToBinary (xml, destData);
}

void StackProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("STACKSTATE"))
        return;

    *mixParam = (float) xml->getDoubleAttribute ("mix", 1.0);

    for (int i = 0; i < numSlots; ++i)
        removePlugin (i);

    for (auto* e : xml->getChildWithTagNameIterator ("SLOT"))
    {
        const int idx = e->getIntAttribute ("index", -1);
        if (! juce::isPositiveAndBelow (idx, numSlots))
            continue;

        if (auto* de = e->getFirstChildElement())
        {
            juce::PluginDescription d;
            if (d.loadFromXml (*de))
                loadInstance (idx, d, e->getStringAttribute ("state"), e->getBoolAttribute ("bypass"), nullptr);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StackProcessor();
}
