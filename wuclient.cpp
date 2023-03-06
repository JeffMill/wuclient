#include <iostream>
#include <codecvt>

#include <atlbase.h> // ATL
#include <atlcom.h>
#include <atlstr.h> // CAtlString
#include <wuapi.h> // WU API

#define CHR(cmd)                                                                                 \
    {                                                                                            \
        HRESULT hrTmp = (cmd);                                                                     \
        if (FAILED(hrTmp))                                                                       \
        {                                                                                        \
            std::cout << "Fail " << std::hex << hrTmp << std::dec << " : " << #cmd << std::endl; \
            return hrTmp;                                                                        \
        }                                                                                        \
    }

std::ostream& operator << (std::ostream& os, const OperationResultCode& orc)
{
    switch (orc)
    {
    case orcNotStarted: os << "orcNotStarted"; break;
    case orcInProgress: os << "orcInProgress"; break;
    case orcSucceeded: os << "orcSucceeded"; break;
    case orcSucceededWithErrors: os << "orcSucceededWithErrors"; break;
    case orcFailed: os << "orcFailed"; break;
    case orcAborted: os << "orcAborted"; break;
    default: os << "?"; break;
    }
    os << " (" << static_cast<unsigned int>(orc) << ")";
    return os;
}

void CoutPIDandTID(const char* prefix)
{
    const DWORD PID{ GetCurrentProcessId() };
    const DWORD TID{ GetCurrentThreadId() };

    std::cout << "[" << prefix << "] "
        << "PID: " << PID << " (0x" << std::hex << PID << std::dec << ") "
        << "TID: " << TID << " (0x" << std::hex << TID << std::dec << ")"
        << std::endl;
}
class CSearchCompletedCallback :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public ISearchCompletedCallback
{
public:
    BEGIN_COM_MAP(CSearchCompletedCallback)
        COM_INTERFACE_ENTRY(ISearchCompletedCallback)
    END_COM_MAP()

    DECLARE_NO_REGISTRY()

        CSearchCompletedCallback()
    {
        eventCompleted = CreateEvent(nullptr /*lpEventAttributes*/, TRUE/*bManualReset*/, FALSE/*bInitialState*/, nullptr/*lpName*/);
    }

    ~CSearchCompletedCallback()
    {
        CloseHandle(eventCompleted);
    }

    STDMETHOD(Invoke)(ISearchJob* job, ISearchCompletedCallbackArgs* callbackArgs)
    {
        UNREFERENCED_PARAMETER(job);
        UNREFERENCED_PARAMETER(callbackArgs);

        CoutPIDandTID("ISearchCompletedCallback::Invoke");

        SetEvent(eventCompleted);
        return S_OK;
    }

    void WaitForCompletion()
    {
        WaitForSingleObject(eventCompleted, INFINITE);
    }

private:
    HANDLE eventCompleted = nullptr;
};

class CAtlExeModule : public CAtlExeModuleT<CAtlExeModule>
{
public:
    HRESULT PreMessageLoop(_In_ int /*nShowCmd*/) throw()
    {
        return S_OK;
    }

    void RunMessageLoop() throw()
    {
        CoutPIDandTID("RunMessageLoop");

        SearchSync();

        SearchAsync();
    }

    HRESULT PostMessageLoop() throw()
    {
        return S_OK;
    }

private:
    HRESULT SearchSync()
    {
        if (IsDebuggerPresent()) { DebugBreak(); }

        std::cout << "Creating IUpdateSession" << std::endl;
        CComPtr<IUpdateSession> session;
        CHR(session.CoCreateInstance(__uuidof(UpdateSession), nullptr, CLSCTX_INPROC_SERVER));

        std::cout << "Creating IUpdateSearcher" << std::endl;
        CComPtr<IUpdateSearcher> searcher;
        CHR(session->CreateUpdateSearcher(&searcher));

        std::cout << "Calling IUpdateSearcher::Search" << std::endl;
        CComBSTR searchCriteria(L"IsInstalled=1");
        CComPtr<ISearchResult> searchResult;
        CHR(searcher->Search(searchCriteria, &searchResult));

        std::cout << "Calling ISearchResult::get_ResultCode" << std::endl;
        OperationResultCode searchResultCode;
        CHR(searchResult->get_ResultCode(&searchResultCode));
        std::cout << "ResultCode = " << searchResultCode << std::endl;

        return S_OK;
    }

    HRESULT SearchAsync()
    {
        if (IsDebuggerPresent()) { DebugBreak(); }

        std::cout << "Creating IUpdateSession" << std::endl;
        CComPtr<IUpdateSession> session;
        CHR(session.CoCreateInstance(__uuidof(UpdateSession), nullptr, CLSCTX_INPROC_SERVER));

        std::cout << "Creating IUpdateSearcher" << std::endl;
        CComPtr<IUpdateSearcher> searcher;
        CHR(session->CreateUpdateSearcher(&searcher));

        CComPtr<CSearchCompletedCallback> searchCompletedCallback(new CComObjectNoLock<CSearchCompletedCallback>());

        std::cout << "Calling IUpdateSearcher::BeginSearch" << std::endl;
        CComBSTR searchCriteria(L"IsInstalled=1");
        CComVariant state;
        CComPtr<ISearchJob> searchJob;
        CHR(searcher->BeginSearch(searchCriteria, searchCompletedCallback, state, &searchJob));

        std::cout << "Waiting for completion" << std::endl;
        searchCompletedCallback->WaitForCompletion();

        std::cout << "Calling IUpdateSearcher::EndSearch" << std::endl;
        CComPtr<ISearchResult> searchResult;
        // This will fail with 80240032 (WU_E_INVALID_CRITERIA) if searchCriteria is invalid.
        CHR(searcher->EndSearch(searchJob, &searchResult));

        std::cout << "Calling ISearchResult::get_ResultCode" << std::endl;
        OperationResultCode searchResultCode;
        CHR(searchResult->get_ResultCode(&searchResultCode));
        std::cout << "ResultCode = " << searchResultCode << std::endl;

        if (searchResultCode != orcSucceeded)
        {
            return E_FAIL;
        }

        CComPtr<IUpdateCollection> updateCollection;
        CHR(searchResult->get_Updates(&updateCollection));
        LONG cUpdates;
        CHR(updateCollection->get_Count(&cUpdates));
        std::cout << "Update count = " << cUpdates << std::endl;

        for (LONG iUpdate = 0; iUpdate < cUpdates; iUpdate++)
        {
            CComPtr<IUpdate> update;
            CHR(updateCollection->get_Item(iUpdate, &update));

            CComBSTR title;
            CHR(update->get_Title(&title));

            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf16_to_utf8;

            std::cout << iUpdate + 1 << ": " << utf16_to_utf8.to_bytes(title) << std::endl;
        }

        return S_OK;
    }
} _Module; // Must be named "_Module"

int main()
{
    return _Module.WinMain(0);
}
