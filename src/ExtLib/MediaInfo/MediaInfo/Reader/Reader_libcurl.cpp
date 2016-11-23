/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// For user: you can disable or enable it
//#define MEDIAINFO_DEBUG
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_LIBCURL_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Reader/Reader_libcurl.h"
#include "MediaInfo/File__Analyze.h"
#if defined MEDIAINFO_LIBCURL_DLL_RUNTIME
    //Copy of cURL include files
    #include "MediaInfo/Reader/Reader_libcurl_Include.h"
#else
    #define CURL_STATICLIB
    #undef __TEXT
    #include "curl/curl.h"
#endif
#include <ctime>
#include <algorithm> //For Url
#define MEDIAINFO_HMAC 1
#if MEDIAINFO_HMAC
    #include "hmac.h"
    #include "base64.h"
#endif //MEDIAINFO_HMAC
#include "MediaInfo/HashWrapper.h"
#include "ZenLib/File.h"
#include "tinyxml2.h"
using namespace tinyxml2;
using namespace ZenLib;
using namespace std;
#ifdef MEDIAINFO_DEBUG
    #include <iostream>
#endif // MEDIAINFO_DEBUG
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Helpers
//***************************************************************************

namespace Http
{
    //Helpers
   static  void CutHead(std::string &Input, std::string &Output, const std::string& Delimiter)
    {
        // Remove the delimiter and everything that precedes
        size_t Delimiter_Pos = Input.find(Delimiter);
        if (Delimiter_Pos != std::string::npos)
        {
            size_t Begin    = Delimiter_Pos + Delimiter.size();
            Output          = Input.substr(0, Delimiter_Pos);
            Input           = Input.substr(Begin, Input.size() - Begin);
        }
    }
    static void CutTail(std::string &Input, std::string &Output, const std::string &Delimiter, bool KeepDelimiter=false)
    {
        // Remove the delimiter and everything that follows
        size_t Delimiter_Pos = Input.find(Delimiter);
        if (Delimiter_Pos != std::string::npos)
        {
            size_t Begin    = Delimiter_Pos;
            if (!KeepDelimiter)
                Begin+=Delimiter.size();
            size_t End      = Input.size() - Begin;
            Output          = Input.substr(Begin, End);
            Input           = Input.substr(0, Delimiter_Pos);
        }
    }

    class Url
    {
    public:
        Url(const std::string &In)
            : Host(In)
        {
            CutHead (Host,  Protocol,   "://"   );
            CutTail (Host,  Search,     "?"     );
            CutHead (Host,  User,       "@"     );
            CutTail (Host,  Path,       "/",    true);
            CutTail (User,  Password,   ":"     );
            CutTail (Host,  Port,       ":"     );

            std::transform(Protocol.begin(), Protocol.end(), Protocol.begin(), ::tolower);
        }

        string ToString()
        {
            string ToReturn;
            if (!Protocol.empty())
            {
                ToReturn += Protocol;
                ToReturn += "://";
            }
            if (!User.empty() || !Password.empty())
            {
                ToReturn += User;
                ToReturn += ':';
                ToReturn += Password;
                ToReturn += '@';
            }
            ToReturn += Host;
            if (!Port.empty())
            {
                ToReturn += ':';
                ToReturn += Port;
            }
            if (!Path.empty() || !Search.empty())
            {
                ToReturn += Path;
                if (!Search.empty())
                {
                    ToReturn += '?';
                    ToReturn += Search;
                }
            }
            return ToReturn;
        }

        // Members
        std::string Protocol;
        std::string User;
        std::string Password;
        std::string Host;
        std::string Port;
        std::string Path;
        std::string Search;
    };
}

Ztring Reader_libcurl_FileNameWithoutPassword(const Ztring &FileName)
{
    Ztring FileName_Modified(FileName);
    size_t Begin=FileName_Modified.find(__T(':'), 6);
    size_t End=FileName_Modified.find(__T('@'));
    if (Begin!=string::npos && End!=string::npos && Begin<End)
        FileName_Modified.erase(Begin, End-Begin);
    return FileName_Modified;
}

//***************************************************************************
// libcurl stuff
//***************************************************************************

//---------------------------------------------------------------------------
struct Reader_libcurl::curl_data
{
    MediaInfo_Internal* MI;
    CURL*               Curl;
    char                ErrorBuffer[CURL_ERROR_SIZE];
    #if MEDIAINFO_NEXTPACKET
        CURLM*          CurlM;
    #endif //MEDIAINFO_NEXTPACKET
    struct curl_slist*  HttpHeader;
    std::bitset<32>     Status;
    String              File_Name;
    std::string         Ssl_CertificateFileName;
    std::string         Ssl_CertificateFormat;
    std::string         Ssl_PrivateKeyFileName;
    std::string         Ssl_PrivateKeyFormat;
    std::string         Ssl_CertificateAuthorityFileName;
    std::string         Ssl_CertificateAuthorityPath;
    std::string         Ssl_CertificateRevocationListFileName;
    bool                Ssl_IgnoreSecurity;
    std::string         Ssh_KnownHostsFileName;
    std::string         Ssh_PublicKeyFileName;
    std::string         Ssh_PrivateKeyFileName;
    bool                Ssh_IgnoreSecurity;
    bool                Init_AlreadyDone;
    bool                Init_NotAFile;
    #if MEDIAINFO_NEXTPACKET
        bool            NextPacket;
    #endif //MEDIAINFO_NEXTPACKET
    time_t              Time_Max;
    #ifdef MEDIAINFO_DEBUG
        int64u          Debug_BytesRead_Total;
        int64u          Debug_BytesRead;
        int64u          Debug_Count;
    #endif // MEDIAINFO_DEBUG
    #if MEDIAINFO_EVENTS
        int64u          Stream_Offset;
    #endif //MEDIAINFO_EVENTS

