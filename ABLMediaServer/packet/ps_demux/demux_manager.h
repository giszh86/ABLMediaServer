#pragma once
#include <memory>
#include <map>
#include <mutex>
#include "demux.h"
#include <unordered_set>
#include "../../HSingleton.h"

class demux_manager 
{
public:
	ps_demux_ptr malloc(ps_demux_callback cb, void* userdata, int32_t mode);
	void free();

	bool push(ps_demux_ptr p);
	bool pop(uint32_t h);
	ps_demux_ptr get(uint32_t h);

private:
	std::map<uint32_t, ps_demux_ptr> m_duxmap;


	std::mutex  m_duxmtx;


};


#define gblDemuxdMgrGet HSingletonTemplatePtr<demux_manager>::Instance()