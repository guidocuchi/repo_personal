/*
Arduino-MAX30102 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Defines.h"
#include <algorithm>
extern SYSTICK Systick;



BeatDetector::BeatDetector() :
    state(BEATDETECTOR_STATE_INIT),
    threshold(BEATDETECTOR_MIN_THRESHOLD),
    beatPeriod(0),
    lastMaxValue(0),
    tsLastBeat(0)
{
}

bool BeatDetector::addSample(int32_t sample)
{
    return checkForBeat(sample);
}

/*float*/ uint32_t BeatDetector::getRate()
{
    if (beatPeriod != 0) {
        return 60000 / beatPeriod; // * 1000 * 60;
    } else {
        return 0;
    }
}

/*float*/ int32_t BeatDetector::getCurrentThreshold()
{
    return threshold;
}

bool BeatDetector::checkForBeat(int32_t value)
{
    bool beatDetected = false;

    switch (state) {
        case BEATDETECTOR_STATE_INIT:
            if (Systick.Get_milis() > BEATDETECTOR_INIT_HOLDOFF) {
                state = BEATDETECTOR_STATE_WAITING;
            }
            break;

        case BEATDETECTOR_STATE_WAITING:
            if (value > threshold) {
                threshold = std::min(value, BEATDETECTOR_MAX_THRESHOLD);
                state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
            }

            // Tracking lost, resetting
            if (Systick.Get_milis() - tsLastBeat > BEATDETECTOR_INVALID_READOUT_DELAY) {
                beatPeriod = 0;
                lastMaxValue = 0;
                threshold = BEATDETECTOR_MIN_THRESHOLD; // la agregué hoy
            }

            decreaseThreshold();
            break;

        case BEATDETECTOR_STATE_FOLLOWING_SLOPE:
            if (value < threshold) {
                state = BEATDETECTOR_STATE_MAYBE_DETECTED;
            } else {
                threshold = std::min(value, BEATDETECTOR_MAX_THRESHOLD);
            }
            break;

        case BEATDETECTOR_STATE_MAYBE_DETECTED:
            if (value + BEATDETECTOR_STEP_RESILIENCY < threshold) {
                // Found a beat
                beatDetected = true;
                lastMaxValue = value;
                state = BEATDETECTOR_STATE_MASKING;
                uint32_t delta = Systick.Get_milis() - tsLastBeat;
                if (delta) {
                    beatPeriod = (BEATDETECTOR_BPFILTER_ALPHA * delta +
                            (100 - BEATDETECTOR_BPFILTER_ALPHA) * beatPeriod)/100;
                }

                tsLastBeat = Systick.Get_milis();
            } else {
                state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
            }
            break;

        case BEATDETECTOR_STATE_MASKING:
            if (Systick.Get_milis() - tsLastBeat > BEATDETECTOR_MASKING_HOLDOFF) {
                state = BEATDETECTOR_STATE_WAITING;
            }
            decreaseThreshold();
            break;
    }

    return beatDetected;
}

void BeatDetector::decreaseThreshold()
{
//    // When a valid beat rate readout is present, target the
//    if (lastMaxValue > 0 && beatPeriod > 0) {
//        threshold -= lastMaxValue * (1 - BEATDETECTOR_THRESHOLD_FALLOFF_TARGET) /
//                (beatPeriod / BEATDETECTOR_SAMPLES_PERIOD);
//    } else {
//        // Asymptotic decay
//        threshold *= BEATDETECTOR_THRESHOLD_DECAY_FACTOR;
//    }
//
//    if (threshold < BEATDETECTOR_MIN_THRESHOLD) {
//        threshold = BEATDETECTOR_MIN_THRESHOLD;
//    }
	if(lastMaxValue > 0 && beatPeriod > 0)
	{
		int32_t decrement = (lastMaxValue * (100 - BEATDETECTOR_THRESHOLD_FALLOFF_TARGET)* BEATDETECTOR_SAMPLES_PERIOD)/beatPeriod;
		threshold -= decrement;
	}else{

		threshold = (threshold * BEATDETECTOR_THRESHOLD_DECAY_FACTOR)/100;
	}

	//threshold = (threshold * BEATDETECTOR_THRESHOLD_DECAY_FACTOR)/100;

	if(threshold < BEATDETECTOR_MIN_THRESHOLD)
	{
		threshold = BEATDETECTOR_MIN_THRESHOLD;
	}

}
