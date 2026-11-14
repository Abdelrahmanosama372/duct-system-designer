#include "autocad/ui/controls/UiHeading.h"

BEGIN_MESSAGE_MAP(UiHeading,CStatic)
    ON_WM_PAINT()
    ON_MESSAGE(WM_PRINT,OnPrint)
    ON_MESSAGE(WM_PRINTCLIENT,OnPrint)
END_MESSAGE_MAP()
