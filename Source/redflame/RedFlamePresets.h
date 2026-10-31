#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RedFlameTheme.h"

namespace rf
{
    // set an APVTS parameter to a plain (denormalised) value
    inline void setP (juce::AudioProcessorValueTreeState& s, const juce::String& id, float value)
    {
        if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value));
    }

    // Shared preset browser. Factory presets are code (apply lambdas); user presets are
    // binary state files (the processor's own get/setStateInformation). Drives the
    // top-bar preset ComboBox + prev/next/save buttons.
    class PresetManager
    {
    public:
        struct Factory { juce::String name; std::function<void()> apply; };

        PresetManager (juce::AudioProcessor& p, std::vector<Factory> factoryPresets)
            : proc (p), factory (std::move (factoryPresets))
        {
            dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("Red Flame Entertainment")
                    .getChildFile (proc.getName())
                    .getChildFile ("Presets");
            dir.createDirectory();
            scanUser();
        }

        void attach (juce::ComboBox& box, juce::Button& prev, juce::Button& next, juce::Button& save)
        {
            combo = &box;
            rebuild();
            box.onChange = [this] { if (! building && combo->getSelectedId() > 0) select (combo->getSelectedId()); };
            prev.onClick = [this] { step (-1); };
            next.onClick = [this] { step (+1); };
            save.onClick = [this] { promptSave(); };
            if (! factory.empty()) { current = 1; combo->setSelectedId (1, juce::dontSendNotification); }
        }

    private:
        void scanUser() { userFiles = dir.findChildFiles (juce::File::findFiles, false, "*.rfpreset"); }

        void rebuild()
        {
            if (combo == nullptr) return;
            building = true;
            combo->clear (juce::dontSendNotification);
            int id = 1; selectable.clear();
            if (! factory.empty())
            {
                combo->addSectionHeading ("FACTORY");
                for (auto& f : factory) { combo->addItem (f.name, id); selectable.push_back (id); ++id; }
            }
            if (! userFiles.isEmpty())
            {
                combo->addSeparator();
                combo->addSectionHeading ("USER");
                for (auto& f : userFiles) { combo->addItem (f.getFileNameWithoutExtension(), id); selectable.push_back (id); ++id; }
            }
            if (current > 0) combo->setSelectedId (current, juce::dontSendNotification);
            building = false;
        }

        void select (int id)
        {
            current = id;
            const int fi = id - 1;
            if (fi >= 0 && fi < (int) factory.size()) { factory[(size_t) fi].apply(); return; }
            const int ui = fi - (int) factory.size();
            if (ui >= 0 && ui < userFiles.size())
            {
                juce::MemoryBlock mb;
                if (userFiles[ui].loadFileAsData (mb)) proc.setStateInformation (mb.getData(), (int) mb.getSize());
            }
        }

        void step (int d)
        {
            if (selectable.empty()) return;
            int idx = 0;
            for (int i = 0; i < (int) selectable.size(); ++i) if (selectable[(size_t) i] == current) { idx = i; break; }
            idx = (idx + d + (int) selectable.size()) % (int) selectable.size();
            const int id = selectable[(size_t) idx];
            if (combo != nullptr) combo->setSelectedId (id, juce::dontSendNotification);
            select (id);
        }

        void promptSave()
        {
            auto* w = new juce::AlertWindow ("Save Preset", "Name this preset:", juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", "My Preset");
            w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
            {
                const juce::String name = w->getTextEditorContents ("name").trim();
                delete w;
                if (r == 1 && name.isNotEmpty()) saveUser (name);
            }), false);
        }

        void saveUser (const juce::String& name)
        {
            juce::MemoryBlock mb; proc.getStateInformation (mb);
            auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".rfpreset");
            file.replaceWithData (mb.getData(), mb.getSize());
            scanUser();
            // select the just-saved preset
            for (int i = 0; i < userFiles.size(); ++i)
                if (userFiles[i] == file) { current = (int) factory.size() + 1 + i; break; }
            rebuild();
        }

        juce::AudioProcessor& proc;
        std::vector<Factory> factory;
        juce::File dir;
        juce::Array<juce::File> userFiles;
        juce::ComboBox* combo = nullptr;
        std::vector<int> selectable;
        int current = 0;
        bool building = false;
    };
}