    curl_data()
    {
        MI=NULL;
        Curl=NULL;
        ErrorBuffer[0]='\0';
        #if MEDIAINFO_NEXTPACKET
            CurlM=NULL;
        #endif //MEDIAINFO_NEXTPACKET
        HttpHeader=NULL;
        Ssl_IgnoreSecurity=false;
        Ssh_IgnoreSecurity=false;
        Init_AlreadyDone=false;
        Init_NotAFile=false;
        #if MEDIAINFO_NEXTPACKET
            NextPacket=false;
        #endif //MEDIAINFO_NEXTPACKET
        Time_Max=0;
        #ifdef MEDIAINFO_DEBUG
            Debug_BytesRead_Total=0;
            Debug_BytesRead=0;
            Debug_Count=1;
        #endif // MEDIAINFO_DEBUG
        #if MEDIAINFO_EVENTS
            Stream_Offset=(int64u)-1;
        #endif //MEDIAINFO_EVENTS
    }
};

//---------------------------------------------------------------------------
struct Reader_libcurl_curl_data_getregion
{
    CURL*               Curl;
    Ztring              File_Name;
    std::string         Amazon_AWS_Region;
};

//---------------------------------------------------------------------------
size_t libcurl_WriteData_CallBack_Amazon_AWS_Region(void *ptr, size_t size, size_t nmemb, void *data)
{
    //Must be a 200 HTTP code
    long http_code=0;
    if (curl_easy_getinfo(((Reader_libcurl_curl_data_getregion*)data)->Curl, CURLINFO_RESPONSE_CODE, &http_code)!=CURLE_OK || http_code!=200)
    {
        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(((Reader_libcurl_curl_data_getregion*)data)->File_Name)+__T(", ")+Ztring().From_UTF8(string((char*)ptr, size*nmemb)));
        return size*nmemb;
    }

    //Must be a LocationConstraint XML
    tinyxml2::XMLDocument document;
    if (!document.Parse((const char*)ptr, size*nmemb))
    {
        XMLElement* Root=document.FirstChildElement("LocationConstraint");
        if (Root)
        {
            const char* Text=Root->GetText();
            if (Text)
                ((Reader_libcurl_curl_data_getregion*)data)->Amazon_AWS_Region=Text;
            else if (!Root->FirstChild())
                ((Reader_libcurl_curl_data_getregion*)data)->Amazon_AWS_Region="us-east-1"; //LocationConstraint XML present + empty content means that it is the standard region
        }
    }

    //Error is displayed if region is not filled
    if (((Reader_libcurl_curl_data_getregion*)data)->Amazon_AWS_Region.empty())
    {
        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(((Reader_libcurl_curl_data_getregion*)data)->File_Name)+__T(", ")+Ztring().From_UTF8(string((char*)ptr, size*nmemb)));
        return 0;
    }

    return size*nmemb;
}

//---------------------------------------------------------------------------
void Amazon_AWS_Sign(MediaInfoLib::String &File_Name, struct curl_slist* &HttpHeader, const Http::Url &File_URL, const string &regionName, const string &serviceName)
{
    // Amazon AWS Authorization Header v4
    // See http://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    char timeStamp_Buffer[17];
    time_t timeStamp_Time=time(0);

    strftime(timeStamp_Buffer, sizeof(timeStamp_Buffer), "%Y%m%dT%H%M%SZ", gmtime(&timeStamp_Time));

    string timeStamp(timeStamp_Buffer);
    string dateStamp=timeStamp.substr(0, 8);

    //SigningKey
    string SigningKey; SigningKey.resize(HASH_OUTPUT_SIZE);
    string Secret="AWS4"+File_URL.Password;
    hmac_sha((int8u*)Secret.c_str(), (unsigned long)Secret.size(), (int8u*)dateStamp.c_str(), (unsigned long)dateStamp.size(), (int8u*)SigningKey.c_str(), HASH_OUTPUT_SIZE);
    hmac_sha((int8u*)SigningKey.c_str(), (unsigned long)HASH_OUTPUT_SIZE, (int8u*)regionName.c_str(), (unsigned long)regionName.size(), (int8u*)SigningKey.c_str(), HASH_OUTPUT_SIZE);
    hmac_sha((int8u*)SigningKey.c_str(), (unsigned long)HASH_OUTPUT_SIZE, (int8u*)serviceName.c_str(), (unsigned long)serviceName.size(), (int8u*)SigningKey.c_str(), HASH_OUTPUT_SIZE);
    hmac_sha((int8u*)SigningKey.c_str(), (unsigned long)HASH_OUTPUT_SIZE, (int8u*)"aws4_request", 12, (int8u*)SigningKey.c_str(), HASH_OUTPUT_SIZE);

    //CanonicalRequest
    string CanonicalRequest;
    CanonicalRequest+="GET\n";
    CanonicalRequest+=File_URL.Path+'\n';
    if (!File_URL.Search.empty())
    {
        CanonicalRequest+=File_URL.Search;
        if (File_URL.Search[File_URL.Search.size()-1]!='=')
            CanonicalRequest+='=';
    }
    CanonicalRequest+='\n';
    CanonicalRequest+="host:"+File_URL.Host+'\n';
    CanonicalRequest+="x-amz-content-sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n";
    CanonicalRequest+="x-amz-date:"+timeStamp+'\n';
    CanonicalRequest+='\n';
    CanonicalRequest+="host;x-amz-content-sha256;x-amz-date\n";
    CanonicalRequest+="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    string CanonicalRequest_Signature=HashWrapper::Generate(HashWrapper::SHA256, (const int8u*)CanonicalRequest.c_str(), CanonicalRequest.size());

    //StringToSign
    string StringToSign;
    StringToSign+="AWS4-HMAC-SHA256\n";
    StringToSign+=timeStamp+'\n';
    StringToSign+=dateStamp+"/"+regionName+"/"+serviceName+"/aws4_request\n";
    StringToSign+=CanonicalRequest_Signature;
    string Signature; Signature.resize(HASH_OUTPUT_SIZE);
    hmac_sha((int8u*)SigningKey.c_str(), (unsigned long)HASH_OUTPUT_SIZE, (int8u*)StringToSign.c_str(), (unsigned long)StringToSign.size(), (int8u*)Signature.c_str(), HASH_OUTPUT_SIZE);

    //Authorization
    string Amazon_S3_Authorization="AWS4-HMAC-SHA256 Credential="+File_URL.User+"/"+dateStamp+"/"+regionName+"/"+serviceName+"/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature="+HashWrapper::Hex2String((const int8u*)Signature.c_str(), Signature.size());

    //Set cUrl headers
    HttpHeader=curl_slist_append(HttpHeader, ("Authorization: "+Amazon_S3_Authorization).c_str());
    HttpHeader=curl_slist_append(HttpHeader, "x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    HttpHeader=curl_slist_append(HttpHeader, ("x-amz-date: "+timeStamp).c_str());
};

//---------------------------------------------------------------------------
string Amazon_AWS_GetRegion(const string &serviceName, const string &bucketName, const Http::Url &File_URL, CURL* Curl)
{
    //Transform the URL to a location request URL
    Http::Url File_URL2=File_URL;
    File_URL2.Host.erase(0, bucketName.size()+1);
    File_URL2.Path='/'+bucketName;
    File_URL2.Search="location";

    //Send a location request
    Reader_libcurl_curl_data_getregion Curl_Data2;
    Curl_Data2.Curl=Curl;
    Curl_Data2.File_Name.From_UTF8(File_URL2.ToString());
    struct curl_slist* HttpHeader=NULL;
    Amazon_AWS_Sign(Curl_Data2.File_Name, HttpHeader, File_URL2, "us-east-1", serviceName);
    string FileName_String=Curl_Data2.File_Name.To_UTF8();
    curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, &libcurl_WriteData_CallBack_Amazon_AWS_Region);
    curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Curl_Data2);
    curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, HttpHeader);
    curl_easy_setopt(Curl, CURLOPT_URL, FileName_String.c_str());
    CURLcode Result=curl_easy_perform(Curl);
    curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(Curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, NULL);
    curl_easy_setopt(Curl, CURLOPT_URL, NULL);
    curl_slist_free_all(HttpHeader);

    //We get the region name
    return Curl_Data2.Amazon_AWS_Region;
}

