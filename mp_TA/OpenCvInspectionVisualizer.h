#pragma once

#include <filesystem>

#include "InspectionTypes.h"
#include "ProductIssueEvaluator.h"

class OpenCvInspectionVisualizer
{
public:
    [[nodiscard]] bool IsAvailable() const;

    bool Show(const std::filesystem::path& imagePath,
        const ImageInspectionMetrics& metrics,
        const ProductScoreReport& report) const;
};
