#pragma once 
#include "coreaudio.h"

namespace waveform {

    struct instance {
        coreaudio::callback callback = [](
            void                 *in_ref, 
            coreaudio::AURAF     *io_flags, 
            const coreaudio::ATS *in_timestamp, 
            UInt32                in_bus_number, 
            UInt32                in_frames_count, 
            coreaudio::ATS       *io_data){


            return coreaudio::noerr;
        }
    };
};

