#include "dpi.h"

//#include <shellscalingapi.h> // GetDpiForMonitor, MDT_EFFECTIVE_DPI
//#pragma comment(lib, "Shcore.lib") // GetDpiForMonitor


Dpi theDpi = {
	.dpi = 96,
	.scaleFactor = 100,
	.edgeCx = 1,
	.edgeCy = 1,
};

void
dpi_refresh(HWND wnd) {
	extern Dpi theDpi;
	int dpi = GetDpiForWindow(wnd);
	dpi_set(dpi);

	//HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTOPRIMARY);
	//if (!monitor) return;

	//int dpiX, dpiY;
	//HRESULT ret = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
	//if (ret != S_OK) return;
}
