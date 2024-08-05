#include "ImageSorterGUI.hpp"
#include "ImageSorterOperations.hpp"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <windowsx.h>
#include <fstream>
#include <filesystem>

const char* CONFIG_FILE = "ImageSorter_config.cfg";
const char* EXTENSIONS_FILE = "extensions.cfg";

#define ID_FILE_EXIT 1001
#define ID_ABOUT 1002
#define ID_SAVE_CONFIG 1003
#define ID_ADD_EXTENSION 1004

#define IDD_ADDEXTENSION 101
#define IDC_EXTENSION 1001

ImageSorterGUI::ImageSorterGUI()
	: m_hwnd(NULL), m_hEdit(NULL), m_hBrowseButton(NULL), m_hSortButton(NULL), m_hSaveConfigButton(NULL),
	m_hAddExtensionButton(NULL), m_hStatus(NULL), m_hLogEdit(NULL), m_hProgressBar(NULL), m_hFont(NULL),
	m_selectedExtensionIndex(-1), m_hBackgroundBrush(NULL), m_hButtonBrush(NULL), m_progressValue(0)
{
	LoadExtensionsFromConfig();
	m_hBackgroundBrush = CreateSolidBrush(RGB(240, 240, 240));
	m_hButtonBrush = CreateSolidBrush(RGB(0, 120, 215));
}

ImageSorterGUI::~ImageSorterGUI()
{
	if (m_hFont) DeleteObject(m_hFont);
	if (m_hBackgroundBrush) DeleteObject(m_hBackgroundBrush);
	if (m_hButtonBrush) DeleteObject(m_hButtonBrush);
	if (m_sortingThread.joinable()) m_sortingThread.join();
	if (m_hwnd) DestroyWindow(m_hwnd);
	for (HWND hButton : m_extensionButtons)
	{
		if (hButton) DestroyWindow(hButton);
	}
}

bool ImageSorterGUI::Create(HINSTANCE hInstance, const char* title, int width, int height)
{
	WNDCLASSA wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "ImageSorterClass";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClassA(&wc);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	width = screenWidth / 2;
	height = screenHeight;
	int x = 0;
	int y = 0;

	m_hwnd = CreateWindowA(
		"ImageSorterClass", title,
		WS_OVERLAPPEDWINDOW,
		x, y, width, height,
		NULL, NULL, hInstance, this
	);

	if (m_hwnd)
	{
		CreateMenu();
	}

	return (m_hwnd != NULL);
}

void ImageSorterGUI::Show(int nCmdShow)
{
	ShowWindow(m_hwnd, nCmdShow);
}

LRESULT CALLBACK ImageSorterGUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImageSorterGUI* pThis = NULL;

	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		pThis = (ImageSorterGUI*)pCreate->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

		pThis->m_hwnd = hwnd;
	}
	else
	{
		pThis = GetThisFromHandle(hwnd);
	}

	return pThis ? pThis->HandleMessage(uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ImageSorterGUI::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		CreateControls();
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 1: // Browse button
			BrowseFolder();
			break;
		case 2: // Sort images button
			SortImages();
			break;
		case ID_SAVE_CONFIG: // Save config button
		{
			SaveConfig();
			MessageBoxA(m_hwnd, "Configuration saved successfully.", "Save Configuration", MB_OK | MB_ICONINFORMATION);
			break;
		}
		case ID_ADD_EXTENSION: // Add Extension button
			AddExtension();
			break;
		case ID_FILE_EXIT:
		{
			// Auto save before closing!
			SaveConfig();
			DestroyWindow(m_hwnd);
			break;
		}
		case ID_ABOUT:
			MessageBoxA(m_hwnd, "Image Sorter\nVersion 1.0\n\nCreated by Your Name", "About Image Sorter", MB_OK | MB_ICONINFORMATION);
			break;
		default:
		{
			if (LOWORD(wParam) >= 100 && LOWORD(wParam) < 100 + m_extensions.size())
			{
				UpdateExtensionSelection(LOWORD(wParam));
			}
			break;
		}
		}
		return 0;

	case WM_CTLCOLORBTN:
		return (LRESULT)m_hButtonBrush;

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LRESULT)m_hBackgroundBrush;

	case WM_TIMER:
		if (wParam == 1)
		{
			UpdateProgress();
		}
		return 0;

	case WM_APP:
	{
		std::string* pMessage = reinterpret_cast<std::string*>(lParam);
		AppendToLog(*pMessage);
		delete pMessage;
	}
	return 0;

	case WM_APP + 1:
		KillTimer(m_hwnd, 1);
		UpdateProgress();
		AppendToLog("Image sorting operation completed.");
		return 0;

	case WM_SIZE:
		ResizeControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void ImageSorterGUI::CreateMenu()
{
	HMENU hMenu = ::CreateMenu();
	HMENU hFileMenu = CreatePopupMenu();
	HMENU hHelpMenu = CreatePopupMenu();

	AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Exit");
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");

	AppendMenu(hHelpMenu, MF_STRING, ID_ABOUT, "About");
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");

	SetMenu(m_hwnd, hMenu);
}

