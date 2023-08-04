#ifndef _PS_MUX_DEMUX_MUX_MANAGER_H_
#define _PS_MUX_DEMUX_MUX_MANAGER_H_

#include <boost/serialization/singleton.hpp>
#include <boost/unordered/unordered_map.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/mutex.hpp>

#else

#include "auto_lock.h"

#endif

#include "mux.h"


class ps_mux_manager : public boost::serialization::singleton<ps_mux_manager>
{
public:
	ps_mux_ptr malloc(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre);
	void free(ps_mux_ptr& p);

	bool push(ps_mux_ptr p);
	bool pop(uint32_t h);
	ps_mux_ptr get(uint32_t h);

private:
	boost::unordered_map<uint32_t, ps_mux_ptr> m_muxmap;

#if (defined _WIN32 || defined _WIN64)

	boost::mutex  m_muxmtx;

#else

	auto_lock::al_spin m_muxspin;

#endif
};

#endif