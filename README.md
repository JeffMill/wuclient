# wuclient
Simple WU client app using ATL.

## Building

winget install 'Microsoft.VisualStudio.2022.Community'

Tools > Get Tools and Features...

* Desktop Development with C++
* C++ ATL for latest v143 build tools (x86 & x64)

Open project

## Script

in elevated prompt:

c:\debuggers_x86\windbg.exe -y 'cache*;SRV*https://symweb'  Debug\wuclient.exe

('g' until debugbreak in SearchSync)

dt session p
  shows session interface pointer.

0:000:x86> dt session p
Local var @ 0x6ff358 Type ATL::CComPtr<IUpdateSession>
 { 00a8f06c }  { 00a8f06c }    +0x000 p : 0x00a8f06c IUpdateSession
 
0:000:x86> dds poi(00a8f06c) l5
6177111c  617b4760 wuapi![thunk]:ATL::CComObject<CUpdateSession>::QueryInterface`adjustor{8}'
61771120  617b4670 wuapi![thunk]:ATL::CComObject<CUpdateSession>::AddRef`adjustor{8}'
61771124  617b4870 wuapi![thunk]:ATL::CComObject<CUpdateSession>::Release`adjustor{8}'
61771128  6179d480 wuapi!ATL::IDispatchImpl<IWindowsUpdateAgentInfo,&IID_IWindowsUpdateAgentInfo,&LIBID_WUApiLib,2,0,ATL::CComTypeInfoHolder>::GetTypeInfoCount [onecore\enduser\windowsupdate\client\commoncomapi\ComHelper.h @ 296]
6177112c  617bebe0 wuapi!ATL::IDispatchImpl<IUpdateSession3,&IID_IUpdateSession3,&LIBID_WUApiLib,2,0,ATL::CComTypeInfoHolder>::GetTypeInfo [onecore\external\sdk\inc\atlmfc\atlcom.h @ 4666]

discuss binary layout.

===============================================================================

0:014> !comexts.comobj searcher

Object Details
---------------------------------------------------------
CoDecodeProxy requires a valid developer license to be installed on the debugging host.
ERROR: !comobj: extension exception 0x80070005.
    "Access denied. Make sure you have a valid developer license installed."

Show-WindowsDeveloperLicenseRegistration
  then enable "Developer Mode"


0:000> !comexts.comobj searcher

Object Details
---------------------------------------------------------
InProc, Raw Object
Class [wuapi!ATL::CComObject<CUpdateSearcher>], Base address [0x0151188c]

===============================================================================

bu wuapi!CSusInternalWrapper::BeginFindUpdates
g

0:008> dt pSusInternal p
Local var @ 0x2c6fc00 Type ATL::CComPtr<ISusInternal>
 { 00c0c71c }  { 00c0c71c }    +0x000 p : 0x00c0c71c ISusInternal

0:008> dds poi(00c0c71c) l3
60af8008  60af7570 wups!IUnknown_QueryInterface_Proxy
60af800c  60af74a0 wups!IUnknown_AddRef_Proxy
60af8010  60af7500 wups!IUnknown_Release_Proxy

0:008> !comexts.comobj /a 00c0c71c

Object Details
---------------------------------------------------------
Out-of-Proc, Proxy

Hosting process: ID[976], Name[C:\Windows\System32\svchost.ex]
Apartment:MTA
COM object base address: 0x8264f030
Attach to server and dump details...

Click Attach:
.attach 0n976; g; !comobj /a 0x8264F030; .detach

===============================================================================


Start-Service -Name wuauserv

c:\debuggers_x64\windbg.exe -y 'cache*;SRV*https://symweb' -psn wuauserv 

g wuclient!CAtlExeModule::Search

===============================================================================

0:014> !load comexts

0:014> !comexts.apartment
(or !comexts.apartment /t TID)

===============================================================================

bp wuaueng!CClientCallRecorder::BeginFindUpdates

0:011> !comexts.ccalls /findClient
Client system process ID: 0x0000000000000d5c
Client system thread ID: 0x0000000000003868

Thread ID won't match output, as WUAPI creates multiple threads to do federated calls.

(There's also /findServer).

  
===============================================================================

  !comtls          - Displays the COM TLS structure
     +0x068 pCurrentContext  : 0x0000029f`0301a168 CObjectContext
    
    CObjectContext._pApartment points to curent CComApartment (gpMTAApartment)

  !comglobals      - Lists COM global variables and their values
   combase!gpMTAApartment = 0x0000029f0300e3f0 ( dt combase!CComApartment 0x0000029f0300e3f0 )
   combase!gpNTAApartment = 0x0000029f0300dee0 ( dt combase!CComApartment 0x0000029f0300dee0 )
   combase!gpMtaObjServer = 0x0000029f03918820 ( dt combase!CObjServer 0x0000029f03918820 )
   combase!gpNtaObjServer = 0x0000000000000000 ( dt combase!CObjServer 0x0000000000000000 )
   combase!CClassCache::_MTALSvrsFront = 0x0000029f03905c20 ( dt combase!CClassCache::CLSvrClassEntry 0x0000029f03905c20 )
   combase!CClassCache::_NTALSvrsFront = 0x0000000000000000 ( dt combase!CClassCache::CLSvrClassEntry 0x0000000000000000 )
   combase!gOXIDTbl = 0x0000000000000000 ( dt combase!COXIDTable 0x0000000000000000 )
   combase!gRIFTbl = 0x0000000000000000 ( dt combase!CRIFTable 0x0000000000000000 )
   combase!gSRFTbl = 0x00007fff3a5d4f20 ( dt combase!CNameHashTable 0x00007fff3a5d4f20 )
   combase!gGIPTbl = 0x00007fff3a5d4f58 ( dt combase!CGIPTable 0x00007fff3a5d4f58 )