void Amazon_AWS_Manage(Http::Url &File_URL, Reader_libcurl::curl_data* Curl_Data)
{
    // Looking for style of URL (Virtual-hosted–style URL Path-style URL), service name, region name
    // Format:
    // <bucket>.<service>.<aws-region>
    // <service>.<aws-region>
    // bucket can have dots
    // Exception: s3 and s3-website services, <service>-<aws-region> means <service>.<aws-region>
    // Exception: s3 on the right means unknown region
    // Exception: s3-external-1 on the right means unknown region
    string regionName(File_URL.Host, 0, File_URL.Host.size()-14);
    string serviceName;

    // Handling of exceptions
    size_t AfterLastDot=regionName.rfind('.');
    if (AfterLastDot==string::npos)
        AfterLastDot=0;
    else
        AfterLastDot++;
    if (regionName.substr(AfterLastDot)=="s3" || regionName.substr(AfterLastDot)=="s3-external-1")
    {
        serviceName="s3";
        if (AfterLastDot)
        {
            string bucketName=regionName.substr(0, AfterLastDot-1); 
            regionName=Amazon_AWS_GetRegion(serviceName, bucketName, File_URL, Curl_Data->Curl);
        }
        else
            regionName="us-east-1";
    }
    else
    {
        if (regionName.find("s3-website-", AfterLastDot)==AfterLastDot)
        {
            regionName[AfterLastDot+10]='.';
        }
        else if (regionName.find("s3-", AfterLastDot)==AfterLastDot)
        {
            regionName[AfterLastDot+2]='.';
        }
    }
    
    // Splitting regionName and serviceName
    size_t LastDot=regionName.rfind('.');
    if (LastDot!=string::npos)
    {
        serviceName=regionName.substr(0, LastDot);
        regionName=regionName.substr(LastDot+1);
        LastDot=serviceName.rfind('.');
        if (LastDot!=string::npos)
            serviceName=serviceName.substr(LastDot + 1);
    }

    // Handling S3 signature
    if (serviceName=="s3" && !regionName.empty())
    {
        Amazon_AWS_Sign(Curl_Data->File_Name, Curl_Data->HttpHeader, File_URL, regionName, serviceName);

        //Removing the login/pass FTP-style, not usable by libcurl
        File_URL.User.clear();
        File_URL.Password.clear();
        Curl_Data->File_Name=Ztring().From_UTF8(File_URL.ToString());
    }
}

