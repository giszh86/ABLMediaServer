#ifndef _RecordFileSource_
#define _RecordFileSource_

class CRecordFileSource
{
public:
   CRecordFileSource(char* app,char* stream);
   ~CRecordFileSource();
   
   bool          queryRecordFile(char* szRecordFileName);
   void          Sort();
   std::mutex    RecordFileLock;
   char          szDeleteFile[512];

   char          m_app[string_length_256];
   char          m_stream[string_length_512];
   char          m_szShareURL[string_length_512];
   char          szBuffer[string_length_4096];
   char          szJson[string_length_4096];

   bool   AddRecordFile(char* szFileName);
   bool   UpdateExpireRecordFile(char* szNewFileName);

   list<uint64_t> fileList;
};

#endif