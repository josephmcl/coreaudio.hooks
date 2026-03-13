#include "coreaudio.h"
#include <iostream>

#include <deque>

#include <chrono>

// #include "envelope.h"

double y(double t) {
  return ((16 * cos(t/2)) - cos(t*2)) / 15;
}

double old = 440.;   /* we're all shackled to 440 hz */
double mew = 440.;
double pitch() { return old * 2*3.14159; }  
/*
int main() {
  int16_t v;
  char* p = (char*) (&v);  // "buffer" for standard output
  double t=0;

  while (t+=1./44100) {
    v = (int16_t) ( y(t*pitch) * (2<<12) );
    putchar(p[0]);putchar(p[1]);  // disclaimer: this is NOT good programming
    putchar(p[0]);putchar(p[1]);  // style, just a quick hack to output data
  }
  return 0;
}
*/

using double_seconds = std::chrono::duration<double>;

class adsr_envelope {
public:

    double pitch = 440.;

    std::chrono::microseconds 
        attack_duration  = std::chrono::microseconds(125000),
        decay_duration   = std::chrono::microseconds(125000),
        sustain_duration = std::chrono::microseconds(125000),
        release_duration = std::chrono::microseconds(125000);

    double_seconds sample_rate = double_seconds(2.0 * M_PI * 440 / 44100);

    double_seconds
        class_time = double_seconds(0.);

    double ground_amplitude  = 0.0F;
    double attack_amplitude  = 0.30F;
    double sustain_amplitude = 0.25F;

    auto total_duration() -> std::chrono::microseconds {
        return attack_duration + decay_duration + sustain_duration + release_duration;
    }

    OSStatus
    sample(void *, AudioUnitRenderActionFlags *, const AudioTimeStamp *,
        UInt32, UInt32 inNumberFrames, AudioBufferList *ioData) {

        
        // This is a mono tone generator so we only need the first buffer
        Float32 *buffer = (Float32 *) ioData->mBuffers[0].mData;

        // Generate the samples
        double_seconds delta = class_time;
        for (UInt32 frame = 0; frame < inNumberFrames; ++frame) {
            buffer[frame] = y(delta.count() * pitch * 2 * 3.14159);

            delta += sample_rate;
            if (delta > double_seconds(2.0 * M_PI)) {
                delta -= double_seconds(2.0 * M_PI);
            }
        }
        return noErr;
    }

    auto amp_at(double dur) -> double { 
        double x;
        double x0, x1, y0, y1; 
        if (dur < 0.025) {
            y0 = ground_amplitude;
            y1 = attack_amplitude;
            x0 = 0.;
            x1 = 0.025;
            x = dur;
            std::cout << "A" << std::endl;
        }
        else if (dur < 0.25) {
            y0 = attack_amplitude;
            y1 = sustain_amplitude;
            x0 = 0.025;
            x1 = 0.25;
            x = dur;
            std::cout << "B" << std::endl;
        }
        else if (dur < 0.45) {
            y0 = sustain_amplitude;
            y1 = sustain_amplitude;
            x0 = 0.25;
            x1 = 0.45;
            x = dur;
            std::cout << "C" << std::endl;
        }
        else  { // (dur >= 0.375)
            y0 = sustain_amplitude;
            y1 = ground_amplitude;
            x0 = 0.45;
            x1 = 0.5;
            x = dur;
            std::cout << "D" << std::endl;
        }

        auto left = y0 * (x1 - x);
        auto right = y1 * (x - x0);
        auto interp = (left + right) / (x1 - x0);
        std::cout << "> " << dur << " || amp at " << x << " is " << interp << std::endl;
        return interp;
    }

};

std::deque<double> a_major;

void change_pitch() {
    old = a_major.front();
    a_major.pop_front();
}

static double gtheta = 0.;
static double xtheta = 0.;
static double rtheta = 0.;

static adsr_envelope env = {};