//---------------------------------------------------------------------------
size_t libcurl_WriteData_CallBack(void *ptr, size_t size, size_t nmemb, void *data)
{
    #ifdef MEDIAINFO_DEBUG
        ((Reader_libcurl::curl_data*)data)->Debug_BytesRead_Total+=size*nmemb;
        ((Reader_libcurl::curl_data*)data)->Debug_BytesRead+=size*nmemb;
    #endif //MEDIAINFO_DEBUG

    //Init
    if (!((Reader_libcurl::curl_data*)data)->Init_AlreadyDone)
    {
        Http::Url File_URL=Http::Url(Ztring(((Reader_libcurl::curl_data*)data)->File_Name).To_UTF8());
        if (File_URL.Protocol=="http" || File_URL.Protocol=="https")
        {
            long http_code = 0;
            if (curl_easy_getinfo (((Reader_libcurl::curl_data*)data)->Curl, CURLINFO_RESPONSE_CODE, &http_code)!=CURLE_OK || http_code != 200)
            {
                MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(((Reader_libcurl::curl_data*)data)->File_Name)+__T(", ")+Ztring().From_UTF8(string((char*)ptr, size*nmemb)));
                return size*nmemb;
            }
        }
        double File_SizeD;
        CURLcode Result=curl_easy_getinfo(((Reader_libcurl::curl_data*)data)->Curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &File_SizeD);
        if (Result==CURLE_OK && File_SizeD==0)
        {
            ((Reader_libcurl::curl_data*)data)->Init_NotAFile=true;
            return 0; //Great chances it is FTP file listing due to interogation mark in the file name
        }
        else if (Result==CURLE_OK && File_SizeD!=-1)
        {
            ((Reader_libcurl::curl_data*)data)->MI->Open_Buffer_Init((int64u)File_SizeD, ((Reader_libcurl::curl_data*)data)->File_Name);
        }
        else
            ((Reader_libcurl::curl_data*)data)->MI->Open_Buffer_Init((int64u)-1, ((Reader_libcurl::curl_data*)data)->File_Name);
        #if MEDIAINFO_EVENTS
            ((Reader_libcurl::curl_data*)data)->Stream_Offset=0;
        #endif //MEDIAINFO_EVENTS
        ((Reader_libcurl::curl_data*)data)->Init_AlreadyDone=true;
    }

    #if MEDIAINFO_EVENTS
        if (size*nmemb)
        {
            struct MediaInfo_Event_Global_BytesRead_0 Event;
            memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_Generic));
            Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_Global_BytesRead, 0);
            Event.EventSize=sizeof(struct MediaInfo_Event_Global_BytesRead_0);
            Event.StreamIDs_Size=0;
            Event.StreamOffset=((Reader_libcurl::curl_data*)data)->Stream_Offset;
            Event.Content_Size=size*nmemb;
            Event.Content=(int8u*)ptr;
            ((Reader_libcurl::curl_data*)data)->MI->Config.Event_Send(NULL, (const int8u*)&Event, sizeof(MediaInfo_Event_Global_BytesRead_0));
            ((Reader_libcurl::curl_data*)data)->Stream_Offset+=size*nmemb;
        }
    #endif //MEDIAINFO_EVENTS

    //Continue
    ((Reader_libcurl::curl_data*)data)->Status=((Reader_libcurl::curl_data*)data)->MI->Open_Buffer_Continue((int8u*)ptr, size*nmemb);
    time_t CurrentTime = time(0);

    if (((Reader_libcurl::curl_data*)data)->Status[File__Analyze::IsFinished] || (((Reader_libcurl::curl_data*)data)->Time_Max && CurrentTime>=((Reader_libcurl::curl_data*)data)->Time_Max))
    {
        return 0;
    }

    //GoTo
    if (((Reader_libcurl::curl_data*)data)->MI->Open_Buffer_Continue_GoTo_Get()!=(int64u)-1)
    {
        return 0;
    }

    //Continue parsing
    return size*nmemb;
}

bool Reader_libcurl_HomeIsSet()
{
    return getenv("HOME")?true:false;
}

