#include "autocad/ui/model/NetworkSession.h"

namespace
{
    std::vector<NetworkDraft> gNetworkDrafts;
    int gNextNetworkId=4;
    bool gDemoNetworksInitialized=false;
}

void NetworkSession::InitializeSamples()
{
    if (gDemoNetworksInitialized) return;
    gDemoNetworksInitialized = true;
    const wchar_t* names[] = { L"Supply Network 01", L"Return Network 01", L"Exhaust Network 01" };
    const wchar_t* systems[] = { L"Supply Air", L"Return Air", L"Exhaust Air" };
    const wchar_t* counts[] = { L"24", L"18", L"12" };
    const wchar_t* flows[] = { L"2500 CFM", L"1800 CFM", L"900 CFM" };
    for (int i = 0; i < 3; ++i)
    {
        NetworkDraft draft;
        draft.id = i + 1; draft.name = names[i]; draft.system = systems[i];
        draft.segmentCount = counts[i]; draft.totalFlow = flows[i];
        draft.status = i == 1 ? L"Drawn" : L"Analyzed";
        gNetworkDrafts.push_back(draft);
    }
}

std::vector<NetworkDraft>& NetworkSession::Drafts(){return gNetworkDrafts;}
int& NetworkSession::NextId(){return gNextNetworkId;}
void NetworkSession::Reset(){gNetworkDrafts.clear();gNextNetworkId=4;gDemoNetworksInitialized=false;}
