#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <atlimage.h>
#include <cstdio>
#include "autocad/ui/panels/NetworkPanel.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/panels/PropertiesPanel.h"
#include <stdexcept>
#include "autocad/ui/panels/DrawingGenerationPanel.h"
#include "autocad/ui/panels/ReportsPanel.h"
#include "autocad/ui/panels/ProjectInformationPanel.h"

namespace {
void* simulatedDrawing=nullptr;
void* DrawingKeyFixture(){return simulatedDrawing;}
int labelRefreshRequests=0;
int propertyFinishRequests=0;
std::optional<NetworkActions::GeometryRequest> geometryRequest;
std::optional<NetworkActions::AnnotationRequest> annotationRequest;
bool CaptureAnnotations(const NetworkActions::AnnotationRequest& request,HWND) {
    annotationRequest=request;return true;
}
std::vector<std::wstring> TextStyleFixture() {return {L"Standard",L"Office HVAC",L"Notes"};}
std::optional<NetworkActions::VisibilityRequest> visibilityRequest;
bool CaptureVisibility(const NetworkActions::VisibilityRequest& request,HWND) {
    visibilityRequest=request;
    for(auto& record:NetworkSession::Drafts())
        if(dsd::application::network_item_id("NET",record.id)==request.network_id)
            record.visualizationVisible=request.visible;
    NetworkSession::Notify();
    return true;
}
std::vector<std::wstring> DrawingLayerFixture() {
    std::vector<std::wstring> layers{L"0",L"M-HVAC-DUCT",L"M-HVAC-CENTERLINE",L"Supply Main"};
    for(int i=0;i<200;++i)layers.push_back(L"Test layer "+std::to_wstring(i));
    return layers;
}
bool CaptureGeometry(const NetworkActions::GeometryRequest& request,HWND) {
    geometryRequest=request;
    return true;
}
bool CaptureLabelRefresh(NetworkActions::Action action,HWND) {
    if(action==NetworkActions::Action::RefreshLabels)++labelRefreshRequests;
    if(action==NetworkActions::Action::FinishPropertyApply)++propertyFinishRequests;
    return true;
}
}
struct ResizeProbe {int attempts=0,sizeMessages=0;};
static LRESULT CALLBACK ProbeScrollResize(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data)
{
    auto* probe=reinterpret_cast<ResizeProbe*>(data);
    if(message==WM_WINDOWPOSCHANGING && !(reinterpret_cast<WINDOWPOS*>(lp)->flags&SWP_NOSIZE))++probe->attempts;
    if(message==WM_SIZE)++probe->sizeMessages;
    return DefSubclassProc(window,message,wp,lp);
}

class ProbePage : public NetworkRegistrationPage
{
public: int ScrollPosition() const { return m_scroll; }
};
static void Pump()
{
    MSG msg;
    while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE))
    {TranslateMessage(&msg);DispatchMessage(&msg);}
}
static void Capture(CWnd& window,LPCTSTR path)
{
    CRect r;window.GetClientRect(&r);
    CImage image;image.Create(r.Width(),r.Height(),32);
    HDC dc=image.GetDC();
    window.SendMessage(WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND|PRF_NONCLIENT);
    image.ReleaseDC();image.Save(path);
}

static bool ScrollingHeader(CWnd& content,UINT headerId)
{
    content.SendMessage(WM_VSCROLL,SB_TOP);Pump();CRect before,after,restored;
    content.GetDlgItem(headerId)->GetWindowRect(&before);
    content.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA*3)),0);Pump();
    content.GetDlgItem(headerId)->GetWindowRect(&after);
    content.SendMessage(WM_VSCROLL,SB_TOP);Pump();content.GetDlgItem(headerId)->GetWindowRect(&restored);
    return after.top<before.top && before==restored;
}

class QaFrame : public CFrameWnd { void PostNcDestroy() override {} };
class QaApp : public CWinApp
{
    int exitCode_=1;
public:
    int ExitInstance() override { CWinApp::ExitInstance(); return exitCode_; }
    BOOL InitInstance() override
    {
        CWinApp::InitInstance();
        UiStyle::StartGraphics();
        const bool fontsReady=Fonts::LoadForModule(AfxGetInstanceHandle());
        INITCOMMONCONTROLSEX common={sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);
        FILE* log=nullptr;_wfopen_s(&log,L"qa.log",L"w");
        if(!log)return FALSE;
        int errors=fontsReady?0:1;
        {
            QaFrame frame;
            if(!frame.Create(nullptr,L"Network controls QA",WS_OVERLAPPEDWINDOW,CRect(0,0,700,900)))return FALSE;
            frame.SetWindowPos(nullptr,-30000,-30000,700,900,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
            ProbePage registration;RegisteredNetworksPage registered;ReportsContent reports;ProjectInformationPage projectInfo;
            NetworkSession::Reset();
            if(!registration.CreatePage(&frame,100) || !registered.CreatePage(&frame,101))return FALSE;
            registration.MoveWindow(0,0,540,700);registration.ShowWindow(SW_SHOW);
            registered.MoveWindow(0,0,540,700);registered.ShowWindow(SW_HIDE);
            Pump();

            ResizeProbe sizingProbe;HWND measuredCombo=registration.GetDlgItem(NC_MIN)->m_hWnd;
            SetWindowSubclass(measuredCombo,ProbeScrollResize,1,reinterpret_cast<DWORD_PTR>(&sizingProbe));
            for(int i=0;i<12;++i){registration.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);registration.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(WHEEL_DELTA)),0);}
            RemoveWindowSubclass(measuredCombo,ProbeScrollResize,1);
            if(sizingProbe.attempts!=0 || sizingProbe.sizeMessages!=0)++errors;
            fwprintf(log,L"Scroll resize probe: %d resize attempts, %d WM_SIZE messages across 24 wheel events\n",sizingProbe.attempts,sizingProbe.sizeMessages);

