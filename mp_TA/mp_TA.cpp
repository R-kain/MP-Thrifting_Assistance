#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "ProductIssueEvaluator.h"

namespace
{
    namespace fs = std::filesystem;

    bool IsSupportedImageFile(const fs::path& filePath)
    {
        if (!filePath.has_extension())
        {
            return false;
        }

        std::string extension = filePath.extension().string();
        for (char& ch : extension)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        return extension == ".jpg"
            || extension == ".jpeg"
            || extension == ".png"
            || extension == ".bmp"
            || extension == ".webp";
    }

    std::vector<fs::path> CollectImageFiles(const fs::path& directoryPath)
    {
        std::vector<fs::path> imageFiles;

        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (IsSupportedImageFile(entry.path()))
            {
                imageFiles.push_back(entry.path());
            }
        }

        std::sort(imageFiles.begin(), imageFiles.end());
        return imageFiles;
    }

    void PrintReport(const fs::path& imagePath, const ProductScoreReport& report)
    {
        std::cout << "========================================\n";
        std::cout << "Image file: " << imagePath.filename().string() << "\n";
        std::cout << "Final score: " << std::fixed << std::setprecision(1) << report.score << "\n";
        std::cout << "Grade: " << report.grade << "\n";
        std::cout << "Summary: " << report.summary << "\n";

        if (report.detectedIssues.empty())
        {
            std::cout << "Detected issues: none\n";
            return;
        }

        std::cout << "Detected issues:\n";
        for (const auto& issue : report.detectedIssues)
        {
            std::cout << "- " << issue << "\n";
        }
    }

    ImageInspectionMetrics BuildPlaceholderMetrics(const fs::path& imagePath)
    {
        ImageInspectionMetrics metrics;
        metrics.scratchSeverity = 0.35;
        metrics.stainSeverity = 0.20;
        metrics.discolorationSeverity = 0.15;
        metrics.edgeDamageSeverity = 0.40;
        metrics.shapeDeformationSeverity = 0.10;
        metrics.analysisConfidence = 0.88;
        metrics.detectedIssues = {
            "Placeholder analysis for: " + imagePath.filename().string(),
            "Connect OpenCV image processing to replace these sample issues"
        };
        return metrics;
    }

    void PrintUsage(const char* executableName)
    {
        std::cout << "Usage: " << executableName << " <image_directory>\n";
        std::cout << "Example: " << executableName << " C:/images/products\n";
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const fs::path imageDirectory = argv[1];
    if (!fs::exists(imageDirectory) || !fs::is_directory(imageDirectory))
    {
        std::cerr << "The specified path is not a valid directory: " << imageDirectory.string() << "\n";
        return 1;
    }

    const std::vector<fs::path> imageFiles = CollectImageFiles(imageDirectory);
    if (imageFiles.empty())
    {
        std::cout << "No supported image files were found in: " << imageDirectory.string() << "\n";
        return 0;
    }

    std::cout << "Found " << imageFiles.size() << " image file(s) in: " << imageDirectory.string() << "\n";
    std::cout << "OpenCV is not connected yet, so placeholder metrics are being used.\n";

    const ProductIssueEvaluator evaluator;
    for (const auto& imagePath : imageFiles)
    {
        const ImageInspectionMetrics metrics = BuildPlaceholderMetrics(imagePath);
        const ProductScoreReport report = evaluator.Evaluate(metrics);
        PrintReport(imagePath, report);
    }

    return 0;
}