Ztring Reader_libcurl_ExpandFileName(const Ztring &FileName)
{
    Ztring FileName_Modified(FileName);
    if (FileName_Modified.find(__T("$HOME"))==0)
    {
        char* env=getenv("HOME");
        if (env)
            FileName_Modified.FindAndReplace(__T("$HOME"), Ztring().From_Local(env));
    }
    if (FileName_Modified.find(__T("~"))==0)
    {
        char* env=getenv("HOME");
        if (env)
            FileName_Modified.FindAndReplace(__T("~"), Ztring().From_Local(env));
    }
    return FileName_Modified;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Reader_libcurl::Reader_libcurl ()
{
    Curl_Data=NULL;
}

//---------------------------------------------------------------------------
Reader_libcurl::~Reader_libcurl ()
{
    if (Curl_Data==NULL)
        return;

    //Cleanup
    #if MEDIAINFO_NEXTPACKET
        if (Curl_Data->CurlM)
        {
             curl_multi_remove_handle(Curl_Data->CurlM, Curl_Data->Curl);
             curl_multi_cleanup(Curl_Data->CurlM);
        }
    #endif //MEDIAINFO_NEXTPACKET
    if (Curl_Data->Curl)
        curl_easy_cleanup(Curl_Data->Curl);
    if (Curl_Data->HttpHeader)
        curl_slist_free_all(Curl_Data->HttpHeader);
    delete Curl_Data; //Curl_Data=NULL;
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
size_t Reader_libcurl::Format_Test(MediaInfo_Internal* MI, String File_Name)
{
    #if defined MEDIAINFO_LIBCURL_DLL_RUNTIME
        if (libcurl_Module_Count==0)
        {
            size_t Errors=0;

            /* Load library */
            #ifdef MEDIAINFO_GLIBC
                libcurl_Module=g_module_open(MEDIAINFODLL_NAME, G_MODULE_BIND_LAZY);
            #elif defined (_WIN32) || defined (WIN32)
                libcurl_Module=LoadLibrary(MEDIAINFODLL_NAME);
            #else
                libcurl_Module=dlopen(MEDIAINFODLL_NAME, RTLD_LAZY);
                if (!libcurl_Module)
                    libcurl_Module=dlopen("./" MEDIAINFODLL_NAME, RTLD_LAZY);
                if (!libcurl_Module)
                    libcurl_Module=dlopen("/usr/local/lib/" MEDIAINFODLL_NAME, RTLD_LAZY);
                if (!libcurl_Module)
                    libcurl_Module=dlopen("/usr/local/lib64/" MEDIAINFODLL_NAME, RTLD_LAZY);
                if (!libcurl_Module)
                    libcurl_Module=dlopen("/usr/lib/" MEDIAINFODLL_NAME, RTLD_LAZY);
                if (!libcurl_Module)
                    libcurl_Module=dlopen("/usr/lib64/" MEDIAINFODLL_NAME, RTLD_LAZY);
            #endif
            if (!libcurl_Module)
            {
                #if MEDIAINFO_EVENTS
                    MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(File_Name)+__T(", Libcurl library is not found"));
                #endif //MEDIAINFO_EVENTS
                return 0;
            }

            /* Load methods */
            MEDIAINFO_ASSIGN    (curl_easy_init,            "curl_easy_init")
            MEDIAINFO_ASSIGN    (curl_easy_setopt,          "curl_easy_setopt")
            MEDIAINFO_ASSIGN    (curl_easy_perform,         "curl_easy_perform")
            MEDIAINFO_ASSIGN    (curl_easy_cleanup,         "curl_easy_cleanup")
            MEDIAINFO_ASSIGN    (curl_easy_getinfo,         "curl_easy_getinfo")
            MEDIAINFO_ASSIGN    (curl_slist_append,         "curl_slist_append")
            MEDIAINFO_ASSIGN    (curl_slist_free_all,       "curl_slist_free_all")
            MEDIAINFO_ASSIGN    (curl_easy_duphandle,       "curl_easy_duphandle")
            MEDIAINFO_ASSIGN    (curl_easy_strerror,        "curl_easy_strerror")
            MEDIAINFO_ASSIGN    (curl_version_info,         "curl_version_info")
            MEDIAINFO_ASSIGN    (curl_multi_init,           "curl_multi_init")
            MEDIAINFO_ASSIGN    (curl_multi_add_handle,     "curl_multi_add_handle")
            MEDIAINFO_ASSIGN    (curl_multi_remove_handle,  "curl_multi_remove_handle")
            MEDIAINFO_ASSIGN    (curl_multi_perform,        "curl_multi_perform")
            MEDIAINFO_ASSIGN    (curl_multi_cleanup,        "curl_multi_cleanup")
            if (Errors>0)
            {
                #if MEDIAINFO_EVENTS
                    MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(File_Name)+__T(", Libcurl library is not correctly loaded"));
                #endif //MEDIAINFO_EVENTS
                return 0;
            }

            libcurl_Module_Count++;
        }
    #endif //defined MEDIAINFO_LIBCURL_DLL_RUNTIME

    #if MEDIAINFO_EVENTS
        {
            string File_Name_Local=Ztring(File_Name).To_Local();
            wstring File_Name_Unicode=Ztring(File_Name).To_Unicode();
            struct MediaInfo_Event_General_Start_0 Event;
            memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_General_Start_0));
            Event.StreamIDs_Size=0;
            Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_General_Start, 0);
            Event.Stream_Size=(int64u)-1;
            Event.FileName=File_Name_Local.c_str();
            Event.FileName_Unicode=File_Name_Unicode.c_str();
            MI->Config.Event_Send(NULL, (const int8u*)&Event, sizeof(MediaInfo_Event_General_Start_0));
        }
    #endif //MEDIAINFO_EVENTS

    //With Parser MultipleParsing
    return Format_Test_PerParser(MI, File_Name);
}

