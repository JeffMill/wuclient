#include <iostream>
#include <codecvt>

#include <atlbase.h> // ATL
#include <atlcom.h>
#include <atlstr.h> // CAtlString
#include <wuapi.h> // WU API

#define CHR(cmd)                                                                                 \
    {                                                                                            \
        HRESULT hrTmp = cmd;                                                                     \
        if (FAILED(hrTmp))                                                                       \
        {                                                                                        \
            std::cout << "Fail " << std::hex << hrTmp << std::dec << " : " << #cmd << std::endl; \
            return hrTmp;                                                                        \
        }                                                                                        \
    }

CComModule _Module;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_Module;

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

    STDMETHOD(Invoke)(ISearchJob* job,  ISearchCompletedCallbackArgs* callbackArgs)
    {
        UNREFERENCED_PARAMETER(job);
        UNREFERENCED_PARAMETER(callbackArgs);

        std::cout << "ISearchCompletedCallback::Invoke called" << std::endl;
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


HRESULT Run()
{
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

        using utf16_to_utf8 = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;
        utf16_to_utf8 utf8_converter;

        std::cout << iUpdate + 1 << ": " << utf8_converter.to_bytes(title) << std::endl;
    }

    return S_OK;
}

int main()
{
    HRESULT hr;

    (void)CoInitializeEx(NULL, COINIT_MULTITHREADED);

    std::cout << "wu client app started" << std::endl;

    Run();

    std::cout << "wu client app ended" << std::endl;

    CoUninitialize();
}
