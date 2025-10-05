#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
  #include <windows.h>
  #include <shlobj.h>
  #pragma comment(lib, "Shell32.lib")
#endif

class Functions {
public:
    static auto streamToVector(InputStream& stream) -> std::vector<std::byte> {
        std::vector<std::byte> result(static_cast<size_t>(stream.getTotalLength()));
        stream.setPosition(0);
        [[maybe_unused]] auto bytesRead = stream.read (result.data(), result.size());
        jassert (bytesRead == static_cast<ssize_t>(result.size()));
        return result;
    }

    static auto getMimeForExtension(const String& extension) -> const char* {
        static std::unordered_map<String, const char*> mimeMap = {
            {"html", "text/html"       },
            {"css",  "text/css"        },
            {"js",   "text/javascript" },
            {"txt",  "text/plain"      },
            {"jpg",  "image/jpeg"      },
            {"png",  "image/png"       },
            {"jpeg", "image/jpeg"      },
            {"svg",  "image/svg+xml"   },
            {"json", "application/json"},
            {"map",  "application/json"},
            {"ttf",  "font/ttf"        },
            {"otf",  "font/otf"        },
            {"woff2","font/woff2"      }
        };
    
        auto it = mimeMap.find(extension.toLowerCase());
        if (it != mimeMap.end()) {
            return it->second;
        }
    
        jassertfalse;
        return "";
    }

    static auto getDownloadsFolder() -> File {
        #if JUCE_WINDOWS
            PWSTR path = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path))) {
                int utf8Size = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                if (utf8Size > 0) {
                    auto utf8Path = std::unique_ptr<char[]>(new char[utf8Size]);
                    WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8Path.get(), utf8Size, nullptr, nullptr);
                    String downloadsPath = String::fromUTF8(utf8Path.get());
                    CoTaskMemFree(path);
                    return File(downloadsPath);
                }
                CoTaskMemFree(path);
            }
            return File::getSpecialLocation(File::userDocumentsDirectory);
        #else
            auto downloadsPath = File::getSpecialLocation(File::userHomeDirectory).getChildFile("Downloads");
            if (downloadsPath.exists() && downloadsPath.isDirectory()) {
                return downloadsPath;
            }
            return File::getSpecialLocation(File::userDocumentsDirectory);
        #endif
    }
};