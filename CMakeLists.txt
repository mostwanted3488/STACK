cmake_minimum_required(VERSION 3.22)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(STACK VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(STACK
    COMPANY_NAME "Underwater Palace"
    PLUGIN_MANUFACTURER_CODE Uwpl
    PLUGIN_CODE Stck
    FORMATS VST3
    PRODUCT_NAME "STACK"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(JOST_TTF "${CMAKE_BINARY_DIR}/fonts/Jost.ttf")
if(NOT EXISTS "${JOST_TTF}")
    file(DOWNLOAD "https://github.com/google/fonts/raw/main/ofl/jost/Jost%5Bwght%5D.ttf" "${JOST_TTF}"
         STATUS JOST_STATUS TLS_VERIFY ON)
    list(GET JOST_STATUS 0 JOST_CODE)
    file(SIZE "${JOST_TTF}" JOST_SIZE)
    if(NOT JOST_CODE EQUAL 0 OR JOST_SIZE LESS 20000)
        file(REMOVE "${JOST_TTF}")
        message(FATAL_ERROR "Telechargement de la police Jost impossible: ${JOST_STATUS}")
    endif()
endif()
juce_add_binary_data(StackAssets SOURCES "${JOST_TTF}")

target_sources(STACK PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(STACK PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(STACK
    PRIVATE
        StackAssets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