LRESULT CALLBACK ImageSorterGUI::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImageSorterGUI* pThis = reinterpret_cast<ImageSorterGUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (pThis)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND:
			if (pThis->m_hButtonBrush)
			{
				HDC hdc = (HDC)wParam;
				RECT rc;
				GetClientRect(hwnd, &rc);
				FillRect(hdc, &rc, pThis->m_hButtonBrush);
				return TRUE;
			}
			break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			if (hdc && pThis->m_hButtonBrush)
			{
				RECT rc;
				GetClientRect(hwnd, &rc);
				FillRect(hdc, &rc, pThis->m_hButtonBrush);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(255, 255, 255));
				char buttonText[256] = { 0 };
				GetWindowTextA(hwnd, buttonText, sizeof(buttonText));
				DrawTextA(hdc, buttonText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_LBUTTONDOWN:
			SendMessage(pThis->m_hwnd, WM_COMMAND, GetWindowLongPtr(hwnd, GWLP_ID), (LPARAM)hwnd);
			return 0;
		}
	}

	// Call the original window procedure
	WNDPROC originalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hwnd, GWLP_USERDATA + sizeof(LONG_PTR)));
	if (originalProc)
		return CallWindowProc(originalProc, hwnd, uMsg, wParam, lParam);

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ImageSorterGUI::UpdateProgress()
{
	SendMessage(m_hProgressBar, PBM_SETPOS, m_progressValue, 0);
}

