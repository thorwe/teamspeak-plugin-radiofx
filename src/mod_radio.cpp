#include "mod_radio.h"

#include "core/ts_helpers_qt.h"
#include "core/ts_serversinfo.h"
#include <ts3_functions.h>
#include "plugin.h"
#include "teamspeak/public_errors.h"

#include <utility>

Radio::Radio(TSServersInfo& servers_info, Talkers& talkers, QObject* parent)
	: m_servers_info(servers_info)
	, m_talkers(talkers)
{
	setParent(parent);
    setObjectName("Radio");
    m_isPrintEnabled = false;
}

void Radio::setHomeId(ts::connection_id_t sch_id)
{
    m_home_id.store(sch_id);
    
    if (sch_id != 0 && isRunning())
        m_talkers.DumpTalkStatusChanges(this, true);
}

void Radio::setChannelStripEnabled(QString name_q, bool val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.enabled.load() != val)
            it->second.enabled.store(val);
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.enabled.store(val);
        m_settings_map.try_emplace(name, std::move(setting));
    }
}

void Radio::setFudge(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.fudge != val)
            it->second.fudge = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.fudge = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setInLoFreq(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.freq_low != val)
            it->second.freq_low = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.freq_low = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setInHiFreq(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.freq_hi != val)
            it->second.freq_hi = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.freq_hi = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setRingModFrequency(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.rm_mod_freq != val)
            it->second.rm_mod_freq = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.rm_mod_freq = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setRingModMix(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.rm_mix != val)
            it->second.rm_mix = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.rm_mix = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setOutLoFreq(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.o_freq_lo != val)
            it->second.o_freq_lo = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.o_freq_lo = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::setOutHiFreq(QString name_q, double val)
{
    const auto name = name_q.toStdString();
    if (const auto it = m_settings_map.find(name); it != std::cend(m_settings_map))
    {
        if (it->second.o_freq_hi != val)
            it->second.o_freq_hi = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.o_freq_hi = val;
        m_settings_map.emplace(name, std::move(setting));
    }
}

void Radio::ToggleClientBlacklisted(ts::connection_id_t sch_id, ts::client_id_t client_id)
{
    {
        auto it = std::find_if(std::begin(m_client_blacklist), std::end(m_client_blacklist), [sch_id, client_id](const auto& test)
        {
            return test.first == sch_id && test.second == client_id;
        });
        if (it != std::end(m_client_blacklist))
            m_client_blacklist.erase(it);
        else
            m_client_blacklist.emplace(sch_id, client_id);
    }

    if (!(isRunning()))
        return;

	auto* dsp_radio = findRadio(sch_id, client_id);
	const auto enable = !isClientBlacklisted(sch_id, client_id);
	if (dsp_radio)
	{
		auto&& settings = dsp_radio->settings();
		if (settings.enabled != enable)
			settings.enabled = enable;
	}
}

DspRadio* Radio::findRadio(ts::connection_id_t sch_id, ts::client_id_t client_id)
{
	auto result = std::find_if(std::cbegin(m_talkers_dspradios), std::cend(m_talkers_dspradios), [sch_id, client_id](const auto& test) {
		return sch_id == test.first && client_id == test.second.first;
	});
	if (result != std::cend(m_talkers_dspradios))
		return result->second.second.get();

	return nullptr;
}

bool Radio::isClientBlacklisted(ts::connection_id_t sch_id, ts::client_id_t client_id)
{
    return std::find_if(std::cbegin(m_client_blacklist), std::cend(m_client_blacklist), [sch_id, client_id](const auto& test)
    {
        return test.first == sch_id && test.second == client_id;
    }) != std::cend(m_client_blacklist);
}

void Radio::onRunningStateChanged(bool value)
{
    m_talkers.DumpTalkStatusChanges(this, ((value) ? STATUS_TALKING : STATUS_NOT_TALKING));	//FlushTalkStatusChanges((value)?STATUS_TALKING:STATUS_NOT_TALKING);
    Log(QString("enabled: %1").arg((value)?"true":"false"));
}

//! Returns true iff it will or has been an active processing
bool Radio::onTalkStatusChanged(ts::connection_id_t sch_id, int status, bool is_received_whisper, ts::client_id_t client_id, bool is_me)
{
    if (is_me || !isRunning())
        return false;

    if (status == STATUS_TALKING)
    {   // Robust against multiple STATUS_TALKING in a row to be able to use it when the user changes settings

        auto* dsp_obj = findRadio(sch_id, client_id);
		if (dsp_obj)
			return dsp_obj->settings().enabled;

		// new dsp object
        auto get_settings_ref = [this, sch_id, client_id, is_received_whisper]() -> RadioFX_Settings&
        {
            if (is_received_whisper)
                return m_settings_map.at("Whisper");
            
            // get channel. Reminder: We don't get talk status changes for channels we're not in.
            uint64_t channel_id;
            auto error = ts3Functions.getChannelOfClient(sch_id, client_id, &channel_id);
            if (error != ERROR_ok)
                this->Error("Error getting channel id", sch_id, error);

            const auto server_id = m_servers_info.get_server_info(sch_id, true)->getUniqueId().toStdString();
            const auto channel_path = TSHelpers::GetChannelPath(sch_id, channel_id).toStdString();

            if (error == ERROR_ok && !channel_path.empty())
            {
                const auto settings_map_key = server_id + channel_path;

                if (const auto it = m_settings_map.find(settings_map_key); it != std::cend(m_settings_map))
                    return m_settings_map.at(settings_map_key);
            }

            if (sch_id == m_home_id.load())
                return m_settings_map.at("Home");
            
            return m_settings_map.at("Other");
        };
        auto&& settings = get_settings_ref();

		auto dsp_radio = std::make_unique<DspRadio>(settings);
		m_talkers_dspradios.emplace(sch_id, std::make_pair(client_id, std::move(dsp_radio)));

		return settings.enabled;
    }
    else if (status == STATUS_NOT_TALKING)
    {
        // Removing does not need to be robust against multiple STATUS_NOT_TALKING in a row, since that doesn't happen on user setting change
        if (
            auto it = std::find_if(std::cbegin(m_talkers_dspradios), std::cend(m_talkers_dspradios), [sch_id, client_id](const auto& test) {
                return sch_id == test.first && client_id == test.second.first;
                }); it != std::cend(m_talkers_dspradios))
        {
            const auto is_enabled = it->second.second->settings().enabled.load();
            // *should* be fine here shouldn't it?
            m_talkers_dspradios.erase(it);
            return is_enabled;
        }
    }
    return false;
}

//! Routes the arguments of the event to the corresponding volume object
/*!
 * \brief Radio::onEditPlaybackVoiceDataEvent pre-processing voice event
 * \param sch_id the connection id of the server
 * \param client_id the client-side runtime-id of the sender
 * \param samples the sample array to manipulate
 * \param frame_count amount of samples in the array
 * \param channels amount of channels
 */
void Radio::onEditPlaybackVoiceDataEvent(ts::connection_id_t sch_id, ts::client_id_t client_id, short *samples, int frame_count, int channels)
{
    if (!(isRunning()))
        return;

	auto* dsp_radio = findRadio(sch_id, client_id);
	if (dsp_radio)
		dsp_radio->process(samples, frame_count, channels);
}

std::unordered_map<std::string, RadioFX_Settings>& Radio::settings_map_ref()
{
    return m_settings_map;
}
