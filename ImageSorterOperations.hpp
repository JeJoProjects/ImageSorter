#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <functional>

class ImageSorterOperations
{
public:
    static void sortImagesWithoutMatchingExtensions(
        const std::filesystem::path& sourceDir, 
        const std::string_view checkedExtension,
        const std::function<void(const std::string&)>& logCallback,
        const std::function<void(int, int)>& progressCallback
    );

private:
    static std::string normalizeExtension(const std::string_view ext);
    static bool hasMatchingFile(
        const std::filesystem::path& dir, 
        const std::string_view stemName, 
        const std::string_view checkedExt
    );
};
