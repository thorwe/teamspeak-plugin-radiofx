#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>

#include "core/module.h"
#include "core/talkers.h"
#include "dsp_radio.h"
#include "core/ts_serversinfo.h"
#include "core/talkers.h"

#include <unordered_map>
#include <atomic>

namespace thorwe {
namespace ts {
    using connection_id_t = std::uint64_t;
    using channel_id_id = std::uint64_t;
    using client_id_t = std::uint16_t;
}
}
using namespace thorwe;

class Radio : public Module, public TalkInterface
{
    Q_OBJECT
    Q_INTERFACES(TalkInterface)
    Q_PROPERTY(ts::connection_id_t homeId READ homeId WRITE setHomeId)

public:
    explicit Radio(TSServersInfo& servers_info, Talkers& talkers, QObject* parent = nullptr);
    
    bool onTalkStatusChanged(ts::connection_id_t sch_id, int status, bool is_received_whisper, ts::client_id_t client_id, bool is_me) override;

    void setHomeId(ts::connection_id_t sch_id);
    ts::connection_id_t homeId() {return m_home_id.load();}

    bool isClientBlacklisted(ts::connection_id_t sch_id, ts::client_id_t client_id);

    // events forwarded from plugin.cpp
    void onEditPlaybackVoiceDataEvent(ts::connection_id_t sch_id, ts::client_id_t client_id, short* samples, int frame_count, int channels);

    std::unordered_map<std::string, RadioFX_Settings>& settings_map_ref();

	void setChannelStripEnabled(QString name, bool val);
	void setFudge(QString name, double val);
	void setInLoFreq(QString name, double val);
	void setInHiFreq(QString name, double val);
	void setRingModFrequency(QString name, double val);
	void setRingModMix(QString name, double val);
	void setOutLoFreq(QString name, double val);
	void setOutHiFreq(QString name, double val);

	void ToggleClientBlacklisted(ts::connection_id_t sch_id, ts::client_id_t client_id);

private:
    std::atomic<ts::connection_id_t> m_home_id = 0;
    
	TSServersInfo& m_servers_info;
	Talkers& m_talkers;
	
	std::unordered_multimap<ts::connection_id_t, std::pair<ts::client_id_t, std::unique_ptr<DspRadio>>> m_talkers_dspradios;
	DspRadio* findRadio(ts::connection_id_t sc_handler_id, ts::client_id_t client_id);

    std::unordered_map<std::string, RadioFX_Settings> m_settings_map;

    std::unordered_multimap<ts::connection_id_t, ts::client_id_t> m_client_blacklist;

protected:
    void onRunningStateChanged(bool value) override;
};
