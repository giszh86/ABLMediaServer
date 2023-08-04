#include <boost/unordered_set.hpp>

#if (defined _WIN32 || defined _WIN64)
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#else
#include "auto_lock.h"
#endif

#include "common.h"
#include "rtp_packet.h"

boost::unordered_set<uint32_t> g_identifier_set_rtppacket;

#if (defined _WIN32 || defined _WIN64)

boost::mutex g_identifier_mutex;
#else

auto_lock::al_spin g_identifier_spin_rtppacket;

#endif

uint32_t generate_identifier_rtppacket()
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(g_identifier_mutex);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(g_identifier_spin_rtppacket);

#endif

	static uint32_t s_id = 1;
	boost::unordered_set<uint32_t>::iterator it;

	for (;;)
	{
		it = g_identifier_set_rtppacket.find(s_id);
		if ((g_identifier_set_rtppacket.end() == it) && (0 != s_id))
		{
			std::pair<boost::unordered_set<uint32_t>::iterator, bool> ret = g_identifier_set_rtppacket.insert(s_id);
			if (ret.second)
			{
				break;	//useful
			}
		}
		else
		{
			++s_id;
		}
	}

	return s_id++;
}

void recycle_identifier_rtppacket(uint32_t id)
{
#if (defined _WIN32 || defined _WIN64)

	boost::lock_guard<boost::mutex> lg(g_identifier_mutex);

#else

	auto_lock::al_lock<auto_lock::al_spin> al(g_identifier_spin_rtppacket);

#endif

	boost::unordered_set<uint32_t>::iterator it = g_identifier_set_rtppacket.find(id);
	if (g_identifier_set_rtppacket.end() != it)
	{
		g_identifier_set_rtppacket.erase(it);
	}
}

int32_t get_mediatype(int32_t st)
{
	int32_t mt = e_rtppkt_mt_unknown;

	switch (st)
	{

	case e_rtppkt_st_mpeg1v:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_mpeg2v:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_mpeg1a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mpeg2a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mjpeg:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_aac:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mpeg4:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_aac_latm:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_h264:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_h265:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_svacv:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_pcm:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_pcmlaw:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g711a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g711u:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g7221:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g7231:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g729:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_svaca:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_hkp:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_hwp:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_dhp:
	{
		mt = e_rtppkt_mt_video;

	}break;


	default:
		break;
	}

	return mt;
}