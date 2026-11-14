#include "autocad/ui/controls/UiEdit.h"

BEGIN_MESSAGE_MAP(UiEdit,CEdit)
    ON_WM_PAINT()
    ON_WM_NCCALCSIZE()
    ON_WM_NCPAINT()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_MESSAGE(WM_PRINT,OnPrint)
END_MESSAGE_MAP()
