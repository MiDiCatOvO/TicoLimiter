#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

class PresetManager {
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvts)
        : mApvts(apvts)
    {
        // Preset directory: ~/Library/Audio/Presets/MiDiCat/Tico Limiter/
        mPresetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("Audio/Presets/MiDiCat/Tico Limiter");
        if (!mPresetDir.exists())
            mPresetDir.createDirectory();

        // Factory presets
        mFactoryPresets = {
            "Default",
            "Let's FIGHT",
        };

        refreshPresetList();
    }

    juce::StringArray getPresetNames() const { return mPresetNames; }

    int getNumPresets() const { return mPresetNames.size(); }

    juce::String getPresetName(int index) const {
        return juce::isPositiveAndBelow(index, mPresetNames.size()) ? mPresetNames[index] : juce::String();
    }

    // Load preset by name
    bool loadPreset(const juce::String& name) {
        if (name.isEmpty()) return false;

        // Factory presets
        if (name == "Default") {
            loadFactoryDefault();
            return true;
        }
        if (name == "Let's FIGHT") {
            loadLetsFight();
            return true;
        }

        // User presets from file
        auto file = mPresetDir.getChildFile(name + ".xml");
        if (file.existsAsFile()) {
            auto xml = juce::XmlDocument::parse(file);
            if (xml) {
                auto state = juce::ValueTree::fromXml(*xml);
                if (state.isValid()) {
                    mApvts.replaceState(state);
                    mCurrentPreset = name;
                    return true;
                }
            }
        }
        return false;
    }

    // Save current state as preset
    bool savePreset(const juce::String& name) {
        if (name.isEmpty()) return false;

        // Don't overwrite factory presets
        if (isFactoryPreset(name)) return false;

        auto state = mApvts.copyState();
        auto xml = state.createXml();
        auto file = mPresetDir.getChildFile(name + ".xml");
        bool ok = xml->writeTo(file);
        if (ok) {
            mCurrentPreset = name;
            refreshPresetList();
        }
        return ok;
    }

    // Delete a user preset
    bool deletePreset(const juce::String& name) {
        if (name.isEmpty() || isFactoryPreset(name)) return false;

        auto file = mPresetDir.getChildFile(name + ".xml");
        if (file.existsAsFile()) {
            bool ok = file.deleteFile();
            if (ok) {
                if (mCurrentPreset == name) mCurrentPreset.clear();
                refreshPresetList();
            }
            return ok;
        }
        return false;
    }

    juce::String getCurrentPreset() const { return mCurrentPreset; }
    void setCurrentPreset(const juce::String& name) { mCurrentPreset = name; }

    bool isFactoryPreset(const juce::String& name) const {
        return mFactoryPresets.contains(name);
    }

    int getPresetIndex(const juce::String& name) const {
        return mPresetNames.indexOf(name);
    }

