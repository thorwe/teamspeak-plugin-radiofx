#include "dsp_radio.h"

#include <algorithm>

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif
const double TWO_PI_OVER_SAMPLE_RATE = 2*M_PI/48000;

DspRadio::DspRadio(RadioFX_Settings& settings)
	: m_settings(settings)
{
    Dsp::Params params;
    params[0] = 48000; // sample rate
    params[1] = 4; // order
    params[2] = 1600; // center frequency
    params[3] = 1300; // band width
    f_m->setParams(params);
    f_s->setParams(params);
    f_m_o->setParams(params);
    f_s_o->setParams(params);

	m_samples.reserve(480);
	m_samples_r.reserve(480);
}

void DspRadio::do_process(float *samples, int frame_count, float &volFollow)
{
    // ALL INPUTS AND OUTPUTS IN THIS ARE -1.0f and +1.0f

	const auto fudge = static_cast<float>(m_settings.fudge);

    // Find volume of current block of frames...
    float vol = 0.0f;
 //   float min = 1.0f, max = -1.0f;
    for (int i = 0; i < frame_count; i++)
    {
       vol += (samples[i] * samples[i]);
    }
    vol /= (float)frame_count;

    // Fudge factor, inrease for more noise
    vol *= fudge;

    // Smooth follow from last frame, both multiplies add up to 1...
    volFollow = volFollow * 0.5f + vol * 0.5f;

    // Between -1.0f and 1.0f...
    float random = (((float)(rand()&32767)) / 16384.0f) - 1.0f;

    // Between 1 and 128...
    int count = (rand() & 127) + 1;
    float temp;
    for (int i = 0; i < frame_count; i++)
    {
       if (!count--)
       {
//          // Between -1.0f and 1.0f...
          random = (((float)(rand()&32767)) / 16384.0f) - 1.0f;
//          // Between 1 and 128...
          count = (rand() & 127) + 1;
       }
        // Add random to inputs * by current volume;
       temp = samples[i] + random * volFollow;

       // Make it an integer between -60 and 60
       temp = (int)(temp * 40.0f);

       // Drop it back down but massively quantised and too high
       temp = (temp / 40.0f);
       temp *= 0.05 * (float)m_settings.fudge;
       temp += samples[i] * (1 - (0.05 * fudge));
       samples[i] = std::clamp(-1.0f, temp, 1.0f);
    }
}

void DspRadio::do_process_ring_mod(float *samples, int frame_count, double &modAngle)
{
	const auto rm_mod_freq = m_settings.rm_mod_freq.load();
	const auto rm_mix = m_settings.rm_mix.load();
    if ((rm_mod_freq != 0.0f) && (rm_mix != 0.0f))
    {
        for (int i = 0; i < frame_count; ++i)
        {
            float sample = samples[i];
            sample = (sample * (1 - rm_mix)) + (rm_mix * (sample * sin(modAngle)));
            samples[i] = std::clamp(-1.0f , sample, 1.0f);
            modAngle += rm_mod_freq * TWO_PI_OVER_SAMPLE_RATE;
        }
    }
}

namespace {
	double calculate_bandwidth(double low_frequency, double high_frequency)
	{
		return high_frequency - low_frequency;
	}
	double calculate_center_frequency(double low_frequency, double high_frequency)
	{
		return low_frequency + (calculate_bandwidth(low_frequency, high_frequency) / 2.0);
	}
	void update_filter_frequencies(
		double low_frequency,
		double high_frequency,
		std::pair<double, double>& last,
		const std::unique_ptr<Dsp::Filter>& mono_filter,
		const std::unique_ptr<Dsp::Filter>& stereo_filter)
	{
		if (last.first == low_frequency && last.second == high_frequency)
			return;

		// calculate center frequency and bandwidth
		const auto new_center_frequency = calculate_center_frequency(low_frequency, high_frequency);
		const auto new_bandwidth = calculate_bandwidth(low_frequency, high_frequency);
		if (mono_filter)
		{
			mono_filter->setParam(2, new_center_frequency); // center frequency
			mono_filter->setParam(3, new_bandwidth); // bandwidth
		}
		if (stereo_filter)
		{
			stereo_filter->setParam(2, new_center_frequency); // center frequency
			stereo_filter->setParam(3, new_bandwidth); // bandwidth
		}
		last.first = low_frequency;
		last.second = high_frequency;
	}
}

void DspRadio::process(short *samples, int frame_count, int channels)
{
    if (!m_settings.enabled)
        return;

	// update filters if necessary
	update_filter_frequencies(m_settings.freq_low.load(), m_settings.freq_hi.load(), m_last_eq_in, f_m, f_s);
	update_filter_frequencies(m_settings.o_freq_lo.load(), m_settings.o_freq_hi.load(), m_last_eq_out, f_m_o, f_s_o);

	const auto fudge = static_cast<float>(m_settings.fudge);

    if (channels == 1)
    {
		m_samples.resize(frame_count);
        for (int i=0; i < frame_count; ++i)
            m_samples[i] = samples[i] / 32768.f;

        float* audioData[1];
        audioData[0] = m_samples.data();
        f_m->process(frame_count, audioData);

        do_process_ring_mod(m_samples.data(), frame_count, m_rm_mod_angle);

        if (fudge > 0.0f)
            do_process(m_samples.data(), frame_count, m_vol_follow);

        f_m_o->process(frame_count, audioData);

        dsp_volume_agmu.process(samples, frame_count, channels);

        for (int i = 0; i < frame_count; ++i)
            samples[i] = static_cast<int16_t>(m_samples[i] * 32768.f);
    }
    else if (channels == 2)
    {
        // Extract from interleaved and convert to QVarLengthArray<float>
		m_samples.resize(frame_count);
		m_samples_r.resize(frame_count);
        
        for (int i = 0; i < frame_count; ++i)
        {
            m_samples[i] = samples[i*2] / 32768.f;
            m_samples_r[i] = samples[1 + (i*2)] / 32768.f;
        }

        float* audioData[2];
        audioData[0] = m_samples.data();
        audioData[1] = m_samples_r.data();
        f_s->process(frame_count, audioData);

        do_process_ring_mod(m_samples.data(), frame_count, m_rm_mod_angle);
        do_process_ring_mod(m_samples_r.data(), frame_count, m_rm_mod_angle_r);

        if (fudge > 0.0f)
        {
            do_process(m_samples.data(), frame_count, m_vol_follow);
            do_process(m_samples_r.data(), frame_count, m_vol_follow_r);
        }

        f_s_o->process(frame_count, audioData);

        dsp_volume_agmu.process(samples, frame_count, channels);

        for (int i = 0; i < frame_count; ++i)
        {
            samples[i*2] = static_cast<int16_t>(m_samples[i] * 32768.f);
            samples[1 + (i*2)] = static_cast<int16_t>(m_samples_r[i] * 32768.f);
        }
    }
}
