#include <iomanip>
#include <iostream>
#include <locale>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

#include "ProductIssueEvaluator.h"

namespace
{
    void InitializeConsole()
    {
#if defined(_WIN32)
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
#endif
        std::locale::global(std::locale(""));
        std::wcout.imbue(std::locale());
        std::wcerr.imbue(std::locale());
    }

    void PrintReport(const ProductScoreReport& report)
    {
        std::wcout << L"=== Product Exterior Inspection Report ===\n";
        std::wcout << L"Final score: " << std::fixed << std::setprecision(1) << report.score << L"\n";
        std::wcout << L"Grade: " << report.grade << L"\n";
        std::wcout << L"Summary: " << report.summary << L"\n";

        if (report.detectedIssues.empty())
        {
            std::wcout << L"Detected issues: none\n";
            return;
        }

        std::wcout << L"Detected issues:\n";
        for (const auto& issue : report.detectedIssues)
        {
            std::wcout << L"- " << issue << L"\n";
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
            .detectedIssues = {L"Light scratches on the front surface", L"Wear on the lower-right corner"}
        };
    }
}

int main()
{
    InitializeConsole();

    // TODO: OpenCV로 이미지에서 외관 지표를 추출한 뒤 BuildSampleMetrics 대신 실제 측정값을 연결합니다.
    // TODO: OpenGL을 도입하면 손상 위치를 오버레이 시각화할 수 있습니다.
    const ImageInspectionMetrics metrics = BuildSampleMetrics();

    const ProductIssueEvaluator evaluator;
    const ProductScoreReport report = evaluator.Evaluate(metrics);

    PrintReport(report);
    return 0;
}
