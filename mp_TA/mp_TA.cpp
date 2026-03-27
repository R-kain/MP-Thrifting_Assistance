#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#if __has_include(<opencv2/highgui.hpp>) && __has_include(<opencv2/imgcodecs.hpp>) && __has_include(<opencv2/imgproc.hpp>)
#define MP_TA_HAS_OPENCV 1
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#else
#define MP_TA_HAS_OPENCV 0
#endif


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

#if MP_TA_HAS_OPENCV
    std::vector<cv::Rect> BuildPlaceholderDefectRegions(const cv::Size& size)
    {
        const int width = std::max(1, size.width);
        const int height = std::max(1, size.height);

        std::vector<cv::Rect> regions;
        regions.emplace_back(width / 8, height / 6, std::max(20, width / 5), std::max(20, height / 10));
        regions.emplace_back(width / 2, height / 2, std::max(24, width / 6), std::max(24, height / 8));
        regions.emplace_back(width * 3 / 4, height * 7 / 10, std::max(16, width / 8), std::max(16, height / 10));

        for (auto& region : regions)
        {
            region &= cv::Rect(0, 0, width, height);
        }

        return regions;
    }

    void HighlightDefectRegions(cv::Mat& image, const std::vector<cv::Rect>& regions)
    {
        for (const auto& region : regions)
        {
            if (region.width <= 0 || region.height <= 0)
            {
                continue;
            }

            cv::Mat roi = image(region);
            cv::Mat redOverlay(roi.size(), roi.type(), cv::Scalar(0, 0, 255));
            cv::addWeighted(roi, 0.45, redOverlay, 0.55, 0.0, roi);
            cv::rectangle(image, region, cv::Scalar(0, 0, 255), 3);
        }
    }

    void ShowAnnotatedImage(const fs::path& imagePath)
    {
        cv::Mat image = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
        if (image.empty())
        {
            std::cerr << "Could not read image for visualization: " << imagePath.string() << "\n";
            return;
        }

        std::vector<cv::Rect> defectRegions = BuildPlaceholderDefectRegions(image.size());
        HighlightDefectRegions(image, defectRegions);

        const std::string windowName = "Defect Visibility - " + imagePath.filename().string();
        cv::imshow(windowName, image);
        cv::waitKey(700);
        cv::destroyWindow(windowName);
    }
#else
    void ShowAnnotatedImage(const fs::path& imagePath)
    {
        std::cout << "[OpenCV unavailable] Skipping visualization window for: " << imagePath.filename().string() << "\n";
    }
#endif
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
#if MP_TA_HAS_OPENCV
    std::cout << "OpenCV visualization is enabled: defect-like regions will be highlighted in red and shown in windows.\n";
#else
    std::cout << "OpenCV headers were not found at build time; visualization windows are skipped.\n";
#endif
    std::cout << "OpenCV defect analysis is not connected yet, so placeholder metrics are being used.\n";

    const ProductIssueEvaluator evaluator;
    for (const auto& imagePath : imageFiles)
    {
        const ImageInspectionMetrics metrics = BuildPlaceholderMetrics(imagePath);
        const ProductScoreReport report = evaluator.Evaluate(metrics);
        PrintReport(imagePath, report);
        ShowAnnotatedImage(imagePath);
    }

#if MP_TA_HAS_OPENCV
    cv::destroyAllWindows();
#endif

    return 0;
}

