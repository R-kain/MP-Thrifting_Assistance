#include <iomanip>
#include <iostream>

#include "ProductIssueEvaluator.h"

namespace
{
    void PrintReport(const ProductScoreReport& report)
    {
        std::cout << "=== Product Exterior Inspection Report ===\n";
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

    ImageInspectionMetrics BuildSampleMetrics()
    {
        ImageInspectionMetrics metrics;
        metrics.scratchSeverity = 0.35;
        metrics.stainSeverity = 0.20;
        metrics.discolorationSeverity = 0.15;
        metrics.edgeDamageSeverity = 0.40;
        metrics.shapeDeformationSeverity = 0.10;
        metrics.analysisConfidence = 0.88;
        metrics.detectedIssues = {
            "Light scratches on the front surface",
            "Wear on the lower-right corner"
        };
        return metrics;
    }
}

int main()
{
    // TODO: Replace this sample with actual metrics extracted from product photos through OpenCV.
    // TODO: Add OpenGL overlays later if you want to visualize damaged regions on top of the image.
    const ImageInspectionMetrics metrics = BuildSampleMetrics();

    const ProductIssueEvaluator evaluator;
    const ProductScoreReport report = evaluator.Evaluate(metrics);

    PrintReport(report);
    return 0;
}
