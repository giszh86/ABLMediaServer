#ifndef _PS_MUX_DEMUX_DEMUX_MANAGER_CONSUMER_H_
#define _PS_MUX_DEMUX_DEMUX_MANAGER_CONSUMER_H_

#include <boost/serialization/singleton.hpp>
#include <boost/unordered/unordered_map.hpp>

#if (defined _WIN32 || defined _WIN64)

#include <boost/thread/mutex.hpp>

#else

#include "auto_lock.h"

#endif

#include "demux.h"

class demux_manager : public boost::serialization::singleton<demux_manager>
{
public:
	ps_demux_ptr malloc(ps_demux_callback cb, void* userdata, int32_t mode);
	void free();

	bool push(ps_demux_ptr p);
	bool pop(uint32_t h);
	ps_demux_ptr get(uint32_t h);

private:
	boost::unordered_map<uint32_t, ps_demux_ptr> m_duxmap;

#if (defined _WIN32 || defined _WIN64)

	boost::mutex  m_duxmtx;

#else

	auto_lock::al_spin m_duxspin;

#endif
};


#endif