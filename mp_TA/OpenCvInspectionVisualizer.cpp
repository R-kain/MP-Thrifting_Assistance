#include "OpenCvInspectionVisualizer.h"

#include <algorithm>
#include <sstream>
#include <string>

#if __has_include(<opencv2/highgui.hpp>) && __has_include(<opencv2/imgcodecs.hpp>) && __has_include(<opencv2/imgproc.hpp>)
#define MP_TA_HAS_OPENCV 1
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#else
#define MP_TA_HAS_OPENCV 0
#endif

namespace
{
#if MP_TA_HAS_OPENCV
    cv::Scalar SelectColor(const std::string& label)
    {
        if (label.find("Scratch") != std::string::npos || label.find("scratch") != std::string::npos)
        {
            return cv::Scalar(0, 0, 255);
        }

        if (label.find("Edge") != std::string::npos || label.find("edge") != std::string::npos)
        {
            return cv::Scalar(0, 165, 255);
        }

        if (label.find("Stain") != std::string::npos || label.find("stain") != std::string::npos)
        {
            return cv::Scalar(255, 0, 0);
        }

        return cv::Scalar(0, 255, 255);
    }

    cv::Rect ToPixelRect(const InspectionRegion& region, const cv::Size& imageSize)
    {
        const int x = std::max(0, static_cast<int>(region.x * imageSize.width));
        const int y = std::max(0, static_cast<int>(region.y * imageSize.height));
        const int width = std::max(1, static_cast<int>(region.width * imageSize.width));
        const int height = std::max(1, static_cast<int>(region.height * imageSize.height));

        cv::Rect rect(x, y, width, height);
        rect &= cv::Rect(0, 0, imageSize.width, imageSize.height);
        return rect;
    }
#endif
}

bool OpenCvInspectionVisualizer::IsAvailable() const
{
#if MP_TA_HAS_OPENCV
    return true;
#else
    return false;
#endif
}

bool OpenCvInspectionVisualizer::Show(const std::filesystem::path& imagePath,
    const ImageInspectionMetrics& metrics,
    const ProductScoreReport& report) const
{
#if MP_TA_HAS_OPENCV
    cv::Mat image = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
    if (image.empty())
    {
        return false;
    }

    cv::Mat annotated = image.clone();

    for (const InspectionRegion& region : metrics.suspectedScratchRegions)
    {
        const cv::Rect rect = ToPixelRect(region, annotated.size());
        if (rect.width <= 0 || rect.height <= 0)
        {
            continue;
        }

        const cv::Scalar color = SelectColor(region.label);
        cv::rectangle(annotated, rect, color, 2, cv::LINE_AA);

        std::ostringstream labelStream;
        labelStream << region.label << " " << static_cast<int>(region.confidence * 100.0) << "%";
        const std::string label = labelStream.str();

        int baseline = 0;
        const cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.55, 1, &baseline);
        const cv::Point labelOrigin(rect.x, std::max(labelSize.height + 8, rect.y - 6));
        const cv::Rect labelRect(
            labelOrigin.x,
            labelOrigin.y - labelSize.height - 6,
            labelSize.width + 10,
            labelSize.height + 10);

        cv::rectangle(annotated, labelRect, color, cv::FILLED, cv::LINE_AA);
        cv::putText(
            annotated,
            label,
            cv::Point(labelRect.x + 5, labelRect.y + labelRect.height - 6),
            cv::FONT_HERSHEY_SIMPLEX,
            0.55,
            cv::Scalar(20, 20, 20),
            1,
            cv::LINE_AA);
    }

    std::ostringstream titleStream;
    titleStream << "Inspection Overlay - " << imagePath.filename().string();
    const std::string windowTitle = titleStream.str();

    cv::putText(
        annotated,
        "Score: " + std::to_string(static_cast<int>(report.score)) + "  Grade: " + report.grade,
        cv::Point(20, 35),
        cv::FONT_HERSHEY_SIMPLEX,
        0.8,
        cv::Scalar(60, 255, 120),
        2,
        cv::LINE_AA);

    cv::putText(
        annotated,
        "Press any key to continue",
        cv::Point(20, annotated.rows - 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.65,
        cv::Scalar(230, 230, 230),
        1,
        cv::LINE_AA);

    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
    cv::imshow(windowTitle, annotated);
    cv::waitKey(0);
    cv::destroyWindow(windowTitle);
    return true;
#else
    (void)imagePath;
    (void)metrics;
    (void)report;
    return false;
#endif
}
