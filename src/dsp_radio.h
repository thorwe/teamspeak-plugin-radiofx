#pragma once

#include "volume/dsp_volume_agmu.h"

#include <DspFilters/Dsp.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

struct RadioFX_Settings
{
    RadioFX_Settings() = default;
    RadioFX_Settings(const RadioFX_Settings&) = delete;
    RadioFX_Settings(RadioFX_Settings&& other) noexcept {
        enabled.store(other.enabled.load());
        freq_hi = other.freq_hi.load();
        fudge = other.fudge.load();
        rm_mod_freq = other.rm_mod_freq.load();
        rm_mix = other.rm_mix.load();
        o_freq_lo = other.o_freq_lo.load();
        o_freq_hi = other.o_freq_hi.load();
    };

    std::atomic_bool enabled = false;
    std::atomic<double> freq_low = 0.0;
	std::atomic<double> freq_hi = 0.0;
	std::atomic<double> fudge = 0.0;
	std::atomic<double> rm_mod_freq = 0.0;
	std::atomic<double> rm_mix = 0.0;
	std::atomic<double> o_freq_lo = 0.0;
	std::atomic<double> o_freq_hi = 0.0;
};

class DspRadio
{

public:
    DspRadio(RadioFX_Settings& settings);
    
    void process(short* samples, int frame_count, int channels);

	RadioFX_Settings& settings() const { return m_settings; };

private:
    void do_process(float *samples, int frame_count, float &volFollow);
    void do_process_ring_mod(float *samples, int frame_count, double &modAngle);

	RadioFX_Settings& m_settings;

    float m_vol_follow = 0.0f;
    float m_vol_follow_r = 0.0f;

    std::unique_ptr<Dsp::Filter> f_m = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 1, Dsp::DirectFormII> >(1024);
    std::unique_ptr<Dsp::Filter> f_s = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 2, Dsp::DirectFormII> >(1024);

    std::unique_ptr<Dsp::Filter> f_m_o = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 1, Dsp::DirectFormII> >(1024);
    std::unique_ptr<Dsp::Filter> f_s_o = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 2, Dsp::DirectFormII> >(1024);

    //RingMod
    double m_rm_mod_angle = 0.0f;
    double m_rm_mod_angle_r = 0.0f;

	// last freq settings
	std::pair<double, double> m_last_eq_in;		// low, high
	std::pair<double, double> m_last_eq_out;	// low, high

    DspVolumeAGMU dsp_volume_agmu;

	std::vector<float> m_samples;
	std::vector<float> m_samples_r;
};
