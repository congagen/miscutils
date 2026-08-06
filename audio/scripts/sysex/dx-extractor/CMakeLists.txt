cmake_minimum_required(VERSION 3.22)
project(dx-extractor VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress SDK deprecation errors from breaking juceaide bootstrap
if(APPLE)
    add_compile_options(-Wno-deprecated-declarations)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_console_app(dx-extractor PRODUCT_NAME "dx-extractor")

target_sources(dx-extractor PRIVATE src/main.cpp)

target_link_libraries(dx-extractor PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_core
    "-framework AudioUnit"
    "-framework CoreAudioKit"
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework Cocoa"
)

target_compile_definitions(dx-extractor PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)