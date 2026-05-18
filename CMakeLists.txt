cmake_minimum_required(VERSION 3.22)

project(WetDiaper VERSION 0.1.0)

add_subdirectory(JUCE)

# --- DSP Library (shared between plugin and tests) ---
add_library(WetDiaperDSP STATIC
    Source/DSP/Overdrive.cpp
)

target_include_directories(WetDiaperDSP PUBLIC Source)

target_link_libraries(WetDiaperDSP
    PUBLIC
        juce::juce_dsp
)

target_compile_definitions(WetDiaperDSP
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# --- Plugin ---
juce_add_plugin(WetDiaper
    COMPANY_NAME "wetdiaper"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Wdpr
    PLUGIN_CODE Wdpr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "wet-diaper"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(WetDiaper
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/KnobDesign.h
)

juce_add_binary_data(WetDiaperBinaryData
    SOURCES
        Assets/conjius-avatar-transparent-bg.png
        Assets/wet-diaper-logo-transparent-bg.png
        Assets/Inconsolata-Bold.ttf
        Assets/Inconsolata-Regular.ttf)

target_compile_definitions(WetDiaper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(WetDiaper
    PRIVATE
        WetDiaperDSP
        WetDiaperBinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Aggressive binary-size reduction ──
target_compile_options(WetDiaper PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -ffunction-sections
        -fdata-sections>)

target_link_options(WetDiaper PRIVATE
    $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip -Wl,-x>
    $<$<PLATFORM_ID:Linux>:-Wl,--gc-sections -Wl,--strip-all>)

if(MSVC)
    target_link_options(WetDiaper PRIVATE /OPT:REF /OPT:ICF)
endif()

# Strip the final plugin binary after build (Release only).
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR NOT CMAKE_BUILD_TYPE)
    foreach(kind VST3 AU Standalone)
        if(TARGET WetDiaper_${kind})
            if(APPLE)
                add_custom_command(TARGET WetDiaper_${kind} POST_BUILD
                    COMMAND ${CMAKE_STRIP} -x -S
                        $<TARGET_FILE:WetDiaper_${kind}>
                    COMMAND codesign --force --sign - --timestamp=none
                        $<TARGET_BUNDLE_DIR:WetDiaper_${kind}>
                    COMMENT "Stripping + re-signing WetDiaper_${kind}")
            elseif(UNIX)
                add_custom_command(TARGET WetDiaper_${kind} POST_BUILD
                    COMMAND ${CMAKE_STRIP} --strip-all
                        $<TARGET_FILE:WetDiaper_${kind}>
                    COMMENT "Stripping WetDiaper_${kind}")
            endif()
        endif()
    endforeach()
endif()

# --- Tests ---
add_executable(WetDiaperTests
    Tests/TestOverdrive.cpp
)

target_include_directories(WetDiaperTests PRIVATE Source)

target_link_libraries(WetDiaperTests
    PRIVATE
        WetDiaperDSP
        juce::juce_core
)

target_compile_definitions(WetDiaperTests
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
)

# --- Screenshot CLI ---
add_executable(WetDiaperScreenshot
    Tools/Screenshot.cpp
)

target_include_directories(WetDiaperScreenshot PRIVATE Source)

target_link_libraries(WetDiaperScreenshot
    PRIVATE
        WetDiaper
        WetDiaperBinaryData
        juce::juce_audio_utils
        juce::juce_gui_basics
)
