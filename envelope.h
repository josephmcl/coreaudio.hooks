#pragma once 
#include <chrono>
#include <tuple>
#include <variant>
#include <functional>

namespace envelope { 

    using duration_t = std::chrono::duration<long long>;
    struct frame {
        constexpr frame(double decibel, 
                        duration_t duration, 
                        bool fixed=false)
            : decibel(decibel)
            , duration(duration)
            , fixed(fixed) {}
        double const decibel;
        duration_t const duration;
        bool const fixed;
    };

    using envelope_function = std::function<double(const duration_t &)>;

    template<frame &...f> 
    auto envelope(duration_t &duration) -> envelope_function {
        // return [] (const duration_t &d) { return 0.; };

        return [] (const duration_t& d) { return 0.; };
    }

    template<frame ...f> 
    struct test {

    };
}

/*  
template<frame ... frames> struct envelope {

};

*/