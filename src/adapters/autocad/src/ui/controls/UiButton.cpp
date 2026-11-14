#include "autocad/ui/controls/UiButton.h"

BEGIN_MESSAGE_MAP(UiButton,CButton)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_ENABLE()
    ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
    ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