OSStatus sample_(void *inRef, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    // Fixed amplitude is good enough for our purposes
    const double amplitude = 0.25;

    // Get the tone parameters out of the static var
    // could be stored in inRef
    double theta = gtheta;
    double theta_increment = 2.0 * M_PI * 440 / 44100;
    theta_increment = 1.0 / 44100;

    // This is a mono tone generator so we only need the first buffer
    const int channel = 0;
    Float32 *buffer = (Float32 *)ioData->mBuffers[channel].mData;

    // Generate the samples
    for (UInt32 frame = 0; frame < inNumberFrames; frame++) 
    {
        //buffer[frame] = (sin(theta * 2) * amplitude) + (sin(theta / 2) * amplitude);
        buffer[frame] = y(theta * pitch());

        theta += theta_increment;
        if (theta > 2.0 * M_PI)
        {
            theta -= 2.0 * M_PI;
        }
    }
    std::cout << theta << " " << old << std::endl;
    gtheta = theta;

    if (gtheta > 0.5 + xtheta) {
        xtheta = gtheta;
        change_pitch();
    }
    //if (mew > old) {
    //    old += 5.;
    //}

    return noErr;
}

OSStatus sample(void *inRef, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    // Fixed amplitude is good enough for our purposes
    const double amplitude = 0.25;

    // Get the tone parameters out of the static var
    // could be stored in inRef
    double theta = gtheta;
    double etheta = rtheta;
    double theta_increment = 2.0 * M_PI * 440 / 44100;
    theta_increment = 1.0 / 44100;

    // This is a mono tone generator so we only need the first buffer
    const int channel = 0;
    Float32 *buffer = (Float32 *)ioData->mBuffers[channel].mData;

    // Generate the samples
    for (UInt32 frame = 0; frame < inNumberFrames; frame++) 
    {
        //buffer[frame] = (sin(theta * 2) * amplitude) + (sin(theta / 2) * amplitude);
        buffer[frame] = y(theta * pitch()) * env.amp_at(etheta);

        theta += theta_increment;
        etheta += theta_increment;
        if (theta > 2.0 * M_PI)
        {
            theta -= 2.0 * M_PI;
        }
    }
    std::cout << theta << " " << old << std::endl;
    gtheta = theta;
    rtheta = etheta;

    if (gtheta > 0.501 + xtheta) {
        xtheta = gtheta;
        change_pitch();
        rtheta = 0.;
    }
    //if (mew > old) {
    //    old += 5.;
    //}

    return noErr;
}

int main() {

    
    a_major.push_back(440.);
    a_major.push_back(493.88);
    a_major.push_back(554.37);
    a_major.push_back(587.33);
    a_major.push_back(659.25);
    a_major.push_back(739.99);
    a_major.push_back(830.61);    
    a_major.push_back(880.);

    coreaudio::definition d;
    d.asbd_flags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    d.sample_rate = 44100;
    d.sample_bits = 32;
    d.sample_bytes = sizeof(float);

    
    auto example = [&](void *, AudioUnitRenderActionFlags *, 
        const AudioTimeStamp *, UInt32, UInt32 a, 
        AudioBufferList *b) -> OSStatus {

        return OSStatus{};
        //return env.sample(nullptr, nullptr, nullptr, 0, a, b);
    };

    //  d.callback(example);
    

    d.callback(sample);

    {

        auto audio = coreaudio::instance::make(d);

        if (audio == std::nullopt) {
            std::cout << "Error" << std::endl;
        }

        audio->volume(100);

        usleep(4530000);
    }
    

    /*
    static constexpr envelope::frame f1(1., std::chrono::seconds(1));
    static constexpr envelope::frame f2(1., std::chrono::seconds(1));
    static constexpr envelope::frame f3(1., std::chrono::seconds(1));

    static constexpr envelope::duration_t t = std::chrono::seconds(1);

    envelope::test<f1, f2>();
    */

    return 0;
}