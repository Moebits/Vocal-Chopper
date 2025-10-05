#include "Processor.h"
#include "Editor.h"
#include "Functions.hpp"
#include "Settings.hpp"
#include "BinaryData.h"
#include "PythonThread.hpp"

Editor::Editor(Processor& p) : AudioProcessorEditor(&p), processor(p),
    webview(webviewOptions()) {
    this->webview.goToURL(this->webview.getResourceProviderRoot());

    int width = static_cast<int>(Settings::getSettingKey("windowWidth", 650));
    int height = static_cast<int>(Settings::getSettingKey("windowHeight", 500));
    float aspectRatio = static_cast<float>(width) / height;

    int minWidth = 240;
    int minHeight = static_cast<int>(minWidth / aspectRatio); 

    this->constrainer.setFixedAspectRatio(aspectRatio);
    this->constrainer.setMinimumSize(minWidth, minHeight);
    this->constrainer.setMaximumSize(10000, 10000);
    
    this->setConstrainer(&this->constrainer);
    this->setResizable(true, true);
    this->setSize(width, height);

    this->addAndMakeVisible(this->webview);
}

auto Editor::webviewOptions() -> WebBrowserComponent::Options {
    return WebBrowserComponent::Options{}
    .withBackend(WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}
    .withUserDataFolder(File::getSpecialLocation(File::tempDirectory)))
    .withResourceProvider([this](const auto& url) { return getResource(url); })
    .withNativeIntegrationEnabled()
    .withKeepPageLoadedWhenBrowserIsHidden()
    .withNativeFunction("getState", [this](auto args, auto completion){ 
        return this->getState(args, completion);
    })
    .withNativeFunction("selectAudio", [this](auto args, auto completion){ 
        return this->selectAudio(args, completion);
    })
    .withNativeFunction("selectDest", [this](auto args, auto completion){ 
        return this->selectDest(args, completion);
    })
    .withNativeFunction("updateSkipVocalExtraction", [this](auto args, auto completion){ 
        return this->updateSkipVocalExtraction(args, completion);
    })
    .withNativeFunction("updateKeepVocalFile", [this](auto args, auto completion){ 
        return this->updateKeepVocalFile(args, completion);
    })
    .withNativeFunction("startProcessing", [this](auto args, auto completion){ 
        return this->startProcessing(args, completion);
    })
    .withNativeFunction("openFolder", [this](auto args, auto completion){ 
        return this->openFolder(args, completion);
    })
    .withNativeFunction("showFileInFolder", [this](auto args, auto completion){ 
        return this->showFileInFolder(args, completion);
    })
    .withEventListener("file-dropped", [this](const var& eventData) {
        return this->fileDropped(eventData);
    });
}

auto Editor::resized() -> void {
    webview.setBounds(this->getLocalBounds());
    Settings::setSettingKey("windowWidth", this->getWidth());
    Settings::setSettingKey("windowHeight", this->getHeight());
}

auto Editor::getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte> {
    MemoryInputStream zipStream(BinaryData::webview_files_zip, BinaryData::webview_files_zipSize, false);
    ZipFile zip{zipStream};

    if (auto* entry = zip.getEntry(resourceStr)) {
        std::unique_ptr<InputStream> entryStream{zip.createStreamForEntry(*entry)};
        if (entryStream == nullptr) {
            jassertfalse;
            return {};
        }
        return Functions::streamToVector(*entryStream);
    }
    return {};
}

auto Editor::getResource(const String& url) -> std::optional<WebBrowserComponent::Resource> {
    static auto fileRoot = File::getCurrentWorkingDirectory().getChildFile("dist");
    auto resourceStr = url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);
    auto ext = resourceStr.fromLastOccurrenceOf(".", false, false);

    #if WEBVIEW_DEV_MODE
        auto stream = fileRoot.getChildFile(resourceStr).createInputStream();
        if (stream) {
            return WebBrowserComponent::Resource(Functions::streamToVector(*stream), Functions::getMimeForExtension(ext));
        }
    #else
        auto resource = Editor::getWebviewFileBytes(resourceStr);
        if (!resource.empty()) {
            return WebBrowserComponent::Resource(std::move(resource), Functions::getMimeForExtension(ext));
        }
    #endif
    return std::nullopt;
}

