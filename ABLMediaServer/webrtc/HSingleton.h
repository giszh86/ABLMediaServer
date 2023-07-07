
#pragma once
#include <mutex>

template<typename T>
class HSingletonTemplatePtr
{
public:
	static T* Instance()
	{
		if (m_instance == nullptr)
		{
			std::lock_guard<std::mutex> lock(_mutex);  // ����
			if (m_instance == nullptr)
			{
				m_instance = new T;		
				//atexit(Destory);
			}

		}
		return m_instance;
	};

	static void Destory()
	{
		std::lock_guard<std::mutex> lock(_mutex);  // ����
		if (m_instance != nullptr)
		{
			delete m_instance;
			m_instance = nullptr;
		}
	};


private:
	HSingletonTemplatePtr() = default;
	~HSingletonTemplatePtr() = default;
	HSingletonTemplatePtr(const HSingletonTemplatePtr&) = delete;
	HSingletonTemplatePtr& operator=(const HSingletonTemplatePtr&) = delete;

private:
	static T* m_instance;
	static std::mutex _mutex;


public:
//	// This is important
//	class GarbageCollector  // ����������
//	{
//	public:
//
//		~GarbageCollector()
//		{
//			// We can destory all the resouce here, eg:db connector, file handle and so on
//			HSingletonTemplatePtr<T>::Destroy();  // �������ٺ���
//		}
//	};
//public:
//	static GarbageCollector  m_gc;  //����������ľ�̬��Ա

};

template<typename T> T*  HSingletonTemplatePtr<T>::m_instance = nullptr;


template<typename T> std::mutex HSingletonTemplatePtr<T>::_mutex;

//#define gblDownloadMgrGet HSingletonTemplatePtr<CDownloadManager>::Instance()
