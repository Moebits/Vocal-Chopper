#pragma once
#include <JuceHeader.h>
#include <regex>
#include "BinaryData.h"
#include "Editor.h"

class PythonThread : public Thread {
public:
    PythonThread(Editor& editor, Processor& processor) : Thread("Python Thread"), editor(editor), processor(processor) {}

    auto run() -> void override {
        auto audioPath = this->processor.audioPath;
        auto destFolder = this->processor.destFolder;
        File vocalFolder{destFolder};
        File vocalFile = vocalFolder.getChildFile("vocals.wav");
        auto vocalPath = vocalFile.getFullPathName();
        bool skipVocalExtraction = this->processor.skipVocalExtraction;
        bool keepVocalFile = this->processor.keepVocalFile;
        auto droppedFileBytes = this->processor.droppedFileBytes;
        bool isTempPath = false;
    
        File tempScript = File::getSpecialLocation(File::tempDirectory).getChildFile("chopper.py");
        tempScript.replaceWithData(BinaryData::chopper_py, BinaryData::chopper_pySize);

        if (audioPath.contains("[dropped file]") && !droppedFileBytes.isEmpty()) {
            auto name = audioPath.fromFirstOccurrenceOf("]", false, false).trim();
            File tempAudioFile = File::getSpecialLocation(File::tempDirectory).getChildFile(name);
            tempAudioFile.replaceWithData(droppedFileBytes.getData(), droppedFileBytes.getSize());
            audioPath = tempAudioFile.getFullPathName();
            isTempPath = true;
        }
    
        String pythonPath;
        #if JUCE_MAC
            pythonPath = "/usr/local/bin/python3";
        #else
            pythonPath = "python3";
        #endif

        char buffer[128];
    
        if (!skipVocalExtraction) {
            this->processor.state = "separating";
            this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"state-changed"}, "separating");
            StringArray argv{pythonPath, tempScript.getFullPathName(), "--separate", "-i", audioPath, "-o", vocalPath};

            if (process.start(argv)) {
                while (process.isRunning()) {
                    int read = process.readProcessOutput(buffer, sizeof(buffer));
                    if (read > 0) {
                        auto output = String::createStringFromData(buffer, read).toStdString();
                        std::regex regex{"(\\d+)%"};
                        std::smatch match;
    
                        if (std::regex_search(output, match, regex)) {
                            double percent = std::stod(match[1].str());
                            if (percent == 100.0) percent = 99.0;
                            this->processor.progress = percent;
                            this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"progress"}, percent);
                        }
                    }
                    Thread::sleep(50);
                }
                this->processor.progress = 100;
                this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"progress"}, 100);
            }
        } else {
            vocalPath = audioPath;
        }
        
        this->processor.state = "chopping";
        this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"state-changed"}, "chopping");
        StringArray argv2{pythonPath, tempScript.getFullPathName(), "--chop", "-i", vocalPath, "-o", destFolder, "-n", audioPath};

        char buffer2[256];

        if (process.start(argv2)) {
            while (process.isRunning()) {
                int read = process.readProcessOutput(buffer2, sizeof(buffer2));
                if (read > 0) {
                    auto output = String::createStringFromData(buffer2, read).toStdString();
                    std::regex regex{"(\\d+/\\d+)"};
                    std::smatch match;

                    if (std::regex_search(output, match, regex)) {
                        auto parts = StringArray::fromTokens(match[1].str(), "/", "");
                        double current = parts[0].getDoubleValue();
                        double total = parts[1].getDoubleValue();
                        double percent = (current / total) * 100.0;
                        if (percent == 100.0) percent = 99.0;
                        this->processor.progress = percent;
                        this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"progress"}, percent);
                    }
                }
                Thread::sleep(50);
            }
            this->processor.progress = 100;
            this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"progress"}, 100);
        }

        File audioFile{audioPath};
        auto outputBaseName = audioFile.getFileNameWithoutExtension();
        auto outputDir = File{destFolder}.getChildFile(outputBaseName + " chops");
        if (outputDir.isDirectory()) outputDir.startAsProcess();

        if (!skipVocalExtraction) {
            File vocalFile{vocalPath};
            if (keepVocalFile && vocalFile.existsAsFile()) {
                auto newDest = outputDir.getChildFile(vocalFile.getFileName());
                vocalFile.moveFileTo(newDest);
            } else {
                if (vocalFile.existsAsFile()) vocalFile.deleteFile();
            }
        }

        if (isTempPath) {
            File tempFile{audioPath};
            if (tempFile.existsAsFile()) tempFile.deleteFile();
        }
    
        this->processor.state = "finished";
        this->editor.webview.emitEventIfBrowserIsVisible(Identifier{"state-changed"}, "finished");
        MessageManager::callAsync([this]() {
            this->editor.deleteThread();
        });
    }
    
    ChildProcess process;
    
private:
    Editor& editor;
    Processor& processor;
};