auto Editor::getState([[maybe_unused]] const Array<var>& args, WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    auto* obj = new DynamicObject();
    obj->setProperty("audioPath", this->processor.audioPath);
    obj->setProperty("destPath", this->processor.destFolder);
    obj->setProperty("skipVocalExtraction", this->processor.skipVocalExtraction);
    obj->setProperty("keepVocalFile", this->processor.keepVocalFile);
    obj->setProperty("state", this->processor.state);
    obj->setProperty("progress", this->processor.progress);
    completion(var{obj});
}

auto Editor::selectAudio([[maybe_unused]] const Array<var>& args, WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    auto directoryPath = Settings::getSettingKey("selectAudioDirectory", Functions::getDownloadsFolder().getFullPathName());
    File directory{directoryPath};

    auto* selectAudioDialog = new FileChooser{
        "Select Audio", directory, "*.wav; *.mp3; *.ogg; *.flac"
    };

    selectAudioDialog->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this, completion, selectAudioDialog](const FileChooser& picker) {
            auto file = picker.getResult();
            if (file.existsAsFile()) {
                Settings::setSettingKey("selectAudioDirectory", file.getParentDirectory().getFullPathName());
                this->processor.audioPath = file.getFullPathName();
                if (!this->processor.droppedFileBytes.isEmpty()) {
                    this->processor.droppedFileBytes.reset();
                }
                completion(this->processor.audioPath);
            }
            delete selectAudioDialog;
        }
    );
}

auto Editor::selectDest([[maybe_unused]] const Array<var>& args, WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    auto directoryPath = Settings::getSettingKey("selectDestDirectory", Functions::getDownloadsFolder().getFullPathName());
    File directory{directoryPath};

    auto* selectDestDialog = new FileChooser{
        "Select Dest", directory
    };

    selectDestDialog->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
        [this, completion, selectDestDialog](const FileChooser& picker) {
            auto folder = picker.getResult();
            if (folder.isDirectory()) {
                Settings::setSettingKey("selectDestDirectory", folder.getParentDirectory().getFullPathName());
                this->processor.destFolder = folder.getFullPathName();
                completion(this->processor.destFolder);
            }
            delete selectDestDialog;
        }
    );
}

auto Editor::updateSkipVocalExtraction(const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    bool value = static_cast<bool>(args[0]);
    this->processor.skipVocalExtraction = value;
}

auto Editor::updateKeepVocalFile(const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    bool value = static_cast<bool>(args[0]);
    this->processor.keepVocalFile = value;
}

auto Editor::deleteThread() -> void {
    if (this->pythonThread != nullptr) {
        this->pythonThread->stopThread(-1);
        delete this->pythonThread;
        this->pythonThread = nullptr;
    }
}

auto Editor::startProcessing([[maybe_unused]] const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    if (this->pythonThread != nullptr) {
        this->processor.state = "finished";
        this->processor.progress = 100;
        this->webview.emitEventIfBrowserIsVisible(Identifier{"state-changed"}, "finished");
        this->webview.emitEventIfBrowserIsVisible(Identifier{"progress"}, 100);
        this->pythonThread->process.kill();
        return;
    }
    this->pythonThread = new PythonThread{*this, this->processor};
    this->pythonThread->startThread();
}

auto Editor::openFolder(const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    auto folderPath = args[0].toString().trim();
    if (folderPath.isEmpty()) return;

    File folder{folderPath};
    if (folder.isDirectory()) {
        folder.startAsProcess();
    }
}

auto Editor::showFileInFolder(const Array<var>& args, 
    [[maybe_unused]] WebBrowserComponent::NativeFunctionCompletion completion) -> void {
    auto filePath = args[0].toString().trim();
    if (filePath.isEmpty()) return;

    File file{filePath};
    if (!file.existsAsFile()) return;

    #if JUCE_MAC
        ChildProcess process;
        process.start({"open", "-R", file.getFullPathName()});
    #elif JUCE_WINDOWS
        auto cmd = "explorer /select,\"" + file.getFullPathName() + "\"";
        system(cmd.toRawUTF8());
    #else
        auto parentFolder = file.getParentDirectory();
        parentFolder.startAsProcess();
    #endif
}

auto Editor::fileDropped(const var& eventData) -> void {
    auto* obj = eventData.getDynamicObject();
    auto name = obj->getProperty("name");
    auto data = obj->getProperty("data");

    MemoryBlock binaryData;
    for (auto& value : *data.getArray()) {
        uint8_t byte = static_cast<uint8_t>(static_cast<int>(value));
        binaryData.append(&byte, 1);
    }
    this->processor.audioPath = "[dropped file] " + name.toString();
    this->processor.droppedFileBytes = std::move(binaryData);
}