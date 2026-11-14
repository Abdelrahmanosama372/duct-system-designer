#include "autocad/ui/controls/UiSeparator.h"

BEGIN_MESSAGE_MAP(UiSeparator,CStatic)
    ON_MESSAGE(WM_PRINT,OnPrintClient)
    ON_WM_PAINT()
    ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