            CString value;registration.GetDlgItemText(NC_NAME,value);
            if(!value.IsEmpty())++errors;
            if(registration.GetDlgItem(NC_TERMINAL)->IsWindowEnabled() ||
               registration.GetDlgItem(NC_DAMPER)->IsWindowEnabled() ||
               registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
            if(!NetworkSession::Drafts().empty())++errors;
            { CClientDC dc(&registration);auto old=dc.SelectObject(registration.GetDlgItem(NC_TITLE)->GetFont());
              wchar_t face[128]={};GetTextFaceW(dc.m_hDC,128,face);fwprintf(log,L"Title font: %s\n",face);dc.SelectObject(old); }
            for(auto probe : {std::pair<UINT,const wchar_t*>{NC_NAME_LABEL,L"Inter"},
                {NC_ROOT,L"Inter SemiBold"},{NC_TITLE,L"Inter SemiBold"}})
            {
                CClientDC dc(&registration);auto old=dc.SelectObject(registration.GetDlgItem(probe.first)->GetFont());
                wchar_t face[128]={};GetTextFaceW(dc.m_hDC,128,face);TEXTMETRIC metric={};GetTextMetricsW(dc.m_hDC,&metric);
                fwprintf(log,L"Font probe %u: %s, weight %ld\n",probe.first,face,metric.tmWeight);
                if(_wcsicmp(face,probe.second)!=0)++errors;
                dc.SelectObject(old);
            }
            Capture(registration,L"registration-top.png");
            CRect before;registration.GetDlgItem(NC_NAME_LABEL)->GetWindowRect(&before);
            for(int cycle=0;cycle<20;++cycle)
            {
                registration.SendMessage(WM_VSCROLL,SB_PAGEDOWN);
                registration.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);
                registration.SendMessage(WM_VSCROLL,SB_BOTTOM);
                registration.MoveWindow(0,0,cycle%2?540:460,cycle%2?700:450);
                Pump();
                registration.SendMessage(WM_VSCROLL,SB_TOP);
                if(registration.ScrollPosition()!=0)++errors;
            }
            registration.MoveWindow(0,0,540,700);Pump();
            CRect after;registration.GetDlgItem(NC_NAME_LABEL)->GetWindowRect(&after);
            if(before!=after)++errors;
            if(registration.GetStyle()&WS_VSCROLL)++errors;
            CWnd* rail=registration.GetDlgItem(2900);CRect railRect;rail->GetClientRect(&railRect);
            registration.SendMessage(WM_VSCROLL,SB_TOP);
            rail->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(7,railRect.bottom-10));
            if(registration.ScrollPosition()<=0)++errors;
            registration.SendMessage(WM_VSCROLL,SB_TOP);
            rail->SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(7,12));
            rail->SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(7,railRect.bottom-12));
            rail->SendMessage(WM_LBUTTONUP,0,MAKELPARAM(7,railRect.bottom-12));
            if(registration.ScrollPosition()<=0)++errors;
            registration.SendMessage(WM_VSCROLL,SB_TOP);
            rail->SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);
            if(registration.ScrollPosition()<=0)++errors;
            registration.SendMessage(WM_VSCROLL,SB_TOP);Pump();
            Capture(registration,L"registration-after-scroll.png");
            registration.SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();
            if(registration.ScrollPosition()<=0)++errors;
            Capture(registration,L"registration-bottom.png");
            // Explicit fixtures for card/search/report checks; product startup remains empty.
            for(int i=0;i<3;++i)
            {
                NetworkDraft fixture;fixture.id=i+1;
                fixture.name=i==0?L"Supply Network 01":i==1?L"Return Network 01":L"Exhaust Network 01";
                fixture.system=i==0?L"Supply Air":i==1?L"Return Air":L"Exhaust Air";
                fixture.segmentCount=L"3";fixture.totalFlow=L"0 CFM";fixture.status=L"Fixture";
                using namespace dsd::application;
                DuctNetwork data;data.id=network_item_id("NET",i+1);
                data.name=std::wstring(fixture.name.GetString());
                data.root=SourcePath{"Fixture",L"Ducts",{{0,0},{1000,0},{1000,1000},{2000,1000}}};
                data.lines={*data.root};
                data=ProjectInfo::Application().prepare_network(data,{});
                for(auto& segment:data.segmented_lines_infos)segment.metadata={{"width","300"},{"height","200"},{"flow","0"}};
                ProjectInfo::Application().register_network(data);
                NetworkSession::Drafts().push_back(fixture);
            }
            registered.Rebuild();
            registration.ShowWindow(SW_HIDE);registered.ShowWindow(SW_SHOW);Pump();
            registered.GetDlgItem(6000)->SetFocus();Pump();
            Capture(registered,L"registered-networks.png");
            registered.SendMessage(WM_COMMAND,MAKEWPARAM(6000,BN_CLICKED),0);Pump();
            { CString title;registered.GetDlgItem(6000)->GetWindowText(title);CRect r;registered.GetDlgItem(6000)->GetClientRect(&r);
              fwprintf(log,L"Collapsed title: %s (client %d x %d)\n",title.GetString(),r.Width(),r.Height()); }
            NetworkActions::SetVisibilityHandler(CaptureVisibility);
            registered.SendDlgItemMessage(9000,BM_CLICK);Pump();
            if(!visibilityRequest || visibilityRequest->network_id!="NET-001" || visibilityRequest->visible ||
                NetworkSession::Drafts()[0].visualizationVisible)++errors;
            if(static_cast<UiButton*>(registered.GetDlgItem(9000))->GetIcon()!=UiButton::Icon::EyeOff ||
                (registered.GetDlgItem(8000)->GetStyle()&WS_VISIBLE))++errors;
            registered.SendDlgItemMessage(10000,BM_CLICK);Pump();
            if(registered.GetDlgItem(8000)->GetStyle()&WS_VISIBLE)++errors;
            NetworkSession::EditRegistered(0);
            if(NetworkSession::Current().form.visualizationVisible)++errors;
            NetworkSession::ResetDraft();
            if(NetworkSession::Drafts()[0].visualizationVisible)++errors;
            registered.SendDlgItemMessage(9000,BM_CLICK);Pump();
            if(!NetworkSession::Drafts()[0].visualizationVisible ||
                static_cast<UiButton*>(registered.GetDlgItem(9000))->GetIcon()!=UiButton::Icon::Eye)++errors;
            NetworkSession::Current().busy=true;NetworkSession::Notify();
            if(registered.GetDlgItem(9000)->IsWindowEnabled())++errors;
            NetworkSession::Current().busy=false;NetworkSession::Notify();
            NetworkActions::SetVisibilityHandler(nullptr);
            fwprintf(log,L"Network toolbar: hide/show, icon state, collapsed card retention, Modify visibility retention, busy state. Errors: %d\n",errors);
            Capture(registered,L"registered-collapsed.png");
            registered.SendMessage(WM_COMMAND,MAKEWPARAM(6000,BN_CLICKED),0);Pump();
            CRect buttonRect;registered.GetDlgItem(8000)->GetWindowRect(&buttonRect);
            if(!registered.GetDlgItem(8000)->IsWindowVisible() && ::IsWindowVisible(frame.m_hWnd))++errors;
            registration.SendDlgItemMessage(NC_FLOW,BM_CLICK);Pump();
            if(registration.IsDlgButtonChecked(NC_FLOW)!=BST_CHECKED)++errors;
            registration.SendDlgItemMessage(NC_FLOW,BM_CLICK);Pump();
            if(registration.IsDlgButtonChecked(NC_FLOW)!=BST_UNCHECKED)++errors;
            registered.SetDlgItemText(NC_SEARCH,L"return");Pump();
            registered.GetDlgItemText(NC_COUNT,value);if(value!=L"1 of 3 networks")++errors;
            registered.SetDlgItemText(NC_SEARCH,L"unmatched");Pump();
            registered.GetDlgItemText(NC_COUNT,value);if(value!=L"0 of 3 networks")++errors;
            if(registered.GetDlgItem(2900)->GetStyle()&WS_VISIBLE)++errors;
            registered.SetDlgItemText(NC_SEARCH,L"");Pump();
            registered.GetDlgItemText(NC_COUNT,value);if(value!=L"3 of 3 networks")++errors;
            if(!(registered.GetDlgItem(2900)->GetStyle()&WS_VISIBLE))++errors;

            {
                registered.ShowWindow(SW_HIDE);registration.ShowWindow(SW_SHOW);Pump();
                for(auto item:{std::pair<UINT,const wchar_t*>{NC_METHOD,L"Equal Friction"},{NC_INCREMENT,L"25"},{NC_MIN,L"100"},
                    {NC_MAX,L"2000"},{NC_ASPECT,L"2.0"},{NC_HEAD,L"0.1"},{NC_VELOCITY,L"1000"},
                    {NC_MATERIAL,L"Galvanized Steel"},{NC_ROUGHNESS,L"0.15"}})
                {registration.GetDlgItemText(item.first,value);if(value!=item.second)++errors;}
                auto visible=[&](UINT id){return (registration.GetDlgItem(id)->GetStyle()&WS_VISIBLE)!=0;};
                if(!visible(NC_HEAD) || visible(NC_VELOCITY) || !visible(NC_HEAD_LABEL) || visible(NC_VELOCITY_LABEL))++errors;
                auto* method=static_cast<CComboBox*>(registration.GetDlgItem(NC_METHOD));
                auto setMethod=[&](int row){method->SetCurSel(row);registration.SendMessage(WM_COMMAND,MAKEWPARAM(NC_METHOD,CBN_SELCHANGE),reinterpret_cast<LPARAM>(method->m_hWnd));Pump();};
                CRect methodBox,target;method->GetWindowRect(&methodBox);registration.GetDlgItem(NC_HEAD)->GetWindowRect(&target);
                if(target.top<methodBox.bottom || target.top-methodBox.bottom>32)++errors;
                using namespace dsd::application;
                const SourcePath root{"QA",L"Ducts",{{0,0},{2000,0}}};
                DuctNetwork draft;draft.id="NET-004";draft.root=root;draft.lines={root};
                NetworkSession::Current().form.id=4;NetworkSession::NextId()=5;
                NetworkSession::Current().network=std::make_unique<DuctNetwork>(
                    ProjectInfo::Application().prepare_network(draft,{}));
                registration.SendMessage(WM_DSD_NETWORK_UPDATED);Pump();
                if(!static_cast<UiButton*>(registration.GetDlgItem(NC_ROOT))->SelectionComplete())++errors;
                if(!registration.GetDlgItem(NC_TERMINAL)->IsWindowEnabled() ||
                   !registration.GetDlgItem(NC_DAMPER)->IsWindowEnabled())++errors;
                NetworkSession::Current().network->air_terminal_block_name=L"Terminal";
                NetworkSession::Current().network->damper_block_name=L"Damper";
                registration.SendMessage(WM_DSD_NETWORK_UPDATED);Pump();
                if(!static_cast<UiButton*>(registration.GetDlgItem(NC_TERMINAL))->SelectionComplete() ||
                   !static_cast<UiButton*>(registration.GetDlgItem(NC_DAMPER))->SelectionComplete())++errors;
                NetworkActions::SetHandler(CaptureLabelRefresh);
                labelRefreshRequests=0;
                for(UINT field:{NC_FLOW,NC_DIMENSIONS,NC_BOD}) {
                    registration.SendDlgItemMessage(field,BM_CLICK);Pump();
                    registration.SendDlgItemMessage(field,BM_CLICK);Pump();
                }
                if(labelRefreshRequests!=6)++errors;
                NetworkActions::SetHandler(nullptr);
                registration.SetDlgItemText(NC_NAME,L"Test network");
                registration.SetDlgItemText(NC_HEAD,L"invalid");setMethod(1);
                if(visible(NC_HEAD) || !visible(NC_VELOCITY))++errors;
                registration.SetDlgItemText(NC_VELOCITY,L"1250");registration.SetDlgItemText(NC_ASPECT,L"0.5");
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();registration.GetDlgItemText(NC_STATUS,value);
                if(value.Find(L"at least 1.0")<0)++errors;
                registration.SetDlgItemText(NC_ASPECT,L"2.0");registration.SetDlgItemText(NC_ROUGHNESS,L"-1");
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();registration.GetDlgItemText(NC_STATUS,value);
                if(value.Find(L"non-negative")<0)++errors;
                registration.SetDlgItemText(NC_ROUGHNESS,L"0.15");registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                registration.GetDlgItemText(NC_STATUS,value);if(value.Find(L"DUCT-001")<0)++errors;
                if(registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                registration.SendMessage(WM_COMMAND,MAKEWPARAM(NC_REGISTER,BN_CLICKED),0);Pump();
                if(NetworkSession::Drafts().size()!=3)++errors;
                if(NetworkSession::Current().form.velocity!=L"1250")++errors;
                PropertiesSession::Select({"NET-004",{"DUCT-001"},NetworkItemType::Segment});
                PropertiesSession::Apply({"NET-004",{"DUCT-001"},NetworkItemType::Segment},{{"flow","300"}});
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                if(!registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                NetworkSession::UpdateForm(NetworkSession::Current().form);
                if(!NetworkSession::CanRegister())++errors;
                // A changed form value invalidates analysis immediately.
                registration.SetDlgItemText(NC_VELOCITY,L"1300");Pump();
                if(registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                registration.SetDlgItemText(NC_VELOCITY,L"1250");
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                PropertiesSession::Select({"NET-004",{"DUCT-001"},NetworkItemType::Segment});
                PropertiesSession::Apply({"NET-004",{"DUCT-001"},NetworkItemType::Segment},{{"flow","400"}});
                if(registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                // Selection temporarily blocks Register without changing the analysis.
                NetworkSession::Current().busy=true;
                if(NetworkSession::CanRegister())++errors;
                NetworkSession::Current().busy=false;
                if(!NetworkSession::CanRegister())++errors;
                NetworkSession::InvalidateAnalysis();
                if(registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                registration.SendDlgItemMessage(NC_REGISTER,BM_CLICK);Pump();
                if(NetworkSession::Current().network || NetworkSession::CanRegister())++errors;
                if(NetworkSession::Drafts().size()!=4 || NetworkSession::Drafts().back().totalFlow!=L"400 CFM")++errors;
                const auto& savedSizing=ProjectInfo::Application().registered_network("NET-004").sizing;
                if(savedSizing.method!=NetworkSizingMethod::EqualVelocity || savedSizing.velocity_fpm!=1250 ||
                    savedSizing.roughness_mm!=0.15)++errors;
                registration.GetDlgItemText(NC_NAME,value);if(!value.IsEmpty())++errors;
                registered.Rebuild();registered.GetDlgItemText(NC_COUNT,value);if(value!=L"4 of 4 networks")++errors;
                // Loading for modification copies geometry and metadata and locks the name.
                registration.LoadDraft(3);Pump();
                if(!(registration.GetDlgItem(NC_NAME)->GetStyle()&ES_READONLY) || NetworkSession::CanRegister())++errors;
                registration.GetDlgItemText(NC_VELOCITY,value);if(value!=L"1250")++errors;
                PropertiesSession::Select({"NET-004",{"DUCT-001"},NetworkItemType::Segment});
                PropertiesSession::Apply({"NET-004",{"DUCT-001"},NetworkItemType::Segment},{{"flow","700"}});
                if(network_total_flow(ProjectInfo::Application().registered_network("NET-004"))!=400)++errors;
                registration.NewDraft();
                if(network_total_flow(ProjectInfo::Application().registered_network("NET-004"))!=400)++errors;
                registration.LoadDraft(3);Pump();
                PropertiesSession::Select({"NET-004",{"DUCT-001"},NetworkItemType::Segment});
                PropertiesSession::Apply({"NET-004",{"DUCT-001"},NetworkItemType::Segment},{{"flow","800"}});
                registration.SendDlgItemMessage(NC_ANALYZE,BM_CLICK);Pump();
                registration.SendDlgItemMessage(NC_REGISTER,BM_CLICK);Pump();
                if(NetworkSession::Drafts().size()!=4 || NetworkSession::Drafts().back().totalFlow!=L"800 CFM")++errors;
                if(network_total_flow(ProjectInfo::Application().registered_network("NET-004"))!=800)++errors;
                if(registration.GetDlgItem(NC_NAME)->GetStyle()&ES_READONLY)++errors;
                registration.NewDraft();
                if(NetworkSession::Current().network || registration.GetDlgItem(NC_TERMINAL)->IsWindowEnabled() ||
                   registration.GetDlgItem(NC_DAMPER)->IsWindowEnabled() || registration.GetDlgItem(NC_REGISTER)->IsWindowEnabled())++errors;
                if(static_cast<UiButton*>(registration.GetDlgItem(NC_ROOT))->SelectionComplete())++errors;
                registration.SetDlgItemText(NC_NAME,L"New network");
                registration.SendMessage(WM_VSCROLL,SB_TOP);Pump();
                for(UINT id:{NC_NAME,NC_HEAD,NC_ASPECT,NC_ROUGHNESS})
                {CRect c,w,f;auto* edit=registration.GetDlgItem(id);edit->GetClientRect(&c);edit->GetWindowRect(&w);edit->SendMessage(EM_GETRECT,0,reinterpret_cast<LPARAM>(&f));edit->GetWindowText(value);fwprintf(log,L"Edit %u [%s] client %d,%d format %d,%d,%d,%d window %d,%d\n",id,value.GetString(),c.Width(),c.Height(),f.left,f.top,f.right,f.bottom,w.Width(),w.Height());}
                Capture(registration,L"network-sizing-top.png");
                registration.SendMessage(WM_UI_SCROLL_TO,450);Pump();Capture(registration,L"network-sizing-friction.png");
                setMethod(1);Pump();Capture(registration,L"network-sizing-velocity.png");setMethod(0);
                registration.SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();Capture(registration,L"network-sizing-bottom.png");
                registration.SendMessage(WM_VSCROLL,SB_TOP);Pump();
                fwprintf(log,L"Network sizing: defaults, conditional fields, method value retention, numeric validation, root/equipment states, analysis invalidation, registration, root flow, modification copies and reset. Errors: %d\n",errors);
            }
            fwprintf(log,L"Native controls: 20 scroll/resize cycles, original label positions restored; dark scrollbar drag, track click, wheel and auto-hide; explicit card fixtures and search checked. Errors: %d\n",errors);
            registration.ShowWindow(SW_HIDE);registered.ShowWindow(SW_HIDE);
            {
                using namespace dsd::application;
                auto draft=DuctNetwork{};draft.id="NET-QA";draft.name=L"Properties QA";
                draft.root=SourcePath{"QA",L"Ducts",{{0,0},{10000,0}}};draft.lines={*draft.root};
                draft.air_terminal_block_name=L"Terminal";draft.damper_block_name=L"Damper";
                const std::vector<dsd::geometry::Block> blocks{
                    {"T",L"Terminal",{2000,0},{{1980,-10},{2020,10}}},
                    {"T2",L"Terminal",{4000,0},{{3980,-10},{4020,10}}},
                    {"D",L"Damper",{6000,0},{{5980,-10},{6020,10}}},
                    {"D2",L"Damper",{8000,0},{{7980,-10},{8020,10}}}};
                NetworkSession::Current().network=std::make_unique<DuctNetwork>(
                    ProjectInfo::Application().prepare_network(draft,blocks));
                NetworkActions::SetHandler(CaptureLabelRefresh);
                labelRefreshRequests=0;propertyFinishRequests=0;
                PropertiesContent content;
                if(!content.CreateView(&frame,106))++errors;
                else {
                    content.MoveWindow(0,0,540,940);content.ShowWindow(SW_SHOW);Pump();
                    auto* tabs=static_cast<CTabCtrl*>(content.GetDlgItem(3701));
                    CWnd* segment=content.GetDlgItem(3801);
                    CWnd* terminal=content.GetDlgItem(3802);
                    CWnd* damper=content.GetDlgItem(3803);
                    if(segment->GetDlgItem(PC_APPLY)->IsWindowEnabled())++errors;
                    PropertiesSession::Select({"NET-QA",{"DUCT-001"},NetworkItemType::Segment});Pump();
                    if(tabs->GetCurSel()!=0)++errors;
                    segment->GetDlgItemText(PC_FLOW,value);if(value!=L"0")++errors;
                    segment->GetDlgItemText(PC_WIDTH,value);if(value!=L"AC")++errors;
                    segment->SetDlgItemText(PC_FLOW,L"350.5");segment->SendDlgItemMessage(PC_APPLY,BM_CLICK);Pump();
                    if(NetworkSession::Current().network->segmented_lines_infos.front().metadata.at("flow")!="350.5")++errors;
                    if(PropertiesSession::Read() || segment->GetDlgItem(PC_APPLY)->IsWindowEnabled() || tabs->GetCurSel()!=0)++errors;
                    segment->GetDlgItemText(PC_SEGMENT,value);if(!value.IsEmpty())++errors;
                    if(propertyFinishRequests!=1 || labelRefreshRequests!=0)++errors;
                    PropertiesSession::Select({"NET-QA",{"DUCT-001"},NetworkItemType::Segment});Pump();
                    segment->SetDlgItemText(PC_FLOW,L"-1");segment->SendDlgItemMessage(PC_APPLY,BM_CLICK);Pump();
                    if(!PropertiesSession::Read() || !segment->GetDlgItem(PC_APPLY)->IsWindowEnabled())++errors;
                    segment->GetDlgItemText(PC_FLOW,value);if(value!=L"-1")++errors;
                    if(propertyFinishRequests!=1)++errors;
                    if(NetworkSession::Current().network->segmented_lines_infos.front().metadata.at("flow")!="350.5")++errors;
                    segment->SetDlgItemText(PC_FLOW,L"999");
                    PropertiesSession::Select({"NET-QA",{"AT-001"},NetworkItemType::AirTerminal});Pump();
                    if(tabs->GetCurSel()!=1)++errors;
                    terminal->GetDlgItemText(EP_SECOND,value);if(value!=L"none")++errors;
                    terminal->SetDlgItemText(EP_FIRST,L"250");terminal->SetDlgItemText(EP_SECOND,L"200 x 150");
                    terminal->SetDlgItemText(EP_THIRD,L"300 x 300");terminal->SendDlgItemMessage(EP_APPLY,BM_CLICK);Pump();
                    const auto& terminalValues=NetworkSession::Current().network->air_terminal_blocks_infos.front().metadata;
                    if(terminalValues.at("width")!="200" || terminalValues.at("height")!="150" || terminalValues.at("face_size")!="300 x 300")++errors;
                    if(PropertiesSession::Read() || terminal->GetDlgItem(EP_APPLY)->IsWindowEnabled() || tabs->GetCurSel()!=1)++errors;
                    Capture(content,L"properties-air-terminals.png");
                    PropertiesSession::Select({"NET-QA",{"VCD-001"},NetworkItemType::Damper});Pump();
                    if(tabs->GetCurSel()!=2)++errors;
                    damper->GetDlgItemText(EP_THIRD,value);if(value!=L"AC")++errors;
                    auto* types=static_cast<CComboBox*>(damper->GetDlgItem(EP_FIRST));types->SetCurSel(types->FindStringExact(-1,L"Fire Damper"));
                    damper->SetDlgItemText(EP_THIRD,L"500");damper->SendDlgItemMessage(EP_APPLY,BM_CLICK);Pump();
                    const auto& damperValues=NetworkSession::Current().network->damper_blocks_infos.front().metadata;
                    if(damperValues.at("width")!="500" || damperValues.at("damper_type")!="Fire Damper")++errors;
                    if(PropertiesSession::Read() || damper->GetDlgItem(EP_APPLY)->IsWindowEnabled() || tabs->GetCurSel()!=2)++errors;
                    if(propertyFinishRequests!=3 || labelRefreshRequests!=0)++errors;
                    Capture(content,L"properties-dampers.png");
                    PropertiesSession::Select({"NET-QA",{"DUCT-001"},NetworkItemType::Segment});Pump();
                    segment->GetDlgItemText(PC_FLOW,value);if(value!=L"350.5")++errors;
                    Capture(content,L"properties-preview.png");
                    for(int i=0;i<12;++i){content.MoveWindow(0,0,i%2?540:460,i%2?740:340);segment->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();segment->SendMessage(WM_VSCROLL,SB_TOP);Pump();}
                    // Batch selections retain the first item's values and deduplicate IDs.
                    auto& batch=*NetworkSession::Current().network;
                    update_network_item_metadata(batch,NetworkItemType::Segment,"DUCT-002",{{"flow","120"},{"width","300"},{"height","200"}});
                    PropertiesSession::Select({"NET-QA",{"DUCT-002","DUCT-001","DUCT-002"},NetworkItemType::Segment});Pump();
                    segment->GetDlgItemText(PC_SEGMENT,value);if(value!=L"DUCT-002 - DUCT-001")++errors;
                    segment->GetDlgItemText(PC_FLOW,value);if(value!=L"120")++errors;
                    segment->GetDlgItemText(PC_STATUS,value);if(value.Find(L"DUCT-002")<0 || value.Find(L"2 selected")<0)++errors;
                    segment->SetDlgItemText(PC_FLOW,L"600");segment->SendDlgItemMessage(PC_APPLY,BM_CLICK);Pump();
                    for(const auto id:{"DUCT-001","DUCT-002"}) {
                        const auto values=read_network_item_metadata(batch,NetworkItemType::Segment,id);
                        if(values.at("flow")!="600" || values.at("width")!="300" || values.at("height")!="200")++errors;
                    }
                    if(PropertiesSession::Read() || propertyFinishRequests!=4 || tabs->GetCurSel()!=0)++errors;
                    update_network_item_metadata(batch,NetworkItemType::AirTerminal,"AT-002",{{"flow","150"},{"width","225"},{"height","175"},{"face_size","400 x 400"}});
                    PropertiesSession::Select({"NET-QA",{"AT-002","AT-001","AT-001"},NetworkItemType::AirTerminal});Pump();
                    terminal->GetDlgItemText(EP_SELECTED,value);if(value!=L"AT-002 - AT-001")++errors;
                    terminal->GetDlgItemText(EP_SECOND,value);if(value!=L"225 x 175")++errors;
                    terminal->SetDlgItemText(EP_FIRST,L"300");terminal->SendDlgItemMessage(EP_APPLY,BM_CLICK);Pump();
                    for(const auto id:{"AT-001","AT-002"}) {
                        const auto values=read_network_item_metadata(batch,NetworkItemType::AirTerminal,id);
                        if(values.at("flow")!="300" || values.at("width")!="225" || values.at("height")!="175" || values.at("face_size")!="400 x 400")++errors;
                    }
                    if(PropertiesSession::Read() || propertyFinishRequests!=5 || tabs->GetCurSel()!=1)++errors;
                    update_network_item_metadata(batch,NetworkItemType::Damper,"VCD-002",{{"damper_type","Smoke Damper"},{"blade_arrangement","Opposed Blade"},{"width","600"},{"height","350"}});
                    PropertiesSession::Select({"NET-QA",{"VCD-002","VCD-001"},NetworkItemType::Damper});Pump();
                    damper->GetDlgItemText(EP_SELECTED,value);if(value!=L"VCD-002 - VCD-001")++errors;
                    damper->SetDlgItemText(EP_THIRD,L"750");damper->SendDlgItemMessage(EP_APPLY,BM_CLICK);Pump();
                    for(const auto id:{"VCD-001","VCD-002"}) {
                        const auto values=read_network_item_metadata(batch,NetworkItemType::Damper,id);
                        if(values.at("damper_type")!="Smoke Damper" || values.at("blade_arrangement")!="Opposed Blade" || values.at("width")!="750" || values.at("height")!="350")++errors;
                    }
                    if(PropertiesSession::Read() || propertyFinishRequests!=6 || labelRefreshRequests!=0 || tabs->GetCurSel()!=2)++errors;
                    PropertiesSession::Select({"NET-QA",{"AT-001","AT-002"},NetworkItemType::AirTerminal});Pump();
                    terminal->SetDlgItemText(EP_FIRST,L"-1");terminal->SendDlgItemMessage(EP_APPLY,BM_CLICK);Pump();
                    if(!PropertiesSession::Read() || PropertiesSession::Read()->selection.item_ids.size()!=2 || propertyFinishRequests!=6)++errors;
                    if(batch.air_terminal_blocks_infos[0].metadata.at("flow")!="300" || batch.air_terminal_blocks_infos[1].metadata.at("flow")!="300")++errors;
                    PropertiesSession::Select({"NET-QA",{"DUCT-001","DUCT-002"},NetworkItemType::Segment});
                    const auto previous=*PropertiesSession::Read();
                    PropertiesSession::Select({"NET-QA",{"DUCT-001","DUCT-003"},NetworkItemType::Segment});
                    try{PropertiesSession::Apply(previous.selection,{{"flow","999"}});++errors;}catch(const std::invalid_argument&){}
                    if(batch.segmented_lines_infos[0].metadata.at("flow")!="600" || batch.segmented_lines_infos[2].metadata.at("flow")!="0")++errors;
                    PropertiesSession::Select({"NET-QA",{},NetworkItemType::Segment});
                    if(PropertiesSession::Read() || segment->GetDlgItem(PC_APPLY)->IsWindowEnabled())++errors;
                    const PropertiesSession::Selection stale{"NET-QA",{"DUCT-001"},NetworkItemType::Segment};
                    NetworkSession::ResetDraft();Pump();
                    if(segment->GetDlgItem(PC_APPLY)->IsWindowEnabled())++errors;
                    try{PropertiesSession::Apply(stale,{{"flow","123"}});++errors;}catch(const std::invalid_argument&){}
                    content.DestroyWindow();
                }
                NetworkActions::SetHandler(nullptr);
                fwprintf(log,L"Properties: actual metadata, automatic subtabs, Apply clears selection while retaining the tab, failed edits retain selection, discarded edits, reset, stale selection and resize/scroll checks. Errors: %d\n",errors);
            }

            {
                DrawingGenerationContent content;
                NetworkActions::SetGeometryHandler(CaptureGeometry);
                NetworkActions::SetLayerProvider(DrawingLayerFixture);
                NetworkActions::SetTextStyleProvider(TextStyleFixture);
                NetworkActions::SetAnnotationHandler(CaptureAnnotations);
                if(!content.CreateView(&frame,107))++errors;
                else
                {
                    content.MoveWindow(0,0,540,1000);content.ShowWindow(SW_SHOW);Pump();
                    auto* tabs=static_cast<CTabCtrl*>(content.GetDlgItem(6501));
                    CWnd* geometry=content.GetDlgItem(6601);CWnd* annotations=content.GetDlgItem(6602);
                    auto change=[&](int index){tabs->SetCurSel(index);NMHDR note={tabs->m_hWnd,6501,TCN_SELCHANGE};content.SendMessage(WM_NOTIFY,6501,reinterpret_cast<LPARAM>(&note));Pump();};
                    if(tabs->GetItemCount()!=2)++errors;

                    auto* generate=static_cast<UiButton*>(geometry->GetDlgItem(DG_GENERATE));
                    auto* remove=static_cast<UiButton*>(geometry->GetDlgItem(DG_REMOVE));
                    if(generate->Role()!=UiStyle::ButtonRole::Generation || remove->Role()!=UiStyle::ButtonRole::Removal)++errors;
                    if(UiStyle::MainBackground!=RGB(43,50,61) || UiStyle::SectionBackground!=RGB(54,64,78) ||
                        UiStyle::Blue!=RGB(57,143,235) || UiStyle::Green!=RGB(46,162,118) || UiStyle::Danger!=RGB(231,107,107))++errors;
                    geometry->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();
                    Capture(content,L"modern-actions-default.png");
                    generate->SendMessage(WM_MOUSEMOVE,0,MAKELPARAM(20,15));Pump();Capture(content,L"modern-actions-hover.png");
                    generate->SendMessage(BM_SETSTATE,TRUE);Pump();Capture(content,L"modern-actions-pressed.png");
                    generate->SendMessage(BM_SETSTATE,FALSE);generate->SendMessage(WM_MOUSELEAVE);generate->EnableWindow(FALSE);Pump();Capture(content,L"modern-actions-disabled.png");
                    generate->EnableWindow(TRUE);generate->SetFocus();Pump();Capture(content,L"modern-actions-focused.png");
                    geometry->SendMessage(WM_VSCROLL,SB_TOP);Pump();
                    geometry->GetDlgItemText(DG_REP,value);if(value!=L"Duct lines")++errors;
                    auto* representation=static_cast<CComboBox*>(geometry->GetDlgItem(DG_REP));
                    if(representation->GetCount()!=2)++errors;
                    if(geometry->GetDlgItem(DG_CENTERLINE_LAYER)->GetStyle()&WS_VISIBLE)++errors;
                    if(geometry->GetDlgItem(DG_CENTERLINE_LAYER_LABEL)->GetStyle()&WS_VISIBLE)++errors;
                    geometry->GetDlgItemText(DG_REDUCER,value);if(value!=L"300")++errors;
                    auto* layerSearch=static_cast<SearchableComboBox*>(geometry->GetDlgItem(DG_DUCT_LAYER));
                    // The legacy common-controls host does not implement CB_GETMINVISIBLE (returns zero).
                    const auto minVisible=layerSearch->SendMessage(CB_GETMINVISIBLE);
                    if(layerSearch->GetCount()!=204 || (minVisible!=0 && minVisible!=6))++errors;
                    layerSearch->SetWindowText(L"sUpPlY");
                    geometry->SendMessage(WM_COMMAND,MAKEWPARAM(DG_DUCT_LAYER,CBN_EDITCHANGE),reinterpret_cast<LPARAM>(layerSearch->m_hWnd));
                    if(layerSearch->GetCount()!=1)++errors;
                    CString match;layerSearch->GetLBText(0,match);if(match!=L"Supply Main")++errors;
                    std::wstring selectedLayer;if(layerSearch->SelectedValue(selectedLayer))++errors;
                    layerSearch->ShowDropDown(FALSE);
                    layerSearch->SetWindowText(L"M-HVAC-DUCT");
                    auto* networks=static_cast<CComboBox*>(geometry->GetDlgItem(DG_NETWORK));
                    if(networks->GetCount()!=4)++errors;
                    if(networks->GetItemHeight(-1)<28 || networks->GetItemHeight(0)<30)++errors;
                    CRect comboRect;networks->GetWindowRect(&comboRect);if(comboRect.Height()<32)++errors;
                    networks->SetCurSel(1);geometry->SendMessage(WM_COMMAND,MAKEWPARAM(DG_NETWORK,CBN_SELCHANGE),reinterpret_cast<LPARAM>(networks->m_hWnd));
                    geometry->SetDlgItemText(DG_REDUCER,L"-1");geometry->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    geometry->GetDlgItemText(DG_STATUS,value);if(value.Find(L"positive value")<0)++errors;
                    geometry->SetDlgItemText(DG_REDUCER,L"350");geometry->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    if(!geometryRequest || geometryRequest->remove || geometryRequest->reducer_length_mm!=350 ||
                        geometryRequest->elbow_inside_radius_ratio!=1.5 || geometryRequest->network_id!="NET-002" ||
                        geometryRequest->representation!=NetworkActions::GeometryRepresentation::Outline)++errors;
                    const auto chooseRepresentation=[&](int index) {
                        representation->SetCurSel(index);
                        geometry->SendMessage(WM_COMMAND,MAKEWPARAM(DG_REP,CBN_SELCHANGE),
                            reinterpret_cast<LPARAM>(representation->m_hWnd));Pump();
                    };
                    chooseRepresentation(1);
                    if(!(geometry->GetDlgItem(DG_CENTERLINE_LAYER)->GetStyle()&WS_VISIBLE) ||
                        !(geometry->GetDlgItem(DG_CENTERLINE_LAYER_LABEL)->GetStyle()&WS_VISIBLE))++errors;
                    geometry->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    if(!geometryRequest || geometryRequest->representation!=NetworkActions::GeometryRepresentation::OutlineWithCenterlines ||
                        geometryRequest->centerline_layer!=L"M-HVAC-CENTERLINE" || geometryRequest->duct_layer!=L"M-HVAC-DUCT")++errors;
                    chooseRepresentation(0);
                    if(geometry->GetDlgItem(DG_CENTERLINE_LAYER)->GetStyle()&WS_VISIBLE)++errors;
                    geometry->GetDlgItemText(DG_ELBOW,value);if(value!=L"1.5")++errors;
                    geometry->SendMessage(WM_VSCROLL,SB_TOP);Pump();Capture(content,L"drawing-geometry-top.png");
                    geometry->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();Capture(content,L"drawing-geometry-bottom.png");
                    change(1);annotations->GetDlgItemText(DG_NETWORK,value);if(value!=L"Return Network 01")++errors;
                    for(UINT id:{DG_DIM,DG_FLOW,DG_BOD})if(annotations->IsDlgButtonChecked(id)!=BST_CHECKED)++errors;
                    for(UINT id:{DG_DIM,DG_FLOW,DG_BOD})annotations->SendDlgItemMessage(id,BM_CLICK);
                    annotations->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    annotations->GetDlgItemText(DG_STATUS,value);if(value.Find(L"at least one")<0)++errors;
                    for(UINT id:{DG_DIM,DG_FLOW,DG_BOD})annotations->SendDlgItemMessage(id,BM_CLICK);
                    annotations->SetDlgItemText(DG_TEXT_HEIGHT,L"0");annotations->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    annotations->GetDlgItemText(DG_STATUS,value);if(value.Find(L"positive value")<0)++errors;
                    annotations->SetDlgItemText(DG_TEXT_HEIGHT,L"150");annotations->SetDlgItemText(DG_OFFSET,L"0");
                    annotations->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    if(!annotationRequest || annotationRequest->remove || annotationRequest->network_id!="NET-002" ||
                        annotationRequest->settings.text_height_mm!=150 || annotationRequest->settings.offset_mm!=0 ||
                        annotationRequest->settings.text_style!=L"Standard" || annotationRequest->settings.layer!=L"0")++errors;
                    auto* styleSearch=static_cast<SearchableComboBox*>(annotations->GetDlgItem(DG_TEXT_STYLE));
                    styleSearch->SetWindowText(L"HVAC");
                    annotations->SendMessage(WM_COMMAND,MAKEWPARAM(DG_TEXT_STYLE,CBN_EDITCHANGE),reinterpret_cast<LPARAM>(styleSearch->m_hWnd));
                    if(styleSearch->GetCount()!=1)++errors;
                    styleSearch->ShowDropDown(FALSE);styleSearch->SetWindowText(L"Office HVAC");
                    annotations->SendDlgItemMessage(DG_GENERATE,BM_CLICK);Pump();
                    if(!annotationRequest || annotationRequest->settings.text_style!=L"Office HVAC")++errors;
                    annotations->SendDlgItemMessage(DG_REMOVE,BM_CLICK);Pump();
                    if(!annotationRequest || !annotationRequest->remove || annotationRequest->network_id!="NET-002")++errors;
                    annotations->SendMessage(WM_VSCROLL,SB_TOP);Pump();Capture(content,L"drawing-annotations-top.png");
                    annotations->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();Capture(content,L"drawing-annotations-bottom.png");
                    for(int i=0;i<12;++i){content.MoveWindow(0,0,i%2?540:460,i%2?740:340);change(i%2);content.GetDlgItem(6601+i%2)->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();content.GetDlgItem(6601+i%2)->SendMessage(WM_VSCROLL,SB_TOP);Pump();}
                    change(0);geometry->GetDlgItemText(DG_REDUCER,value);if(value!=L"350")++errors;
                    geometry->SendDlgItemMessage(DG_REMOVE,BM_CLICK);Pump();
                    if(!geometryRequest || !geometryRequest->remove || geometryRequest->network_id!="NET-002")++errors;
                    content.DestroyWindow();
                }
                NetworkActions::SetGeometryHandler(nullptr);
                NetworkActions::SetLayerProvider(nullptr);
                NetworkActions::SetTextStyleProvider(nullptr);
                NetworkActions::SetAnnotationHandler(nullptr);
                fwprintf(log,L"Drawing Generation: tabs, network synchronization, defaults, validation, geometry requests, retained edits and resize/scroll cycles. Errors: %d\n",errors);
            }


            {
                NetworkContent network;
                if(!network.CreateView(&frame,108))++errors;
                else
                {
                    network.MoveWindow(0,0,540,940);network.ShowWindow(SW_SHOW);Pump();
                    auto* tabs=static_cast<CTabCtrl*>(network.GetDlgItem(2400));
                    if(tabs->GetItemCount()!=2 || !network.GetDlgItem(2399))++errors;
                    CRect heading,strip,body;network.GetDlgItem(2399)->GetWindowRect(&heading);tabs->GetWindowRect(&strip);
                    network.GetDlgItem(2401)->GetDlgItem(NC_TITLE)->GetWindowRect(&body);
                    if(heading.bottom>strip.top || strip.bottom>body.top)++errors;

                    CWnd* form=network.GetDlgItem(2401);form->SetDlgItemText(NC_NAME,L"Retained UI draft");
                    CRect originalHeader,originalTabs;network.GetDlgItem(2399)->GetWindowRect(&originalHeader);tabs->GetWindowRect(&originalTabs);
                    network.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA*3)),0);Pump();
                    CRect scrolledHeader,scrolledTabs;network.GetDlgItem(2399)->GetWindowRect(&scrolledHeader);tabs->GetWindowRect(&scrolledTabs);
                    if(scrolledHeader.top>=originalHeader.top || originalHeader.top-scrolledHeader.top!=originalTabs.top-scrolledTabs.top)++errors;
                    CRect visibleTabs=scrolledTabs;network.ScreenToClient(&visibleTabs);if(visibleTabs.bottom>0)++errors;
                    Capture(network,L"network-registration-scrolled.png");
                    network.SendMessage(WM_VSCROLL,SB_TOP);Pump();
                    CRect restoredHeader,restoredTabs;network.GetDlgItem(2399)->GetWindowRect(&restoredHeader);tabs->GetWindowRect(&restoredTabs);
                    if(originalHeader!=restoredHeader || originalTabs!=restoredTabs)++errors;
                    for(int cycle=0;cycle<30;++cycle)
                    {
                        network.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);
                        network.SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(WHEEL_DELTA)),0);Pump();
                    }
                    network.SendMessage(WM_VSCROLL,SB_TOP);Pump();

                    Capture(network,L"network-header-registration.png");
                    tabs->SetCurSel(1);NMHDR note={tabs->m_hWnd,2400,TCN_SELCHANGE};
                    network.SendMessage(WM_NOTIFY,2400,reinterpret_cast<LPARAM>(&note));Pump();
                    CWnd* search=network.GetDlgItem(2402)->GetDlgItem(NC_SEARCH);
                    wchar_t cue[128]={};search->SendMessage(EM_GETCUEBANNER,reinterpret_cast<WPARAM>(cue),128);
                    if(cue[0]!=0)++errors;
                    search->SetFocus();Pump();tabs->SetFocus();Pump();Capture(network,L"network-header-registered.png");
                    network.SendMessage(WM_VSCROLL,SB_PAGEDOWN);Pump();Capture(network,L"network-registered-scrolled.png");
                    network.SendMessage(WM_VSCROLL,SB_TOP);Pump();
                    tabs->SetCurSel(0);network.SendMessage(WM_NOTIFY,2400,reinterpret_cast<LPARAM>(&note));Pump();
                    form->GetDlgItemText(NC_NAME,value);if(value!=L"Retained UI draft")++errors;
                    if(!::GetProp(form->GetDlgItem(NC_CONFIG)->m_hWnd,L"FormSectionSurface") ||
                        !::GetProp(form->GetDlgItem(NC_NAME)->m_hWnd,L"FormSectionSurface"))++errors;
                    for(int i=0;i<12;++i)
                    {
                        network.MoveWindow(0,0,i%2?540:460,i%2?740:340);
                        form->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();form->SendMessage(WM_VSCROLL,SB_TOP);Pump();
                    }
                    network.EditDraft(1);Pump();form->GetDlgItemText(NC_NAME,value);if(value!=L"Return Network 01" || tabs->GetCurSel()!=0)++errors;
                    network.DestroyWindow();
                }
                fwprintf(log,L"Network container: header above rounded tabs, tab notifications, retained values, section surfaces and 12 resize/scroll cycles. Errors: %d\n",errors);
            }

            if(!reports.CreateView(&frame,103))++errors;
            else
            {
                reports.MoveWindow(0,0,540,740);reports.ShowWindow(SW_SHOW);Pump();
                if(!ScrollingHeader(reports,4700))++errors;
                auto* tabs=static_cast<CTabCtrl*>(reports.GetDlgItem(4701));
                CWnd* system=reports.GetDlgItem(4801);CWnd* summary=reports.GetDlgItem(4802);CWnd* exportPage=reports.GetDlgItem(4803);
                auto switchTab=[&](int index)
                {
                    tabs->SetCurSel(index);NMHDR notification={tabs->m_hWnd,4701,TCN_SELCHANGE};
                    reports.SendMessage(WM_NOTIFY,4701,reinterpret_cast<LPARAM>(&notification));Pump();
                    for(int i=0;i<3;++i)
                        if(((reports.GetDlgItem(4801+i)->GetStyle()&WS_VISIBLE)!=0)!=(i==index))++errors;
                };
                for(int i=0;i<5;++i)if((system->IsDlgButtonChecked(RC_SYSTEM_FIRST+i)==BST_CHECKED)!=(i!=3))++errors;
                system->GetDlgItemText(RC_SYSTEM,value);if(value!=L"Supply Network 01")++errors;
                Capture(reports,L"reports-system-preview.png");
                system->SendDlgItemMessage(RC_SYSTEM_FIRST+3,BM_CLICK);Pump();if(ReportPreview::SystemCount()!=5)++errors;
                switchTab(1);
                for(int i=0;i<4;++i)if(summary->IsDlgButtonChecked(RC_SUMMARY_FIRST+i)!=BST_CHECKED)++errors;
                Capture(reports,L"reports-summary-preview.png");
                summary->SendDlgItemMessage(RC_SUMMARY_FIRST,BM_CLICK);Pump();if(ReportPreview::SummaryCount()!=3)++errors;
                switchTab(0);if(system->IsDlgButtonChecked(RC_SYSTEM_FIRST+3)!=BST_CHECKED)++errors;
                system->SendDlgItemMessage(RC_SYSTEM_FIRST+3,BM_CLICK);Pump();
                auto* systemCombo=static_cast<CComboBox*>(system->GetDlgItem(RC_SYSTEM));systemCombo->SetCurSel(1);
                system->SendMessage(WM_COMMAND,MAKEWPARAM(RC_SYSTEM,CBN_SELCHANGE),reinterpret_cast<LPARAM>(systemCombo->m_hWnd));
                switchTab(2);exportPage->GetDlgItemText(RC_EXPORT_SYSTEM,value);if(value!=L"Return Network 01")++errors;
                exportPage->GetDlgItemText(RC_EXPORT_STATUS,value);if(value.Find(L"4 system report types and 3 project summary")<0)++errors;
                auto* format=static_cast<CComboBox*>(exportPage->GetDlgItem(RC_FORMAT));format->SetCurSel(1);
                exportPage->SendMessage(WM_COMMAND,MAKEWPARAM(RC_FORMAT,CBN_SELCHANGE),reinterpret_cast<LPARAM>(format->m_hWnd));
                exportPage->SendDlgItemMessage(RC_EXPORT_PREVIEW,BM_CLICK);Pump();exportPage->GetDlgItemText(RC_EXPORT_STATUS,value);
                if(value.Find(L"PDF (.pdf)")<0 || value.Find(L"No files have been written")<0)++errors;
                switchTab(1);summary->SendDlgItemMessage(RC_SUMMARY_FIRST,BM_CLICK);Pump();
                switchTab(0);systemCombo->SetCurSel(0);
                system->SendMessage(WM_COMMAND,MAKEWPARAM(RC_SYSTEM,CBN_SELCHANGE),reinterpret_cast<LPARAM>(systemCombo->m_hWnd));
                for(int i=0;i<5;++i)if(system->IsDlgButtonChecked(RC_SYSTEM_FIRST+i)==BST_CHECKED)system->SendDlgItemMessage(RC_SYSTEM_FIRST+i,BM_CLICK);
                system->SendDlgItemMessage(RC_SYSTEM_PREVIEW,BM_CLICK);Pump();system->GetDlgItemText(RC_SYSTEM_STATUS,value);
                if(value.Find(L"Select at least one")<0)++errors;
                for(int i=0;i<5;++i)if(i!=3)system->SendDlgItemMessage(RC_SYSTEM_FIRST+i,BM_CLICK);
                CRect headerBefore;reports.GetDlgItem(4700)->GetWindowRect(&headerBefore);
                system->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();CRect headerAfter;reports.GetDlgItem(4700)->GetWindowRect(&headerAfter);
                if(headerBefore==headerAfter)++errors;
                system->SendMessage(WM_VSCROLL,SB_TOP);
                for(int i=0;i<12;++i)
                {
                    reports.MoveWindow(0,0,i%2?540:460,i%2?740:340);switchTab(i%3);
                    reports.GetDlgItem(4801+i%3)->SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();
                    reports.GetDlgItem(4801+i%3)->SendMessage(WM_VSCROLL,SB_TOP);Pump();
                }
                reports.MoveWindow(0,0,540,740);switchTab(2);format->SetCurSel(0);
                exportPage->SendMessage(WM_COMMAND,MAKEWPARAM(RC_FORMAT,CBN_SELCHANGE),reinterpret_cast<LPARAM>(format->m_hWnd));Pump();
                Capture(reports,L"reports-export-preview.png");
                reports.DestroyWindow();
            }
            fwprintf(log,L"Reports: default sheets, native tab notifications, selection retention, shared export settings, validation, 12 resize/scroll cycles, centered Save and session snapshot. Errors: %d\n",errors);
            if(!projectInfo.CreatePage(&frame,104))++errors;
            else
            {
                projectInfo.MoveWindow(0,0,540,740);projectInfo.ShowWindow(SW_SHOW);Pump();
                const auto checkEditMode=[&](ProjectInformationPage& page,bool editing)
                {
                    CString caption;page.GetDlgItemText(PI_SAVE,caption);
                    if(caption!=(editing?L"Save":L"Modify") || ProjectInfo::Editing()!=editing)++errors;
                    for(size_t i=0;i<ProjectInfo::Count;++i)
                    {
                        const bool readOnly=(page.GetDlgItem(PI_FIELD_FIRST+static_cast<UINT>(i))->GetStyle()&ES_READONLY)!=0;
                        if(readOnly==editing)++errors;
                        CWnd* field=page.GetDlgItem(PI_FIELD_FIRST+static_cast<UINT>(i));
                        CClientDC dc(field);
                        page.SendMessage(readOnly?WM_CTLCOLORSTATIC:WM_CTLCOLOREDIT,
                            reinterpret_cast<WPARAM>(dc.m_hDC),reinterpret_cast<LPARAM>(field->m_hWnd));
                        if(dc.GetTextColor()!=(editing?UiStyle::Colors(page.m_hWnd).bright:UiStyle::Colors(page.m_hWnd).muted))++errors;
                        if(i!=ProjectInfo::AdditionalInformation)
                        {
                            auto oldFont=dc.SelectObject(field->GetFont());TEXTMETRIC metrics{};dc.GetTextMetrics(&metrics);dc.SelectObject(oldFont);
                            CRect client,window;field->GetClientRect(&client);field->GetWindowRect(&window);
                            CPoint origin(0,0);field->ClientToScreen(&origin);
                            CRect format;field->SendMessage(EM_GETRECT,0,reinterpret_cast<LPARAM>(&format));
                            if(client.Height()!=metrics.tmHeight || abs((origin.y-window.top)*2+client.Height()-window.Height())>1)++errors;
                            if(field->GetWindowTextLength()>0)
                            {
                                const LRESULT position=field->SendMessage(EM_POSFROMCHAR,0);
                                if(static_cast<short>(HIWORD(position))!=format.top)++errors;
                            }
                        }
                    }
                };
                const auto registrationMatchesDraft=[&]()
                {
                    const auto& information=ProjectInfo::Application().project_information();
                    if(!information)return false;
                    const std::array<std::wstring,ProjectInfo::Count> values={
                        information->project_name,information->project_number,information->building_name,
                        information->project_address,information->client_name,information->organization_name,
                        information->author,information->level_name,information->drawing_number,
                        information->revision,information->additional_information};
                    for(size_t i=0;i<values.size();++i)
                        if(values[i]!=ProjectInfo::Draft()[i].GetString())return false;
                    return true;
                };
                checkEditMode(projectInfo,true);
                if(ProjectInfo::Application().project_information())++errors;
                for(size_t i=0;i<ProjectInfo::Count;++i)
                {
                    projectInfo.GetDlgItemText(PI_FIELD_FIRST+static_cast<UINT>(i),value);if(!value.IsEmpty())++errors;
                    CString text;text.Format(L"Example field %zu",i+1);
                    projectInfo.SendDlgItemMessage(PI_FIELD_FIRST+static_cast<UINT>(i),EM_SETSEL,0,-1);
                    projectInfo.SendDlgItemMessage(PI_FIELD_FIRST+static_cast<UINT>(i),EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(text.GetString()));Pump();
                    if(ProjectInfo::Draft()[i]!=text)++errors;
                }
                const UINT notesId=PI_FIELD_FIRST+static_cast<UINT>(ProjectInfo::AdditionalInformation);
                projectInfo.SendDlgItemMessage(notesId,EM_SETSEL,0,-1);
                projectInfo.SendDlgItemMessage(notesId,EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L"Design notes\r\nReview pending."));Pump();
                if(ProjectInfo::Draft()[ProjectInfo::AdditionalInformation]!=L"Design notes\r\nReview pending.")++errors;
                if(static_cast<CEdit*>(projectInfo.GetDlgItem(notesId))->GetLineCount()!=2)++errors;
                if(!(projectInfo.GetDlgItem(notesId)->GetStyle()&ES_WANTRETURN))++errors;
                projectInfo.GetDlgItem(PI_FIELD_FIRST+static_cast<UINT>(ProjectInfo::Revision))->SetFocus();
                MSG tab={};tab.message=WM_KEYDOWN;tab.wParam=VK_TAB;
                if(!projectInfo.PreTranslateMessage(&tab) || ::GetFocus()!=projectInfo.GetDlgItem(notesId)->m_hWnd)++errors;
                CRect notes,view;projectInfo.GetDlgItem(notesId)->GetWindowRect(&notes);projectInfo.ScreenToClient(&notes);projectInfo.GetClientRect(&view);
                if(notes.top<0 || notes.bottom>view.bottom)++errors;
                for(int i=0;i<12;++i)
                {
                    projectInfo.MoveWindow(0,0,i%2?540:460,i%2?740:340);
                    projectInfo.SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();projectInfo.SendMessage(WM_VSCROLL,SB_TOP);Pump();
                }
                projectInfo.GetDlgItemText(PI_NOTICE,value);if(value.Find(L"Unsaved changes")<0)++errors;
                projectInfo.SendDlgItemMessage(PI_SAVE,BM_CLICK);Pump();
                if(ProjectInfo::Saved()!=ProjectInfo::Draft())++errors;
                checkEditMode(projectInfo,false);
                Capture(projectInfo,L"project-information-saved-text.png");
                if(!registrationMatchesDraft())++errors;
                projectInfo.SendDlgItemMessage(PI_FIELD_FIRST,WM_CHAR,L'X');Pump();
                projectInfo.GetDlgItemText(PI_FIELD_FIRST,value);if(value!=L"Example field 1")++errors;
                projectInfo.GetDlgItemText(PI_NOTICE,value);if(value.Find(L"saved for this session")<0)++errors;
                const auto registeredBeforeModify=ProjectInfo::Application().project_information();
                projectInfo.SendDlgItemMessage(PI_SAVE,BM_CLICK);Pump();
                checkEditMode(projectInfo,true);
                projectInfo.SetDlgItemText(PI_FIELD_FIRST,L"Modified project");Pump();
                Capture(projectInfo,L"project-information-editing-text.png");
                if(ProjectInfo::Application().project_information()!=registeredBeforeModify)++errors;
                projectInfo.SendDlgItemMessage(PI_SAVE,BM_CLICK);Pump();
                checkEditMode(projectInfo,false);
                if(!registrationMatchesDraft())++errors;
                projectInfo.SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();
                CRect save,client;projectInfo.GetDlgItem(PI_SAVE)->GetWindowRect(&save);projectInfo.ScreenToClient(&save);projectInfo.GetClientRect(&client);
                CRect railBox;projectInfo.GetDlgItem(2900)->GetWindowRect(&railBox);
                if(abs((save.left+save.right)-(client.Width()-railBox.Width()))>2 || save.top<0 || save.bottom>client.bottom)++errors;
                projectInfo.ShowWindow(SW_HIDE);projectInfo.ShowWindow(SW_SHOW);Pump();
                if(ProjectInfo::Draft()[ProjectInfo::ProjectName]!=L"Modified project")++errors;
                checkEditMode(projectInfo,false);
                projectInfo.DestroyWindow();
                {
                    ProjectInformationPage restored;
                    if(!restored.CreatePage(&frame,105))++errors;
                    else
                    {
                        restored.MoveWindow(0,0,540,740);restored.ShowWindow(SW_SHOW);Pump();
                        restored.GetDlgItemText(PI_FIELD_FIRST,value);if(value!=L"Modified project")++errors;
                        restored.GetDlgItemText(notesId,value);if(value!=L"Design notes\r\nReview pending.")++errors;
                        checkEditMode(restored,false);
                        restored.SendDlgItemMessage(PI_SAVE,BM_CLICK);Pump();
                        checkEditMode(restored,true);
                        for(size_t i=0;i<ProjectInfo::Count;++i){restored.SendDlgItemMessage(PI_FIELD_FIRST+static_cast<UINT>(i),EM_SETSEL,0,-1);restored.SendDlgItemMessage(PI_FIELD_FIRST+static_cast<UINT>(i),EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L""));}
                        restored.SendDlgItemMessage(PI_SAVE,BM_CLICK);Pump();
                        if(ProjectInfo::Saved()!=ProjectInfo::Draft())++errors;
                        checkEditMode(restored,false);
                        if(!registrationMatchesDraft())++errors;
                        if(ProjectInfo::Application().project_information()!=
                           std::optional<dsd::application::ProjectInformation>{dsd::application::ProjectInformation{}})++errors;
                        restored.SendMessage(WM_VSCROLL,SB_TOP);Pump();Capture(restored,L"project-information-top.png");
                        restored.SendMessage(WM_VSCROLL,SB_BOTTOM);Pump();Capture(restored,L"project-information-bottom.png");
                        restored.DestroyWindow();
                    }
                }
            }
            ProjectInfo::Reset();
            if(!ProjectInfo::Editing() || ProjectInfo::Application().project_information())++errors;
            fwprintf(log,L"Project information: optional fields, application registration, Save/Modify read-only and text-color toggle, centered native text, multiline notes, keyboard focus scrolling, hide/show and recreation retention, session reset. Errors: %d\n",errors);
            fflush(log);registration.DestroyWindow();registered.DestroyWindow();frame.DestroyWindow();
        }
        Fonts::Unload();
        bool missing=Fonts::LoadDirectory(L".duct-ui-test-missing-fonts");
        if(missing || wcscmp(Fonts::Face(FW_NORMAL),L"Segoe UI")!=0 ||
            wcscmp(Fonts::Face(FW_MEDIUM),L"Segoe UI")!=0 ||
            wcscmp(Fonts::Face(FW_SEMIBOLD),L"Segoe UI")!=0)++errors;
        fwprintf(log,L"Private font load/unload and missing-font fallback: final errors %d\n",errors);
        {
            int firstDrawing=0,secondDrawing=0;
            DuctUi::SetDrawingKeyProvider(DrawingKeyFixture);simulatedDrawing=&firstDrawing;
            ProjectInfo::Draft()[ProjectInfo::ProjectName]=L"Drawing A";
            ProjectInfo::RegisterProjectInformation();NetworkSession::NextId()=17;
            NetworkDraft item;item.id=16;item.name=L"A network";NetworkSession::Drafts().push_back(item);
            simulatedDrawing=&secondDrawing;
            if(ProjectInfo::Application().project_information() || !NetworkSession::Drafts().empty() || NetworkSession::NextId()!=1)++errors;
            ProjectInfo::Draft()[ProjectInfo::ProjectName]=L"Drawing B";ProjectInfo::RegisterProjectInformation();
            NetworkSession::NextId()=33;
            simulatedDrawing=&firstDrawing;
            if(ProjectInfo::Saved()[ProjectInfo::ProjectName]!=L"Drawing A" || NetworkSession::NextId()!=17 || NetworkSession::Drafts().size()!=1)++errors;
            simulatedDrawing=&secondDrawing;
            DuctUi::ReleaseDrawingSession(&firstDrawing);
            if(ProjectInfo::Saved()[ProjectInfo::ProjectName]!=L"Drawing B" || NetworkSession::NextId()!=33)++errors;
            DuctUi::ReleaseDrawingSession(&secondDrawing);DuctUi::SetDrawingKeyProvider(nullptr);simulatedDrawing=nullptr;
            fwprintf(log,L"Drawing sessions: isolated project data, registered UI records and network counters. Errors: %d\n",errors);
        }
        Fonts::Unload();fclose(log);UiStyle::StopGraphics();exitCode_=errors?1:0;return FALSE;
    }
} app;

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show)
{ if (!AfxWinInit(instance,previous,command,show)) return 2; app.InitInstance(); return app.ExitInstance(); }
