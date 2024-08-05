#include "ImageSorterGUI.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		InitCommonControls();

		ImageSorterGUI gui;

		if (!gui.Create(hInstance, "Image Sorter", 640, 400))
		{
			return 0;
		}

		gui.Show(nCmdShow);

		MSG msg = {};
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (int)msg.wParam;
	}
	catch (const std::exception& e)
	{
		MessageBoxA(NULL, e.what(), "Unhandled Exception", MB_OK | MB_ICONERROR);
		return 1;
	}
	catch (...)
	{
		MessageBoxA(NULL, "An unknown error occurred", "Unhandled Exception", MB_OK | MB_ICONERROR);
		return 1;
	}
}
