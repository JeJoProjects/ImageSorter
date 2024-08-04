#include "ImageSorterGUI.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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
