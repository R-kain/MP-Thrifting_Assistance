#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <vcclr.h>

#include "OpenCvInspectionVisualizer.h"
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
            if (entry.is_regular_file() && IsSupportedImageFile(entry.path()))
            {
                imageFiles.push_back(entry.path());
            }
        }

        std::sort(imageFiles.begin(), imageFiles.end());
        return imageFiles;
    }

    std::vector<InspectionRegion> BuildPlaceholderFindings(const fs::path& imagePath)
    {
        const std::size_t seed = std::hash<std::wstring>{}(imagePath.filename().wstring());
        const double offsetA = static_cast<double>((seed >> 8) & 0xF) / 100.0;
        const double offsetB = static_cast<double>((seed >> 16) & 0xF) / 120.0;

        return {
            { 0.18 + offsetA, 0.16, 0.22, 0.07, 0.82, "Scratch suspect" },
            { 0.48, 0.43 + offsetB, 0.18, 0.06, 0.76, "Scratch suspect" },
            { 0.27, 0.68, 0.26, 0.08, 0.71, "Edge damage suspect" }
        };
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
        metrics.suspectedScratchRegions = BuildPlaceholderFindings(imagePath);
        metrics.detectedIssues = {
            "Placeholder analysis for: " + imagePath.filename().string(),
            "OpenCV overlay window will draw detected issue regions on top of the source image",
            "Connect OpenCV image processing to replace these sample issues"
        };
        return metrics;
    }

    System::String^ ToManagedString(const std::string& value)
    {
        return gcnew System::String(value.c_str());
    }

    System::String^ ToManagedString(const fs::path& value)
    {
        return gcnew System::String(value.wstring().c_str());
    }

    fs::path ToNativePath(System::String^ value)
    {
        if (System::String::IsNullOrWhiteSpace(value))
        {
            return fs::path();
        }

        pin_ptr<const wchar_t> pinnedValue = PtrToStringChars(value);
        return fs::path(pinnedValue);
    }
}

namespace mpTA
{
    using namespace System;
    using namespace System::Drawing;
    using namespace System::Windows::Forms;

