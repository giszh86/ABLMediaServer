#pragma once
#include <unordered_map>
#include <mutex>
#include "../../HSingleton.h"
#include "mux.h"


class ps_mux_manager 
{
public:
	ps_mux_ptr malloc(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre);
	void free(ps_mux_ptr& p);

	bool push(ps_mux_ptr p);
	bool pop(uint32_t h);
	ps_mux_ptr get(uint32_t h);

private:
	std::unordered_map<uint32_t, ps_mux_ptr> m_muxmap;
	std::mutex  m_muxmtx;


};



#define gblPsmuxdMgrGet HSingletonTemplatePtr<ps_mux_manager>::Instance()
