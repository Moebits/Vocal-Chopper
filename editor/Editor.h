#pragma once
#pragma clang diagnostic ignored "-Wshadow-field"
#include <JuceHeader.h>
#include "Processor.h"

class PythonThread;

class Editor : public AudioProcessorEditor {
public:
    Editor(Processor& p);
    ~Editor() override {}
    
    auto resized() -> void override;

    auto webviewOptions() -> WebBrowserComponent::Options;
    auto getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte>;
    auto getResource(const String& url) -> std::optional<WebBrowserComponent::Resource>;

    auto getState(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto selectAudio(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto selectDest(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto updateSkipVocalExtraction(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto updateKeepVocalFile(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto startProcessing(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;
    
    auto openFolder(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto showFileInFolder(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto fileDropped(const juce::var& eventData) -> void;
    
    auto deleteThread() -> void;

    Processor& processor;
    WebBrowserComponent webview;
        
private:
    ComponentBoundsConstrainer constrainer;
    PythonThread* pythonThread = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};