    public ref class MainForm sealed : public Form
    {
    public:
        MainForm()
        {
            InitializeComponent();

            array<String^>^ args = Environment::GetCommandLineArgs();
            if (args != nullptr && args->Length > 1)
            {
                directoryTextBox->Text = args[1];
            }
        }

    private:
        TextBox^ directoryTextBox;
        Button^ browseButton;
        Button^ analyzeButton;
        CheckBox^ showOverlayCheckBox;
        TextBox^ outputTextBox;
        String^ pathPlaceholderText;
        array<String^>^ selectedInputPaths;
        bool isPathPlaceholderVisible;
        bool suppressPathTextChange;

        void InitializeComponent()
        {
            pathPlaceholderText = "Drag image files/folders here or click Browse";
            selectedInputPaths = nullptr;
            isPathPlaceholderVisible = false;
            suppressPathTextChange = false;

            Text = "mp_TA Product Inspection";
            StartPosition = FormStartPosition::CenterScreen;
            MinimumSize = System::Drawing::Size(780, 520);
            Size = System::Drawing::Size(920, 640);
            AllowDrop = true;
            DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);

            auto root = gcnew TableLayoutPanel();
            root->Dock = DockStyle::Fill;
            root->ColumnCount = 1;
            root->RowCount = 3;
            root->Padding = System::Windows::Forms::Padding(12);
            root->AllowDrop = true;
            root->DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            root->DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);
            root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 34));
            root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 44));
            root->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
            Controls->Add(root);

            auto titleLabel = gcnew Label();
            titleLabel->Text = "Image inspection";
            titleLabel->Dock = DockStyle::Fill;
            titleLabel->Font = gcnew Drawing::Font("Segoe UI", 11.0f, FontStyle::Bold);
            titleLabel->TextAlign = ContentAlignment::MiddleLeft;
            titleLabel->AllowDrop = true;
            titleLabel->DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            titleLabel->DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);
            root->Controls->Add(titleLabel, 0, 0);

            auto inputPanel = gcnew TableLayoutPanel();
            inputPanel->Dock = DockStyle::Fill;
            inputPanel->ColumnCount = 4;
            inputPanel->RowCount = 1;
            inputPanel->AllowDrop = true;
            inputPanel->DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            inputPanel->DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);
            inputPanel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
            inputPanel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 88));
            inputPanel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 118));
            inputPanel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 112));
            root->Controls->Add(inputPanel, 0, 1);

            directoryTextBox = gcnew TextBox();
            directoryTextBox->Dock = DockStyle::Fill;
            directoryTextBox->Font = gcnew Drawing::Font("Segoe UI", 10.0f);
            directoryTextBox->Margin = System::Windows::Forms::Padding(0, 8, 8, 4);
            directoryTextBox->AllowDrop = true;
            directoryTextBox->DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            directoryTextBox->DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);
            directoryTextBox->Enter += gcnew EventHandler(this, &MainForm::DirectoryTextBox_Enter);
            directoryTextBox->Leave += gcnew EventHandler(this, &MainForm::DirectoryTextBox_Leave);
            directoryTextBox->TextChanged += gcnew EventHandler(this, &MainForm::DirectoryTextBox_TextChanged);
            inputPanel->Controls->Add(directoryTextBox, 0, 0);

            browseButton = gcnew Button();
            browseButton->Text = "Browse";
            browseButton->Dock = DockStyle::Fill;
            browseButton->Margin = System::Windows::Forms::Padding(0, 6, 8, 4);
            browseButton->Click += gcnew EventHandler(this, &MainForm::BrowseButton_Click);
            inputPanel->Controls->Add(browseButton, 1, 0);

            analyzeButton = gcnew Button();
            analyzeButton->Text = "Analyze";
            analyzeButton->Dock = DockStyle::Fill;
            analyzeButton->Margin = System::Windows::Forms::Padding(0, 6, 8, 4);
            analyzeButton->Click += gcnew EventHandler(this, &MainForm::AnalyzeButton_Click);
            inputPanel->Controls->Add(analyzeButton, 2, 0);

            showOverlayCheckBox = gcnew CheckBox();
            showOverlayCheckBox->Text = "Overlay";
            showOverlayCheckBox->Checked = true;
            showOverlayCheckBox->Dock = DockStyle::Fill;
            showOverlayCheckBox->TextAlign = ContentAlignment::MiddleLeft;
            showOverlayCheckBox->Margin = System::Windows::Forms::Padding(0, 8, 0, 4);
            inputPanel->Controls->Add(showOverlayCheckBox, 3, 0);

            outputTextBox = gcnew TextBox();
            outputTextBox->Dock = DockStyle::Fill;
            outputTextBox->Multiline = true;
            outputTextBox->ReadOnly = true;
            outputTextBox->ScrollBars = ScrollBars::Vertical;
            outputTextBox->Font = gcnew Drawing::Font("Consolas", 10.0f);
            outputTextBox->AllowDrop = true;
            outputTextBox->DragEnter += gcnew DragEventHandler(this, &MainForm::MainForm_DragEnter);
            outputTextBox->DragDrop += gcnew DragEventHandler(this, &MainForm::MainForm_DragDrop);
            root->Controls->Add(outputTextBox, 0, 2);

            ShowPathPlaceholder();
        }

        void BrowseButton_Click(Object^ sender, EventArgs^ e)
        {
            (void)sender;
            (void)e;

            auto dialog = gcnew OpenFileDialog();
            dialog->Title = "Select image file";
            dialog->Filter = "Image files (*.jpg;*.jpeg;*.png;*.bmp;*.webp)|*.jpg;*.jpeg;*.png;*.bmp;*.webp|All files (*.*)|*.*";
            dialog->Multiselect = true;
            dialog->CheckFileExists = true;
            dialog->RestoreDirectory = true;

            String^ currentPath = GetActualInputText();
            if (!String::IsNullOrWhiteSpace(currentPath))
            {
                if (System::IO::Directory::Exists(currentPath))
                {
                    dialog->InitialDirectory = currentPath;
                }
                else if (System::IO::File::Exists(currentPath))
                {
                    dialog->InitialDirectory = System::IO::Path::GetDirectoryName(currentPath);
                }
            }

            if (dialog->ShowDialog(this) == Windows::Forms::DialogResult::OK)
            {
                selectedInputPaths = dialog->FileNames;
                std::vector<fs::path> imageFiles = BuildImageListFromDrop(selectedInputPaths);
                outputTextBox->Clear();

                if (imageFiles.empty())
                {
                    SetInputText("");
                    ShowPathPlaceholder();
                    AppendLine("Selected files did not include supported images.");
                    return;
                }

                SetInputText(BuildSelectionDisplayText(selectedInputPaths, imageFiles));
                AnalyzeImageFiles(imageFiles, String::Format("file selection: {0} image file(s)", static_cast<int>(imageFiles.size())));
            }
        }

        void AnalyzeButton_Click(Object^ sender, EventArgs^ e)
        {
            (void)sender;
            (void)e;

            outputTextBox->Clear();
            if (selectedInputPaths != nullptr)
            {
                std::vector<fs::path> imageFiles = BuildImageListFromDrop(selectedInputPaths);
                if (!imageFiles.empty())
                {
                    AnalyzeImageFiles(imageFiles, String::Format("file selection: {0} image file(s)", static_cast<int>(imageFiles.size())));
                    return;
                }

                selectedInputPaths = nullptr;
            }

            const fs::path inputPath = ToNativePath(GetActualInputText());
            if (inputPath.empty())
            {
                AppendLine("Select an image directory first, or drag image files onto this window.");
                return;
            }

            if (fs::is_regular_file(inputPath) && IsSupportedImageFile(inputPath))
            {
                AnalyzeImageFiles({ inputPath }, "file: " + ToManagedString(inputPath));
                return;
            }

            if (!fs::exists(inputPath) || !fs::is_directory(inputPath))
            {
                AppendLine("The specified path is not a valid image file or directory: " + ToManagedString(inputPath));
                return;
            }

            AnalyzeImageFiles(CollectImageFiles(inputPath), "directory: " + ToManagedString(inputPath));
        }

        void DirectoryTextBox_Enter(Object^ sender, EventArgs^ e)
        {
            (void)sender;
            (void)e;
            ClearPathPlaceholder();
        }

        void DirectoryTextBox_Leave(Object^ sender, EventArgs^ e)
        {
            (void)sender;
            (void)e;

            if (String::IsNullOrWhiteSpace(directoryTextBox->Text))
            {
                ShowPathPlaceholder();
            }
        }

        void DirectoryTextBox_TextChanged(Object^ sender, EventArgs^ e)
        {
            (void)sender;
            (void)e;

            if (!suppressPathTextChange && !isPathPlaceholderVisible)
            {
                selectedInputPaths = nullptr;
            }
        }

        void MainForm_DragEnter(Object^ sender, DragEventArgs^ e)
        {
            (void)sender;

            if (!e->Data->GetDataPresent(DataFormats::FileDrop))
            {
                e->Effect = DragDropEffects::None;
                return;
            }

            auto paths = safe_cast<array<String^>^>(e->Data->GetData(DataFormats::FileDrop));
            e->Effect = BuildImageListFromDrop(paths).empty() ? DragDropEffects::None : DragDropEffects::Copy;
        }

        void MainForm_DragDrop(Object^ sender, DragEventArgs^ e)
        {
            (void)sender;

            auto paths = safe_cast<array<String^>^>(e->Data->GetData(DataFormats::FileDrop));
            std::vector<fs::path> imageFiles = BuildImageListFromDrop(paths);
            outputTextBox->Clear();

            if (imageFiles.empty())
            {
                AppendLine("Dropped files did not include supported images.");
                return;
            }

            selectedInputPaths = paths;
            SetInputText(BuildSelectionDisplayText(paths, imageFiles));
            AnalyzeImageFiles(imageFiles, String::Format("drag-and-drop selection: {0} image file(s)", static_cast<int>(imageFiles.size())));
        }

        std::vector<fs::path> BuildImageListFromDrop(array<String^>^ paths)
        {
            std::vector<fs::path> imageFiles;
            if (paths == nullptr)
            {
                return imageFiles;
            }

            for each (String ^ pathText in paths)
            {
                const fs::path path = ToNativePath(pathText);
                if (fs::is_directory(path))
                {
                    std::vector<fs::path> directoryImages = CollectImageFiles(path);
                    imageFiles.insert(imageFiles.end(), directoryImages.begin(), directoryImages.end());
                }
                else if (fs::is_regular_file(path) && IsSupportedImageFile(path))
                {
                    imageFiles.push_back(path);
                }
            }

            std::sort(imageFiles.begin(), imageFiles.end());
            imageFiles.erase(std::unique(imageFiles.begin(), imageFiles.end()), imageFiles.end());
            return imageFiles;
        }

        String^ BuildSelectionDisplayText(array<String^>^ paths, const std::vector<fs::path>& imageFiles)
        {
            if (paths != nullptr && paths->Length == 1)
            {
                return paths[0];
            }

            return String::Format("{0} selected image file(s)", static_cast<int>(imageFiles.size()));
        }

        String^ GetActualInputText()
        {
            if (isPathPlaceholderVisible)
            {
                return "";
            }

            return directoryTextBox->Text;
        }

        void SetInputText(String^ text)
        {
            suppressPathTextChange = true;
            isPathPlaceholderVisible = false;
            directoryTextBox->ForeColor = SystemColors::WindowText;
            directoryTextBox->Text = text;
            suppressPathTextChange = false;
        }

        void ShowPathPlaceholder()
        {
            suppressPathTextChange = true;
            isPathPlaceholderVisible = true;
            directoryTextBox->ForeColor = SystemColors::GrayText;
            directoryTextBox->Text = pathPlaceholderText;
            suppressPathTextChange = false;
        }

        void ClearPathPlaceholder()
        {
            if (!isPathPlaceholderVisible)
            {
                return;
            }

            suppressPathTextChange = true;
            isPathPlaceholderVisible = false;
            directoryTextBox->ForeColor = SystemColors::WindowText;
            directoryTextBox->Clear();
            suppressPathTextChange = false;
        }

        void AnalyzeImageFiles(const std::vector<fs::path>& imageFiles, String^ sourceLabel)
        {
            analyzeButton->Enabled = false;
            browseButton->Enabled = false;
            Cursor = Cursors::WaitCursor;

            try
            {
                if (imageFiles.empty())
                {
                    AppendLine("No supported image files were found in " + sourceLabel + ".");
                    return;
                }

                AppendLine(String::Format("Found {0} image file(s) from {1}.", static_cast<int>(imageFiles.size()), sourceLabel));
                AppendLine("Current analysis values are placeholder metrics until the OpenCV detector is wired in.");

                const ProductIssueEvaluator evaluator;
                const OpenCvInspectionVisualizer visualizer;
                const bool canShowOverlay = visualizer.IsAvailable() && showOverlayCheckBox->Checked;

                if (!visualizer.IsAvailable())
                {
                    AppendLine("OpenCV headers/libs are not configured in this build, so overlay windows are skipped.");
                }
                else if (showOverlayCheckBox->Checked)
                {
                    AppendLine("OpenCV overlay windows are enabled. Close each overlay window to continue.");
                }
                else
                {
                    AppendLine("OpenCV overlay windows are disabled.");
                }

                for (const auto& imagePath : imageFiles)
                {
                    const ImageInspectionMetrics metrics = BuildPlaceholderMetrics(imagePath);
                    const ProductScoreReport report = evaluator.Evaluate(metrics);
                    AppendReport(imagePath, report);

                    if (canShowOverlay && !visualizer.Show(imagePath, metrics, report))
                    {
                        AppendLine("Failed to open OpenCV overlay window for: " + ToManagedString(imagePath));
                    }
                }
            }
            catch (const std::exception& ex)
            {
                AppendLine("Error: " + ToManagedString(std::string(ex.what())));
            }
            finally
            {
                Cursor = Cursors::Default;
                analyzeButton->Enabled = true;
                browseButton->Enabled = true;
            }
        }

        void AppendReport(const fs::path& imagePath, const ProductScoreReport& report)
        {
            AppendLine("");
            AppendLine("========================================");
            AppendLine("Image file: " + ToManagedString(imagePath.filename()));
            AppendLine(String::Format("Final score: {0:F1}", report.score));
            AppendLine("Grade: " + ToManagedString(report.grade));
            AppendLine("Summary: " + ToManagedString(report.summary));

            if (report.detectedIssues.empty())
            {
                AppendLine("Detected issues: none");
                return;
            }

            AppendLine("Detected issues:");
            for (const auto& issue : report.detectedIssues)
            {
                AppendLine("- " + ToManagedString(issue));
            }
        }

        void AppendLine(String^ text)
        {
            outputTextBox->AppendText(text + Environment::NewLine);
        }
    };
}

[System::STAThreadAttribute]
int main()
{
    System::Windows::Forms::Application::EnableVisualStyles();
    System::Windows::Forms::Application::SetCompatibleTextRenderingDefault(false);

    auto form = gcnew mpTA::MainForm();
    System::Windows::Forms::Application::Run(form);
    return 0;
}
