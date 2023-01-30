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

   char          m_app[256];
   char          m_stream[256];
   char          m_szShareURL[256];
   char          szBuffer[256];
   char          szJson[1024];

   bool   AddRecordFile(char* szFileName);
   bool   UpdateExpireRecordFile(char* szNewFileName);

   list<uint64_t> fileList;
};

#endif