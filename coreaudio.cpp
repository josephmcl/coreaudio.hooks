
#include "coreaudio.h"

auto coreaudio::
definition::callback(AURC aurc) -> void {
    aurcs.inputProc = aurc;
    return;
}

auto coreaudio::
definition::acd(void) -> coreaudio::ACD {
    ACD acd;
    acd.componentFlags        = 0;
    acd.componentFlagsMask    = 0;
    acd.componentManufacturer = kAudioUnitManufacturer_Apple;
    acd.componentSubType      = kAudioUnitSubType_DefaultOutput;
    acd.componentType         = kAudioUnitType_Output;
    return acd;
}

auto coreaudio::
definition::asbd(void) -> coreaudio::ASBD {
    ASBD asbd;
    asbd.mSampleRate       = sample_rate;
    asbd.mFormatID         = kAudioFormatLinearPCM;
    asbd.mFormatFlags      = asbd_flags;
    asbd.mFramesPerPacket  = packet_frames;
    asbd.mChannelsPerFrame = channels;
    asbd.mBitsPerChannel   = sample_bits;
    asbd.mBytesPerPacket   = channels * sample_bytes;
    asbd.mBytesPerFrame    = channels * sample_bytes;
    return asbd;
}

auto coreaudio::
instance::make(definition &d) -> std::optional<instance> {

    auto acd = d.acd();

    AC ac;

    if (!(ac = AudioComponentFindNext(nullptr, &acd))) {
        fprintf(stderr, "Error: AudioComponentFindNext().\n");
        return std::nullopt;
    }

    instance i{};

    ACI &aci = i.aci->get();

    if (AudioComponentInstanceNew(ac, &aci)) {
        fprintf(stderr, "Error: AudioComponentInstanceNew().\n");
        return std::nullopt;
    }

    if (AudioUnitInitialize(aci)) {
        fprintf(stderr, "Error: AudioUnitInitialize().\n");
        return std::nullopt;
    }

    ASBD asbd = d.asbd();

    if (AudioUnitSetProperty(
            aci, 
            kAudioUnitProperty_StreamFormat, 
            kAudioUnitScope_Input, 
            0, 
            &asbd, 
            sizeof(ASBD))) {
        fprintf (stderr, "Failed to set audio unit input property.\n");
        return std::nullopt;
    }

    if (AudioUnitSetProperty(
            aci, 
            kAudioUnitProperty_SetRenderCallback, 
            kAudioUnitScope_Input, 
            0, 
            &d.aurcs, 
            sizeof(AURCS))) {
        fprintf (stderr, "Unable to attach an IOProc to the selected audio unit.\n");
        return std::nullopt;
    }

    if (AudioOutputUnitStart(aci)) {
        printf ("Unable to start audio unit.\n");
        return std::nullopt;
    }

    return i;
}

coreaudio::instance::instance(const instance &that) : aci(that.aci) { 

}


coreaudio::instance::instance() : aci(new shared_aci{}) { 
}

coreaudio::instance::~instance(void) { }

auto coreaudio::
instance::volume (int percent) -> int {
    float factor = (percent == 0) ? 0.0 : powf (10, (float) (40) * (percent - 100) / 100 / 20);

    /* lots of pain concerning controlling application volume can be avoided
     * with this one neat trick... */
        AudioUnitSetParameter(aci->get(), kHALOutputParam_Volume, kAudioUnitScope_Global, 0, factor, 0);
    return percent;
}

