#include <iomanip>
#include <iostream>

#include "ProductIssueEvaluator.h"

namespace
{
    void PrintReport(const ProductScoreReport& report)
    {
        std::cout << "=== 상품 외관 분석 결과 ===\n";
        std::cout << "최종 점수: " << std::fixed << std::setprecision(1) << report.score << "\n";
        std::cout << "등급: " << report.grade << "\n";
        std::cout << "요약: " << report.summary << "\n";

        if (report.detectedIssues.empty())
        {
            std::cout << "탐지된 문제: 없음\n";
            return;
        }

        std::cout << "탐지된 문제:\n";
        for (const auto& issue : report.detectedIssues)
        {
            std::cout << "- " << issue << "\n";
        }
    }

    ImageInspectionMetrics BuildSampleMetrics()
    {
        return ImageInspectionMetrics{
            .scratchSeverity = 0.35,
            .stainSeverity = 0.20,
            .discolorationSeverity = 0.15,
            .edgeDamageSeverity = 0.40,
            .shapeDeformationSeverity = 0.10,
            .analysisConfidence = 0.88,
            .detectedIssues = {"전면 생활 스크래치", "우측 하단 모서리 마모"}
        };
    }
}

int main()
{
    // TODO: OpenCV로 이미지에서 외관 지표를 추출한 뒤 BuildSampleMetrics 대신 실제 측정값을 연결합니다.
    // TODO: OpenGL을 도입하면 손상 위치를 오버레이 시각화할 수 있습니다.
    const ImageInspectionMetrics metrics = BuildSampleMetrics();

    const ProductIssueEvaluator evaluator;
    const ProductScoreReport report = evaluator.Evaluate(metrics);

    PrintReport(report);
    return 0;
}
