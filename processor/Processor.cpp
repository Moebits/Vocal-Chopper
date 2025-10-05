#include "Processor.h"
#include "Editor.h"

auto Processor::createEditor() -> AudioProcessorEditor* {
    return new Editor(*this);
}

auto Processor::getStateInformation([[maybe_unused]] MemoryBlock& destData) -> void {
    auto* obj = new DynamicObject();
    obj->setProperty("audioPath", this->audioPath);
    obj->setProperty("destFolder", this->destFolder);
    obj->setProperty("skipVocalExtraction", this->skipVocalExtraction);
    obj->setProperty("keepVocalFile", this->keepVocalFile);
    obj->setProperty("droppedFileBytes", this->droppedFileBytes.toBase64Encoding());
    obj->setProperty("state", this->state);
    obj->setProperty("progress", this->progress);

    auto jsonString = JSON::toString(var{obj});
    MemoryOutputStream(destData, true).writeString(jsonString);
}
auto Processor::setStateInformation([[maybe_unused]] const void* data, [[maybe_unused]] int sizeInBytes) -> void {
    String jsonString{static_cast<const char*>(data), static_cast<size_t>(sizeInBytes)};
    auto parsed = JSON::parse(jsonString);

    if (auto* obj = parsed.getDynamicObject()) {
        this->audioPath = obj->getProperty("audioPath").toString();
        this->destFolder = obj->getProperty("destFolder").toString();
        this->skipVocalExtraction = static_cast<bool>(obj->getProperty("skipVocalExtraction"));
        this->keepVocalFile = static_cast<bool>(obj->getProperty("keepVocalFile"));
        auto base64 = obj->getProperty("droppedFileBytes").toString();
        droppedFileBytes.fromBase64Encoding(base64);
        this->state = obj->getProperty("state").toString();
        this->progress = static_cast<double>(obj->getProperty("progress"));
    }
}

auto JUCE_CALLTYPE createPluginFilter() -> AudioProcessor* {
    return new Processor();
}