#pragma once 
#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <optional>
#include <memory>
#include <system_error>
#include <cstdlib>
#include <functional>

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

namespace coreaudio {

    using AC    = AudioComponent;
    using ACI   = AudioComponentInstance;
    using ACD   = AudioComponentDescription;
    using ASBD  = AudioStreamBasicDescription;
    using AURCS = AURenderCallbackStruct;
    using AURC  = AURenderCallback;
    using AURAF = AudioUnitRenderActionFlags;
    using ATS   = AudioTimeStamp;
    using ABL   = AudioBufferList;

    // using noerr = noErr;

    using callback = std::function<OSStatus(
        void      *in_ref, 
        AURAF     *io_flags, 
        const ATS *in_timestamp, 
        UInt32     in_bus_number, 
        UInt32     in_frames_count, 
        ATS       *io_data)>;

    struct shared_callback {

        // push()

        OSStatus callback(
            void *in_ref, 
            AudioUnitRenderActionFlags *io_flags, 
            const AudioTimeStamp *in_timestamp, 
            UInt32 in_bus_number, 
            UInt32 in_frames_count, 
            AudioBufferList *io_data);

    };  

    struct shared_aci {
        ACI aci;
        shared_aci(void) {
        }
        ~shared_aci(void) {
            AudioOutputUnitStop(aci);
            AudioComponentInstanceDispose(aci);
        }
        auto get(void) -> ACI & { return aci; }
    };

    struct definition {

        int channels      = 1;
        int asbd_flags    = 0;
        int format_id     = kAudioFormatLinearPCM;
        int packet_frames = 1;
        int sample_rate   = 0;
        int sample_bits   = 0;
        int sample_bytes  = 0;

        AURCS aurcs = {
            nullptr,
            nullptr
        };  

        auto callback(AURC aurc) -> void;
        auto acd(void) -> ACD;
        auto asbd(void) -> ASBD;
    };

    class instance {
    public: 
        static 
        auto make(definition &d) -> std::optional<instance>;
        auto volume(int percent) -> int;
        auto pause(bool paused) -> bool;
        ~instance(void);
        int const max_db = 40;
        instance (const instance &that); 
    private:
        instance(void);
        std::shared_ptr<shared_aci> aci = nullptr;
    };

};