//---------------------------------------------------------------------------
size_t Reader_libcurl::Format_Test_PerParser(MediaInfo_Internal* MI, const String &File_Name)
{
    #if defined MEDIAINFO_LIBCURL_DLL_RUNTIME
        if (libcurl_Module_Count==0)
            return 0; //No libcurl library
    #endif //defined MEDIAINFO_LIBCURL_DLL_RUNTIME

    Curl_Data=new curl_data();
    Curl_Data->Ssl_CertificateFileName=MediaInfoLib::Config.Ssl_CertificateFileName_Get().To_Local();
    Curl_Data->Ssl_CertificateFormat=MediaInfoLib::Config.Ssl_CertificateFormat_Get().To_Local();
    Curl_Data->Ssl_PrivateKeyFileName=MediaInfoLib::Config.Ssl_PrivateKeyFileName_Get().To_Local();
    Curl_Data->Ssl_PrivateKeyFormat=MediaInfoLib::Config.Ssl_PrivateKeyFormat_Get().To_Local();
    Curl_Data->Ssl_CertificateAuthorityFileName=MediaInfoLib::Config.Ssl_CertificateAuthorityFileName_Get().To_Local();
    Curl_Data->Ssl_CertificateAuthorityPath=MediaInfoLib::Config.Ssl_CertificateAuthorityPath_Get().To_Local();
    Curl_Data->Ssl_CertificateRevocationListFileName=MediaInfoLib::Config.Ssl_CertificateRevocationListFileName_Get().To_Local();
    Curl_Data->Ssl_IgnoreSecurity=MediaInfoLib::Config.Ssl_IgnoreSecurity_Get();
    Curl_Data->Ssh_PublicKeyFileName=Reader_libcurl_ExpandFileName(MediaInfoLib::Config.Ssh_PublicKeyFileName_Get()).To_Local();
    if (Curl_Data->Ssh_PublicKeyFileName.empty())
    {
        if (Reader_libcurl_HomeIsSet())
        {
            Ztring Temp=Reader_libcurl_ExpandFileName(__T("$HOME/.ssh/id_rsa.pub"));
            if (File::Exists(Temp))
                Curl_Data->Ssh_PublicKeyFileName=Temp.To_Local();
        }
        else
        {
            if (File::Exists(__T("id_rsa.pub")))
                Curl_Data->Ssh_PublicKeyFileName="id_rsa.pub";
        }
    }
    Curl_Data->Ssh_PrivateKeyFileName=Reader_libcurl_ExpandFileName(MediaInfoLib::Config.Ssh_PrivateKeyFileName_Get()).To_Local();
    if (Curl_Data->Ssh_PrivateKeyFileName.empty())
    {
        if (Reader_libcurl_HomeIsSet())
        {
            Ztring Temp=Reader_libcurl_ExpandFileName(__T("$HOME/.ssh/id_rsa"));
            if (File::Exists(Temp))
                Curl_Data->Ssh_PrivateKeyFileName=Temp.To_Local();
        }
        else
        {
            if (File::Exists(__T("id_rsa")))
                Curl_Data->Ssh_PrivateKeyFileName = "id_rsa";
        }
    }
    Curl_Data->Ssh_KnownHostsFileName=Reader_libcurl_ExpandFileName(MediaInfoLib::Config.Ssh_KnownHostsFileName_Get()).To_Local();
    if (Curl_Data->Ssh_KnownHostsFileName.empty())
    {
        if (Reader_libcurl_HomeIsSet())
            Curl_Data->Ssh_KnownHostsFileName=Reader_libcurl_ExpandFileName(__T("$HOME/.ssh/known_hosts")).To_Local();
        else
            Curl_Data->Ssh_KnownHostsFileName="known_hosts";
    }
    Curl_Data->Ssh_IgnoreSecurity=MediaInfoLib::Config.Ssh_IgnoreSecurity_Get();
    Curl_Data->Curl=curl_easy_init();
    if (Curl_Data->Curl==NULL)
        return 0;
    #if MEDIAINFO_NEXTPACKET
        if (MI->Config.NextPacket_Get())
        {
            Curl_Data->CurlM=curl_multi_init( );
            if (Curl_Data->CurlM==NULL)
                return 0;
            CURLMcode CodeM=curl_multi_add_handle(Curl_Data->CurlM, Curl_Data->Curl);
            if (CodeM!=CURLM_OK)
                return 0;
            Curl_Data->NextPacket=true;
        }
    #endif //MEDIAINFO_NEXTPACKET
    Curl_Data->MI=MI;
    Curl_Data->File_Name=File_Name;
    if (MI->Config.File_TimeToLive_Get())
        Curl_Data->Time_Max=time(0)+(time_t)MI->Config.File_TimeToLive_Get();
    if (!MI->Config.File_Curl_Get(__T("UserAgent")).empty())
        curl_easy_setopt(Curl_Data->Curl, CURLOPT_USERAGENT, MI->Config.File_Curl_Get(__T("UserAgent")).To_Local().c_str());
    if (!MI->Config.File_Curl_Get(__T("Proxy")).empty())
        curl_easy_setopt(Curl_Data->Curl, CURLOPT_PROXY, MI->Config.File_Curl_Get(__T("Proxy")).To_Local().c_str());
    if (!MI->Config.File_Curl_Get(__T("HttpHeader")).empty())
    {
        ZtringList HttpHeaderStrings; HttpHeaderStrings.Separator_Set(0, EOL); //End of line is set depending of the platform: \n on Linux, \r on Mac, or \r\n on Windows
        HttpHeaderStrings.Write(MI->Config.File_Curl_Get(__T("HttpHeader")));
        for (size_t Pos=0; Pos<HttpHeaderStrings.size(); Pos++)
            Curl_Data->HttpHeader=curl_slist_append(Curl_Data->HttpHeader, HttpHeaderStrings[Pos].To_Local().c_str());
    }

    //Special cases
    Http::Url File_URL=Http::Url(Ztring(File_Name).To_UTF8());
    if (!File_URL.Protocol.empty())
    {
        // Amazon S3 specific credentials
        if ((File_URL.Protocol=="http" || File_URL.Protocol=="https") && !File_URL.User.empty())
        {
            if (File_URL.Host.find(".amazonaws.com")+14==File_URL.Host.size())
                Amazon_AWS_Manage(File_URL, Curl_Data);
        }

        if (File_URL.Protocol=="sftp" || File_URL.Protocol=="scp")
        {
            if (!Curl_Data->Ssh_PublicKeyFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSH_PUBLIC_KEYFILE, Curl_Data->Ssh_PublicKeyFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        if (Result==CURLE_UNKNOWN_TELNET_OPTION)
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0xF1010101, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", The Curl library you use has no support for secure connections."));
                        else
                        {
                            Ztring MessageString;
                            MessageString.From_Local(Curl_Data->ErrorBuffer);
                            if (MessageString.empty())
                                MessageString.From_Local(curl_easy_strerror(Result));
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                        }
                        Curl_Data->ErrorBuffer[0]='\0';
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssh_PrivateKeyFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSH_PRIVATE_KEYFILE, Curl_Data->Ssh_PrivateKeyFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        if (Result==CURLE_UNKNOWN_TELNET_OPTION)
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0xF1010101, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", The Curl library you use has no support for secure connections."));
                        else
                        {
                            Ztring MessageString;
                            MessageString.From_Local(Curl_Data->ErrorBuffer);
                            if (MessageString.empty())
                                MessageString.From_Local(curl_easy_strerror(Result));
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                        }
                        Curl_Data->ErrorBuffer[0]='\0';
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssh_IgnoreSecurity)
            {
                #if !defined(MEDIAINFO_LIBCURL_DLL_RUNTIME)
                    #if LIBCURL_VERSION_NUM >= 0x071306
                        #define MEDIAINFO_LIBCURL_CURLOPT_SSH_KNOWNHOSTS
                    #endif
                #else
                    #define MEDIAINFO_LIBCURL_CURLOPT_SSH_KNOWNHOSTS
                #endif
                #ifdef MEDIAINFO_LIBCURL_CURLOPT_SSH_KNOWNHOSTS
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSH_KNOWNHOSTS, Curl_Data->Ssh_KnownHostsFileName.c_str());
                #else //MEDIAINFO_LIBCURL_CURLOPT_SSH_KNOWNHOSTS
                const CURLcode Result=CURLE_UNKNOWN_TELNET_OPTION;
                #endif //MEDIAINFO_LIBCURL_CURLOPT_SSH_KNOWNHOSTS
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        if (Result==CURLE_UNKNOWN_TELNET_OPTION)
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0xF1010102, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", The Curl library you use has no support for known_host security file, transfer would not be secure."));
                        else
                        {
                            Ztring MessageString;
                            MessageString.From_Local(Curl_Data->ErrorBuffer);
                            if (MessageString.empty())
                                MessageString.From_Local(curl_easy_strerror(Result));
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                        }
                        Curl_Data->ErrorBuffer[0]='\0';
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }
        }

        if (File_URL.Protocol=="https" || File_URL.Protocol=="ftps")
        {
            if (!Curl_Data->Ssl_CertificateFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSLCERT, Curl_Data->Ssl_CertificateFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_CertificateFormat.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSLCERTTYPE, Curl_Data->Ssl_CertificateFormat.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_PrivateKeyFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSLKEY, Curl_Data->Ssl_PrivateKeyFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_PrivateKeyFormat.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSLKEYTYPE, Curl_Data->Ssl_PrivateKeyFormat.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_CertificateAuthorityFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_CAINFO, Curl_Data->Ssl_CertificateAuthorityFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_CertificateAuthorityPath.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_CAPATH, Curl_Data->Ssl_CertificateAuthorityPath.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (!Curl_Data->Ssl_CertificateRevocationListFileName.empty())
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_CRLFILE, Curl_Data->Ssl_CertificateRevocationListFileName.c_str());
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }

            if (Curl_Data->Ssl_IgnoreSecurity)
            {
                CURLcode Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSL_VERIFYPEER, 0);
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }

                Result=curl_easy_setopt(Curl_Data->Curl, CURLOPT_SSL_VERIFYHOST, 0);
                if (Result)
                {
                    #if MEDIAINFO_EVENTS
                        Ztring MessageString;
                        MessageString.From_Local(Curl_Data->ErrorBuffer);
                        if (MessageString.empty())
                            MessageString.From_Local(curl_easy_strerror(Result));
                        Curl_Data->ErrorBuffer[0]='\0';
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                    #endif //MEDIAINFO_EVENTS
                    return 0;
                }
            }
        }
    }
    string FileName_String=Ztring(Curl_Data->File_Name).To_UTF8();
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_URL, FileName_String.c_str());
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_MAXREDIRS, 3);
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_WRITEFUNCTION, &libcurl_WriteData_CallBack);
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_WRITEDATA, Curl_Data);
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_ERRORBUFFER, Curl_Data->ErrorBuffer);
    curl_easy_setopt(Curl_Data->Curl, CURLOPT_HTTPHEADER, Curl_Data->HttpHeader);

    //Test the format with buffer
    return Format_Test_PerParser_Continue(MI);
}

