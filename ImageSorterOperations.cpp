#include "ImageSorterOperations.hpp"
#include <algorithm>
#include <ranges>
#include <vector>
#include <cctype>
#include <thread>
#include <mutex>
#include <atomic>

std::string ImageSorterOperations::normalizeExtension(const std::string_view ext)
{
    std::string normalized = ext.empty() || ext[0] == '.' ? std::string{ ext } : "." + std::string{ ext };
    std::ranges::transform(normalized, normalized.begin(), [](unsigned char c) { return std::tolower(c); });
    return normalized;
}

bool ImageSorterOperations::hasMatchingFile(const std::filesystem::path& dir, const std::string_view stemName, const std::string_view checkedExt)
{
    return std::ranges::any_of(std::filesystem::directory_iterator{ dir }, [&](const auto& entry) {
        return std::filesystem::is_regular_file(entry)
            && entry.path().stem() == stemName
            && normalizeExtension(entry.path().extension().string()) == checkedExt;
    });
}

void ImageSorterOperations::sortImagesWithoutMatchingExtensions(
    const std::filesystem::path& sourceDir,
    const std::string_view checkedExtension,
    const std::function<void(const std::string&)>& logCallback,
    const std::function<void(int, int)>& progressCallback)
{
    if (!std::filesystem::exists(sourceDir) || !std::filesystem::is_directory(sourceDir))
    {
        logCallback("Error: Source directory does not exist or is not a directory.");
        return;
    }

    const std::string normalizedCheckedExt = normalizeExtension(checkedExtension);
    const auto filesToMove = std::filesystem::directory_iterator{ sourceDir }
        | std::views::filter([](const auto& entry) { return std::filesystem::is_regular_file(entry); })
        | std::views::filter([&](const auto& entry) {
            return normalizeExtension(entry.path().extension().string()) != normalizedCheckedExt
                && !hasMatchingFile(sourceDir, entry.path().stem().string(), normalizedCheckedExt);
        })
        | std::ranges::to<std::vector>();

    if (filesToMove.empty())
    {
        logCallback("No images need to be sorted.");
        return;
    }

    const std::filesystem::path destDir = sourceDir / ("ImagesOtherThan" + normalizedCheckedExt.substr(1));

    if (!std::filesystem::exists(destDir))
    {
        std::filesystem::create_directory(destDir);
        logCallback("Created destination directory: " + destDir.string());
    }

    std::atomic<int> movedCount = 0;
    int totalFiles = filesToMove.size();
    std::mutex logMutex;

    auto moveFiles = [&](const std::vector<std::filesystem::directory_entry>& files) {
        for (const auto& file : files)
        {
            const std::filesystem::path destFile = destDir / file.path().filename();
            try
            {
                std::filesystem::rename(file, destFile);
                {
                    std::lock_guard<std::mutex> lock(logMutex);
                    logCallback("Moved: " + file.path().filename().string());
                }
                ++movedCount;
                progressCallback(movedCount, totalFiles);
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::lock_guard<std::mutex> lock(logMutex);
                logCallback("Error moving file " + file.path().filename().string() + ": " + e.what());
            }
        }
    };

    unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    size_t filesPerThread = filesToMove.size() / numThreads;
    size_t remainingFiles = filesToMove.size() % numThreads;

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        size_t start = i * filesPerThread;
        size_t end = (i + 1) * filesPerThread;
        if (i == numThreads - 1)
        {
            end += remainingFiles;
        }
        threads.emplace_back(moveFiles, std::vector<std::filesystem::directory_entry>(filesToMove.begin() + start, filesToMove.begin() + end));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    std::string message = "Sorted " + std::to_string(movedCount) + " images to " + destDir.string();
    logCallback(message);
}
