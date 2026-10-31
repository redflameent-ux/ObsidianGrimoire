cmake_minimum_required(VERSION 3.22)
project(ObsidianGrimoire VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE: use a local copy if present, otherwise download it ----
set(JUCE_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Optional local JUCE checkout")
if(EXISTS "${JUCE_LOCAL_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_LOCAL_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---- ORACLE: first plugin built on the shared RedFlame UI kit ----
set(ORACLE_SOURCES
    Source/oracle/PluginProcessor.cpp
    Source/oracle/PluginEditor.cpp)

set(ORACLE_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

set(ORACLE_LIBS
    juce::juce_audio_utils
    juce::juce_dsp)

juce_add_plugin(ORACLE
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.oracle
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Orcl
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "ORACLE"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

# the shared UI kit is header-only; expose it on the include path
target_include_directories(ORACLE PRIVATE Source)
target_sources(ORACLE PRIVATE ${ORACLE_SOURCES})
target_compile_definitions(ORACLE PUBLIC ${ORACLE_DEFS})
target_link_libraries(ORACLE
    PRIVATE ${ORACLE_LIBS}
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- SIGIL: second plugin on the same shared kit ----
juce_add_plugin(SIGIL
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.sigil
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Sigl
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "SIGIL"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(SIGIL PRIVATE Source)
target_sources(SIGIL PRIVATE Source/sigil/PluginProcessor.cpp Source/sigil/PluginEditor.cpp)
target_compile_definitions(SIGIL PUBLIC ${ORACLE_DEFS})
target_link_libraries(SIGIL
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- ETHEREAL: effect (galactic space reverb) on the same shared kit ----
juce_add_plugin(ETHEREAL
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.ethereal
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Ethr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "ETHEREAL"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Reverb
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(ETHEREAL PRIVATE Source)
target_sources(ETHEREAL PRIVATE Source/ethereal/PluginProcessor.cpp Source/ethereal/PluginEditor.cpp)
target_compile_definitions(ETHEREAL PUBLIC ${ORACLE_DEFS})
target_link_libraries(ETHEREAL
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- INCANT: groove generator (step-sequenced drum machine) on the same kit ----
juce_add_plugin(INCANT
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.incant
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Inct
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "INCANT"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(INCANT PRIVATE Source)
target_sources(INCANT PRIVATE Source/incant/PluginProcessor.cpp Source/incant/PluginEditor.cpp)
target_compile_definitions(INCANT PUBLIC ${ORACLE_DEFS})
target_link_libraries(INCANT
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- AETHER: sample-stack synth on the same kit ----
juce_add_plugin(AETHER
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.aether
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Aeth
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "AETHER"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(AETHER PRIVATE Source)
target_sources(AETHER PRIVATE Source/aether/PluginProcessor.cpp Source/aether/PluginEditor.cpp)
target_compile_definitions(AETHER PUBLIC ${ORACLE_DEFS})
target_link_libraries(AETHER
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- TORMENT: hi-fi lo-fi processor (effect) on the same kit ----
juce_add_plugin(TORMENT
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.torment
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Trmt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "TORMENT"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(TORMENT PRIVATE Source)
target_sources(TORMENT PRIVATE Source/torment/PluginProcessor.cpp Source/torment/PluginEditor.cpp)
target_compile_definitions(TORMENT PUBLIC ${ORACLE_DEFS})
target_link_libraries(TORMENT
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- FOUNDATION: saturation + loudness (effect) on the same kit ----
juce_add_plugin(FOUNDATION
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.foundation
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Fndn
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "FOUNDATION"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(FOUNDATION PRIVATE Source)
target_sources(FOUNDATION PRIVATE Source/foundation/PluginProcessor.cpp Source/foundation/PluginEditor.cpp)
target_compile_definitions(FOUNDATION PUBLIC ${ORACLE_DEFS})
target_link_libraries(FOUNDATION
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- CLEANSE: mastering / dynamic EQ (effect) on the same kit ----
juce_add_plugin(CLEANSE
    COMPANY_NAME "Red Flame Entertainment"
    BUNDLE_ID com.redflame.cleanse
    PLUGIN_MANUFACTURER_CODE RdFl
    PLUGIN_CODE Clns
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CLEANSE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_include_directories(CLEANSE PRIVATE Source)
target_sources(CLEANSE PRIVATE Source/cleanse/PluginProcessor.cpp Source/cleanse/PluginEditor.cpp)
target_compile_definitions(CLEANSE PUBLIC ${ORACLE_DEFS})
target_link_libraries(CLEANSE
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