private:
    void refreshPresetList() {
        mPresetNames.clear();
        mPresetNames.addArray(mFactoryPresets);

        // Scan user presets directory
        for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(mPresetDir, false, "*.xml")) {
            auto name = entry.getFile().getFileNameWithoutExtension();
            if (!mPresetNames.contains(name))
                mPresetNames.add(name);
        }
    }

    void loadFactoryDefault() {
        // Reset all parameters to defaults
        mApvts.getParameter("inputGain")->setValueNotifyingHost(mApvts.getParameter("inputGain")->convertTo0to1(0.0f));
        mApvts.getParameter("mix")->setValueNotifyingHost(mApvts.getParameter("mix")->convertTo0to1(100.0f));
        mApvts.getParameter("ceiling")->setValueNotifyingHost(mApvts.getParameter("ceiling")->convertTo0to1(0.0f));
        mApvts.getParameter("release")->setValueNotifyingHost(mApvts.getParameter("release")->convertTo0to1(100.0f));
        mApvts.getParameter("lookAhead")->setValueNotifyingHost(mApvts.getParameter("lookAhead")->convertTo0to1(0.0f));
        mApvts.getParameter("autoRelease")->setValueNotifyingHost(1.0f);
        mApvts.getParameter("truePeak")->setValueNotifyingHost(0.0f);
        mApvts.getParameter("saturationOn")->setValueNotifyingHost(0.0f);
        mApvts.getParameter("oddEvenMix")->setValueNotifyingHost(mApvts.getParameter("oddEvenMix")->convertTo0to1(50.0f));
        mApvts.getParameter("drive")->setValueNotifyingHost(mApvts.getParameter("drive")->convertTo0to1(0.0f));
        mApvts.getParameter("softClipOn")->setValueNotifyingHost(1.0f);
        mApvts.getParameter("ratio")->setValueNotifyingHost(mApvts.getParameter("ratio")->convertTo0to1(2.0f));
        mApvts.getParameter("oversampling")->setValueNotifyingHost(mApvts.getParameter("oversampling")->convertTo0to1(1.0f));
        mApvts.getParameter("sampleRate")->setValueNotifyingHost(mApvts.getParameter("sampleRate")->convertTo0to1(1.0f));
        mApvts.getParameter("tiltOn")->setValueNotifyingHost(1.0f);
        mApvts.getParameter("tiltAmount")->setValueNotifyingHost(mApvts.getParameter("tiltAmount")->convertTo0to1(0.0f));
        mApvts.getParameter("tiltMuddy")->setValueNotifyingHost(mApvts.getParameter("tiltMuddy")->convertTo0to1(0.0f));
        mApvts.getParameter("stereoLink")->setValueNotifyingHost(mApvts.getParameter("stereoLink")->convertTo0to1(100.0f));
        mCurrentPreset = "Default";
    }

    void loadGentleMaster() {
        loadFactoryDefault();
        mApvts.getParameter("inputGain")->setValueNotifyingHost(mApvts.getParameter("inputGain")->convertTo0to1(3.0f));
        mApvts.getParameter("ceiling")->setValueNotifyingHost(mApvts.getParameter("ceiling")->convertTo0to1(1.0f)); // -0.3 dB
        mApvts.getParameter("release")->setValueNotifyingHost(mApvts.getParameter("release")->convertTo0to1(200.0f));
        mApvts.getParameter("lookAhead")->setValueNotifyingHost(mApvts.getParameter("lookAhead")->convertTo0to1(5.0f));
        mApvts.getParameter("ratio")->setValueNotifyingHost(mApvts.getParameter("ratio")->convertTo0to1(0.0f)); // 1:2
        mApvts.getParameter("oversampling")->setValueNotifyingHost(mApvts.getParameter("oversampling")->convertTo0to1(2.0f)); // 8x
        mCurrentPreset = "Gentle Master";
    }

    void loadLetsFight() {
        loadFactoryDefault();
        mApvts.getParameter("inputGain")->setValueNotifyingHost(mApvts.getParameter("inputGain")->convertTo0to1(6.0f));
        mApvts.getParameter("ceiling")->setValueNotifyingHost(mApvts.getParameter("ceiling")->convertTo0to1(0.0f)); // -0.1 dB
        mApvts.getParameter("release")->setValueNotifyingHost(mApvts.getParameter("release")->convertTo0to1(80.0f));
        mApvts.getParameter("lookAhead")->setValueNotifyingHost(mApvts.getParameter("lookAhead")->convertTo0to1(3.0f));
        mApvts.getParameter("ratio")->setValueNotifyingHost(mApvts.getParameter("ratio")->convertTo0to1(1.0f)); // 1:3
        mApvts.getParameter("saturationOn")->setValueNotifyingHost(1.0f);
        mApvts.getParameter("drive")->setValueNotifyingHost(mApvts.getParameter("drive")->convertTo0to1(20.0f));
        mApvts.getParameter("tiltOn")->setValueNotifyingHost(1.0f);
        mApvts.getParameter("tiltAmount")->setValueNotifyingHost(mApvts.getParameter("tiltAmount")->convertTo0to1(3.0f));
        mApvts.getParameter("oversampling")->setValueNotifyingHost(mApvts.getParameter("oversampling")->convertTo0to1(2.0f)); // 8x
        mApvts.getParameter("truePeak")->setValueNotifyingHost(1.0f);
        mCurrentPreset = "Let's FIGHT";
    }

    juce::AudioProcessorValueTreeState& mApvts;
    juce::File mPresetDir;
    juce::StringArray mFactoryPresets;
    juce::StringArray mPresetNames;
    juce::String mCurrentPreset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