void ImageSorterGUI::CreateControls()
{
	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE);

	m_hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

	if (!m_hFont)
	{
		MessageBoxA(NULL, "Failed to create font", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	m_hBrowseButton = CreateWindowA("BUTTON", "Browse Folder", WS_VISIBLE | WS_CHILD, 10, 10, 120, 30, m_hwnd, (HMENU)1, hInstance, NULL);
	m_hEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 135, 10, 335, 30, m_hwnd, NULL, hInstance, NULL);
	m_hSortButton = CreateWindowA("BUTTON", "Sort Images", WS_VISIBLE | WS_CHILD, 475, 10, 120, 30, m_hwnd, (HMENU)2, hInstance, NULL);

	if (!m_hBrowseButton || !m_hEdit || !m_hSortButton)
	{
		MessageBoxA(NULL, "Failed to create main controls", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	// Second row
	m_hSaveConfigButton = CreateWindowA("BUTTON", "Save Config", WS_VISIBLE | WS_CHILD, 10, 45, 120, 30, m_hwnd, (HMENU)ID_SAVE_CONFIG, hInstance, NULL);
	HWND hStaticText = CreateWindowA("STATIC", "Select image format:", WS_VISIBLE | WS_CHILD, 135, 50, 150, 25, m_hwnd, NULL, hInstance, NULL);

	if (!m_hSaveConfigButton || !hStaticText)
	{
		MessageBoxA(NULL, "Failed to create secondary controls", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	// Image format buttons
	int buttonWidth = 50;
	int buttonHeight = 25;
	int buttonSpacing = 5;
	int currentX = 290;
	int currentY = 48;

	for (size_t i = 0; i < m_extensions.size(); ++i)
	{
		HWND hButton = CreateWindowA("BUTTON", m_extensions[i].c_str(),
			WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
			currentX, currentY, buttonWidth, buttonHeight,
			m_hwnd, (HMENU)(100 + i), hInstance, NULL);
		if (!hButton)
		{
			MessageBoxA(NULL, "Failed to create extension button", "Error", MB_OK | MB_ICONERROR);
			PostQuitMessage(1);
			return;
		}
		m_extensionButtons.push_back(hButton);
		currentX += buttonWidth + buttonSpacing;
	}

	m_hAddExtensionButton = CreateWindowA("BUTTON", "+", WS_VISIBLE | WS_CHILD,
		currentX, currentY, 25, 25, m_hwnd, (HMENU)ID_ADD_EXTENSION, hInstance, NULL);

	if (!m_hAddExtensionButton)
	{
		MessageBoxA(NULL, "Failed to create add extension button", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	// Progress bar
	m_hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		10, 80, 585, 20,
		m_hwnd, (HMENU)3, hInstance, NULL);

	if (!m_hProgressBar)
	{
		MessageBoxA(NULL, "Failed to create progress bar", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	SendMessage(m_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(m_hProgressBar, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));  // Windows 10 blue

	// Console output
	m_hLogEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
		WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		10, 105, 585, 200, m_hwnd, NULL, hInstance, NULL);

	if (!m_hLogEdit)
	{
		MessageBoxA(NULL, "Failed to create log edit control", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	m_hStatus = CreateWindowA(STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, m_hwnd, NULL, hInstance, NULL);

	if (!m_hStatus)
	{
		MessageBoxA(NULL, "Failed to create status bar", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(1);
		return;
	}

	EnumChildWindows(m_hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
		SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
		return TRUE;
		}, (LPARAM)m_hFont);

	for (HWND hButton : {m_hBrowseButton, m_hSortButton, m_hSaveConfigButton, m_hAddExtensionButton}) {
		SetWindowLongPtr(hButton, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		LONG_PTR originalProc = SetWindowLongPtr(hButton, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ImageSorterGUI::StaticWindowProc));
		SetWindowLongPtr(hButton, GWLP_USERDATA + sizeof(LONG_PTR), originalProc);
	}

	LoadConfig();
	AppendToLog("Application started. Please select a folder and image format.");
}

void ImageSorterGUI::BrowseFolder()
{
	BROWSEINFOA bi = { 0 };
	bi.lpszTitle = "Select Folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.hwndOwner = m_hwnd;  // Set the owner window

	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

	if (pidl != 0)
	{
		char path[MAX_PATH];
		if (SHGetPathFromIDListA(pidl, path))
		{
			SetWindowTextA(m_hEdit, path);
			AppendToLog("Selected folder: " + std::string(path));
		}

		CoTaskMemFree(pidl);
	}
}

void ImageSorterGUI::SortImages()
{
	char path[MAX_PATH];
	GetWindowTextA(m_hEdit, path, MAX_PATH);

	if (strlen(path) == 0 || m_selectedExtensionIndex == -1)
	{
		MessageBoxA(m_hwnd, "Please select a folder and an image format.", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	std::filesystem::path sourceDir = path;
	std::string checkedExtension = m_extensions[m_selectedExtensionIndex];

	AppendToLog("Sorting images...");

	// Start sorting in a separate thread
	if (m_sortingThread.joinable()) m_sortingThread.join();
	m_progressValue = 0;
	m_sortingThread = std::thread([this, sourceDir, checkedExtension]() {
		ImageSorterOperations::sortImagesWithoutMatchingExtensions(
			sourceDir,
			checkedExtension,
			[this](const std::string& message) {
				PostMessage(m_hwnd, WM_APP, 0, reinterpret_cast<LPARAM>(new std::string(message)));
			},
			[this](int value, int max) {
				m_progressValue = static_cast<int>((static_cast<float>(value) / max) * 100);
			}
		);
		PostMessage(m_hwnd, WM_APP + 1, 0, 0);  // Sorting completed
		});

	// Start progress update timer
	SetTimer(m_hwnd, 1, 100, NULL);  // Update every 100ms
}

void ImageSorterGUI::ResizeControls(int width, int height)
{
	int margin = 10;
	int buttonWidth = 120;
	int editWidth = width - 2 * buttonWidth - 3 * margin;

	// First row
	SetWindowPos(m_hBrowseButton, NULL, margin, margin, buttonWidth, 30, SWP_NOZORDER);
	SetWindowPos(m_hEdit, NULL, 2 * margin + buttonWidth, margin, editWidth, 30, SWP_NOZORDER);
	SetWindowPos(m_hSortButton, NULL, width - buttonWidth - margin, margin, buttonWidth, 30, SWP_NOZORDER);

	// Second row
	SetWindowPos(m_hSaveConfigButton, NULL, margin, 2 * margin + 30, buttonWidth, 30, SWP_NOZORDER);

	// Image format buttons
	int formatButtonWidth = 50;
	int formatButtonHeight = 25;
	int formatButtonSpacing = 5;
	int currentX = 290;
	int currentY = 2 * margin + 33;

	for (size_t i = 0; i < m_extensionButtons.size(); ++i)
	{
		SetWindowPos(m_extensionButtons[i], NULL, currentX, currentY, formatButtonWidth, formatButtonHeight, SWP_NOZORDER);
		currentX += formatButtonWidth + formatButtonSpacing;
	}

	SetWindowPos(m_hAddExtensionButton, NULL, currentX, currentY, 25, 25, SWP_NOZORDER);

	// Progress bar
	SetWindowPos(m_hProgressBar, NULL, margin, 3 * margin + 60, width - 2 * margin, 20, SWP_NOZORDER);

	// Console output
	int consoleTop = 4 * margin + 80;
	SetWindowPos(m_hLogEdit, NULL, margin, consoleTop, width - 2 * margin, height - consoleTop - 2 * margin, SWP_NOZORDER);

	// Status bar
	SendMessage(m_hStatus, WM_SIZE, 0, 0);

	InvalidateRect(m_hwnd, NULL, TRUE);
}

void ImageSorterGUI::UpdateExtensionSelection(int buttonId)
{
	m_selectedExtensionIndex = buttonId - 100;
	AppendToLog("Selected image format: " + m_extensions[m_selectedExtensionIndex]);
}

void ImageSorterGUI::AppendToLog(const std::string& message)
{
	int textLength = GetWindowTextLength(m_hLogEdit);
	SendMessage(m_hLogEdit, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength);
	SendMessageA(m_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)(message + "\r\n").c_str());
	SendMessage(m_hLogEdit, EM_SCROLLCARET, 0, 0);
}

void ImageSorterGUI::UpdateProgressBar(int value, int max)
{
	SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, max);
	SendMessage(m_hProgressBar, PBM_SETPOS, value, 0);
}

ImageSorterGUI* ImageSorterGUI::GetThisFromHandle(HWND hwnd)
{
	return (ImageSorterGUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

void ImageSorterGUI::SaveConfig()
{
	char appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, appDataPath)))
	{
		std::string configPath = std::string{ appDataPath } + "\\ImageSorter.cfg";
		std::ofstream configFile{ std::move(configPath) };
		if (configFile.is_open())
		{
			char path[MAX_PATH];
			GetWindowTextA(m_hEdit, path, MAX_PATH);
			configFile << path << std::endl;
			configFile << m_selectedExtensionIndex << std::endl;
			for (const auto& ext : m_extensions)
			{
				configFile << ext << std::endl;
			}
			configFile.close();
		}
	}
}

void ImageSorterGUI::LoadConfig()
{
	char appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, appDataPath)))
	{
		std::string configPath = std::string{ appDataPath } + "\\ImageSorter.cfg";
		std::ifstream configFile{ configPath };
		if (configFile.is_open())
		{
			std::string path{};
			std::getline(configFile, path);
			SetWindowTextA(m_hEdit, path.c_str());

			std::string extensionIndex{};
			std::getline(configFile, extensionIndex);
			m_selectedExtensionIndex = std::stoi(extensionIndex);

			// Updating the last selection of extension
			if (m_selectedExtensionIndex >= 0 && m_selectedExtensionIndex < m_extensionButtons.size())
			{
				SendMessage(m_extensionButtons[m_selectedExtensionIndex], BM_SETCHECK, BST_CHECKED, 0);
			}

			m_extensions.clear();
			std::string ext{};
			while (std::getline(configFile, ext))
			{
				m_extensions.emplace_back(std::move(ext));
			}

			configFile.close();
		}
	}
}

void ImageSorterGUI::AddExtension()
{
	char newExt[10] = { 0 };
	if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDEXTENSION), m_hwnd, AddExtensionDialogProc, (LPARAM)newExt) == IDOK)
	{
		std::string extension = newExt;
		if (!extension.empty())
		{
			if (extension[0] != '.')
				extension = "." + extension;

			m_extensions.push_back(extension);

			// Add new button
			int buttonWidth = 50;
			int buttonHeight = 25;
			int buttonSpacing = 5;
			int currentX = 290 + (buttonWidth + buttonSpacing) * m_extensionButtons.size();
			int currentY = 48;

			HWND hButton = CreateWindowA("BUTTON", extension.c_str(),
				WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
				currentX, currentY, buttonWidth, buttonHeight,
				m_hwnd, (HMENU)(100 + m_extensionButtons.size()), GetModuleHandle(NULL), NULL);
			m_extensionButtons.push_back(hButton);
			SendMessage(hButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);

			// Move the "+" button
			SetWindowPos(m_hAddExtensionButton, NULL,
				currentX + buttonWidth + buttonSpacing, currentY, 25, 25, SWP_NOZORDER);

			SaveExtensionsToConfig();
			AppendToLog("Added new extension: " + extension);
		}
	}
}

void ImageSorterGUI::SaveExtensionsToConfig()
{
	std::ofstream configFile(EXTENSIONS_FILE);
	if (configFile.is_open())
	{
		for (const auto& ext : m_extensions)
		{
			configFile << ext << std::endl;
		}
		configFile.close();
	}
	else
	{
		AppendToLog("Failed to save extensions to config.");
	}
}

void ImageSorterGUI::LoadExtensionsFromConfig()
{
	std::ifstream configFile(EXTENSIONS_FILE);
	if (configFile.is_open())
	{
		m_extensions.clear();
		std::string ext;
		while (std::getline(configFile, ext))
		{
			m_extensions.push_back(ext);
		}
		configFile.close();
	}
	else
	{
		// Default extensions if config file doesn't exist
		m_extensions = { ".jpg", ".png", ".gif", ".bmp", ".tiff", ".raw" };
		AppendToLog("Failed to load extensions from config. Opening the default extensions!");
	}
}

// This function should be outside the class definition
INT_PTR CALLBACK AddExtensionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				GetDlgItemTextA(hDlg, IDC_EXTENSION, (LPSTR)lParam, 10);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}