//---------------------------------------------------------------------------
size_t Reader_libcurl::Format_Test_PerParser_Continue (MediaInfo_Internal* MI)
{
    bool StopAfterFilled=MI->Config.File_StopAfterFilled_Get();
    bool ShouldContinue=true;

    #if MEDIAINFO_DEMUX
    //PerPacket
    if (ShouldContinue && MI->Config.Demux_EventWasSent)
    {
        MI->Config.Demux_EventWasSent=false;

        Curl_Data->Status=MI->Open_Buffer_Continue(NULL, 0);

        //Demux
        if (MI->Config.Demux_EventWasSent)
            return 2; //Must return immediately

        //Threading
        if (MI->IsTerminating())
            return 1; //Termination is requested

        if (Curl_Data->Status[File__Analyze::IsFinished] || (StopAfterFilled && Curl_Data->Status[File__Analyze::IsFilled]))
            ShouldContinue=false;
    }
    #endif //MEDIAINFO_DEMUX

    if (ShouldContinue)
    {
        CURLcode Result=CURLE_WRITE_ERROR;
        while ((!(Curl_Data->Status[File__Analyze::IsFinished] || (StopAfterFilled && Curl_Data->Status[File__Analyze::IsFilled]))) && Result==CURLE_WRITE_ERROR)
        {
            //GoTo
            if (Curl_Data->MI->Open_Buffer_Continue_GoTo_Get()!=(int64u)-1)
            {
                #ifdef MEDIAINFO_DEBUG
                    std::cout<<std::hex<<Curl_Data->File_Offset-Curl_Data->Debug_BytesRead<<" - "<<Curl_Data->File_Offset<<" : "<<std::dec<<Curl_Data->Debug_BytesRead<<" bytes"<<std::endl;
                    Curl_Data->Debug_BytesRead=0;
                    Curl_Data->Debug_Count++;
                #endif //MEDIAINFO_DEBUG
                CURLcode Code;
                CURL* Temp=curl_easy_duphandle(Curl_Data->Curl);
                if (Temp==0)
                    return 0;
                #if MEDIAINFO_NEXTPACKET
                    if (Curl_Data->CurlM)
                            curl_multi_remove_handle(Curl_Data->CurlM, Curl_Data->Curl);
                #endif //MEDIAINFO_NEXTPACKET
                curl_easy_cleanup(Curl_Data->Curl); Curl_Data->Curl=Temp;
                #if MEDIAINFO_NEXTPACKET
                    if (Curl_Data->CurlM)
                            curl_multi_add_handle(Curl_Data->CurlM, Curl_Data->Curl);
                #endif //MEDIAINFO_NEXTPACKET
                if (Curl_Data->MI->Open_Buffer_Continue_GoTo_Get()<0x80000000)
                {
                    //We do NOT use large version if we can, because some version (tested: 7.15 linux) do NOT like large version (error code 18)
                    long File_GoTo_Long=(long)Curl_Data->MI->Open_Buffer_Continue_GoTo_Get();
                    Code=curl_easy_setopt(Curl_Data->Curl, CURLOPT_RESUME_FROM, File_GoTo_Long);
                }
                else
                {
                    curl_off_t File_GoTo_Off=(curl_off_t)Curl_Data->MI->Open_Buffer_Continue_GoTo_Get();
                    Code=curl_easy_setopt(Curl_Data->Curl, CURLOPT_RESUME_FROM_LARGE, File_GoTo_Off);
                }
                if (Code==CURLE_OK)
                {
                    #if MEDIAINFO_EVENTS
                        Curl_Data->Stream_Offset=Curl_Data->MI->Open_Buffer_Continue_GoTo_Get();
                    #endif //MEDIAINFO_EVENTS
                    MI->Open_Buffer_Init((int64u)-1, Curl_Data->MI->Open_Buffer_Continue_GoTo_Get());
                }
            }

            //Parsing
            #if MEDIAINFO_NEXTPACKET
                if (Curl_Data->NextPacket)
                {
                    int running_handles=0;
                    do
                    {
                        CURLMcode CodeM=curl_multi_perform(Curl_Data->CurlM, &running_handles);
                        if (Result==CURLE_WRITE_ERROR && Curl_Data->Init_NotAFile)
                        {
                            //Not possible to get the file with UTF-8, trying local code page
                            Curl_Data->Init_NotAFile=false;
                            string FileName_String=Ztring(Curl_Data->File_Name).To_Local();
                            curl_easy_setopt(Curl_Data->Curl, CURLOPT_URL, FileName_String.c_str());
                            CodeM=curl_multi_perform(Curl_Data->CurlM, &running_handles);
                        }
                        if (CodeM!=CURLM_OK && CodeM!=CURLM_CALL_MULTI_PERFORM)
                            break; //There is a problem
                        #if MEDIAINFO_DEMUX
                            if (MI->Config.Demux_EventWasSent)
                                return 2; //Must return immediately
                        #endif //MEDIAINFO_DEMUX
                        if (running_handles==0)
                            break; //cUrl has finished
                    }
                    while (running_handles);
                    if (running_handles==0 && Curl_Data->MI->Open_Buffer_Continue_GoTo_Get()==(int64u)-1)
                        break; //cUrl has finished
                    Result=CURLE_WRITE_ERROR; //Configuring as if classic method is used
                }
                else
            #endif //MEDIAINFO_NEXTPACKET
            {
                Result=curl_easy_perform(Curl_Data->Curl);
                if (Result==CURLE_WRITE_ERROR && Curl_Data->Init_NotAFile)
                {
                    //Not possible to get the file with UTF-8, trying local code page
                    Curl_Data->Init_NotAFile=false;
                    string FileName_String=Ztring(Curl_Data->File_Name).To_Local();
                    curl_easy_setopt(Curl_Data->Curl, CURLOPT_URL, FileName_String.c_str());
                    Result=curl_easy_perform(Curl_Data->Curl);
                }
            }

            if (Result!=CURLE_OK && Result!=CURLE_WRITE_ERROR)
            {
                #if MEDIAINFO_EVENTS
                    Ztring MessageString;
                    int32u MessageCode=0;
                    MessageString.From_Local(Curl_Data->ErrorBuffer);
                    if (MessageString.empty())
                        MessageString.From_Local(curl_easy_strerror(Result));
                    if (Result==CURLE_PEER_FAILED_VERIFICATION)
                    {
                        size_t Protocol_Limit=Curl_Data->File_Name.find(__T(":"));
                        if (Protocol_Limit!=string::npos)
                        {
                            Ztring Protocol=Curl_Data->File_Name;
                            Protocol.resize(Protocol_Limit);
                            Protocol.MakeLowerCase();
                            if (Protocol==__T("sftp") || Protocol==__T("scp"))
                            {
                                MessageString=__T("The remote server's SSH fingerprint was deemed not OK (not in your known_host file).");
                                MessageCode=0xF1010103;
                            }
                            else if (Protocol==__T("https") || Protocol==__T("ftps"))
                            {
                                MessageString=__T("The remote server's SSL certificate was deemed not OK.");
                                MessageCode=0xF1010104;
                            }
                        }
                    }
                    Curl_Data->ErrorBuffer[0]='\0';
                    MediaInfoLib::Config.Log_Send(0xC0, 0xFF, MessageCode, Reader_libcurl_FileNameWithoutPassword(Curl_Data->File_Name)+__T(", ")+MessageString);
                #endif //MEDIAINFO_EVENTS
            }

            #if MEDIAINFO_DEMUX
                if (MI->Config.Demux_EventWasSent)
                    return 2; //Must return immediately
            #endif //MEDIAINFO_DEMUX

            //Threading
            if (MI->IsTerminating())
                break; //Termination is requested
        }
    }

    #ifdef MEDIAINFO_DEBUG
        std::cout<<std::hex<<Curl_Data->File_Offset-Curl_Data->Debug_BytesRead<<" - "<<Curl_Data->File_Offset<<" : "<<std::dec<<Curl_Data->Debug_BytesRead<<" bytes"<<std::endl;
        std::cout<<"Total: "<<std::dec<<Curl_Data->Debug_BytesRead_Total<<" bytes in "<<Curl_Data->Debug_Count<<" blocks"<<std::endl;
    #endif //MEDIAINFO_DEBUG

    //Is this file detected?
    if (!Curl_Data->Status[File__Analyze::IsAccepted])
        return 0;

    MI->Open_Buffer_Finalize();

    #if MEDIAINFO_DEMUX
        if (MI->Config.Demux_EventWasSent)
            return 2; //Must return immediately
    #endif //MEDIAINFO_DEMUX

    return 1;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t Reader_libcurl::Format_Test_PerParser_Seek (MediaInfo_Internal* MI, size_t Method, int64u Value, int64u ID)
{
    size_t ToReturn=MI->Open_Buffer_Seek(Method, Value, ID);

    if (ToReturn==0 || ToReturn==1)
    {
        //Reset
        Curl_Data->Status=0;
    }

    return ToReturn;
}
#endif //MEDIAINFO_SEEK

} //NameSpace

#endif //MEDIAINFO_LIBCURL_YES

