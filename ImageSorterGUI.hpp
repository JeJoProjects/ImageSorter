#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDD_ADDEXTENSION 101
#define IDC_EXTENSION 1001

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

class ImageSorterGUI
{
public:
    ImageSorterGUI();
    ~ImageSorterGUI();

    bool Create(HINSTANCE hInstance, const char* title, int width, int height);
    void Show(int nCmdShow);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void UpdateProgress();

    void CreateControls();
    void CreateMenu();
    void BrowseFolder();
    void SortImages();
    void ResizeControls(int width, int height);
    void UpdateExtensionSelection(int buttonId);
    void AppendToLog(const std::string& message);
    void UpdateProgressBar(int value, int max);
    void SaveConfig();
    void LoadConfig();
    void AddExtension();
    void SaveExtensionsToConfig();
    void LoadExtensionsFromConfig();

    HWND m_hwnd;
    HWND m_hEdit;
    HWND m_hBrowseButton;
    HWND m_hSortButton;
    HWND m_hSaveConfigButton;
    HWND m_hAddExtensionButton;
    HWND m_hStatus;
    HWND m_hLogEdit;
    HWND m_hProgressBar;
    HFONT m_hFont;

    std::vector<HWND> m_extensionButtons;
    std::vector<std::string> m_extensions;
    int m_selectedExtensionIndex;

    HBRUSH m_hBackgroundBrush;
    HBRUSH m_hButtonBrush;
    std::atomic<int> m_progressValue;
    std::thread m_sortingThread;

    static ImageSorterGUI* GetThisFromHandle(HWND hwnd);
};

// This should be outside the class definition
INT_PTR CALLBACK AddExtensionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);