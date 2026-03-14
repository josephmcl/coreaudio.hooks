const lib = "./libjulia_audio.dylib"
const AudioHandle = Ptr{Cvoid}

function create_audio(sample_rate=44_100, capacity=16_384)
    h = ccall((:jlca_create, lib), AudioHandle, (Cint, Cint), sample_rate, capacity)
    h == C_NULL && error("failed to create audio handle")

    rc = ccall((:jlca_start, lib), Cint, (AudioHandle,), h)
    rc != 0 && error("failed to start audio")

    return h
end

function destroy_audio(h)
    ccall((:jlca_stop, lib), Cvoid, (AudioHandle,), h)
    ccall((:jlca_destroy, lib), Cvoid, (AudioHandle,), h)
end

function set_volume(h, percent)
    ccall((:jlca_set_volume, lib), Cint, (AudioHandle, Cint), h, percent)
end

function begin_write(h)
    ptr = ccall((:jlca_begin_write, lib), Ptr{Cfloat}, (AudioHandle,), h)
    n = ccall((:jlca_begin_write_frames, lib), Cint, (AudioHandle,), h)
    return ptr, Int(n)
end

function end_write(h, n)
    ccall((:jlca_end_write, lib), Cvoid, (AudioHandle, Cint), h, n)
end

function queued_frames(h)
    ccall((:jlca_available_read, lib), Cint, (AudioHandle,), h)
end

function wait_until_drained(h)
    while queued_frames(h) > 0
        sleep(0.01)
    end
    sleep(0.15)
end

function tri_pluck!(u, pluck_pos=0.22, amp=0.7)
    n = length(u)
    peak = clamp(round(Int, pluck_pos * (n - 1)) + 1, 2, n - 1)

    @inbounds for i in 1:peak
        u[i] = amp * (i - 1) / (peak - 1)
    end
    @inbounds for i in peak:n
        u[i] = amp * (n - i) / (n - peak)
    end

    u[1] = 0.0
    u[end] = 0.0
    return u
end

function gaussian_pluck!(u, center=0.22, width=0.05, amp=0.8)
    n = length(u)

    @inbounds for i in eachindex(u)
        x = (i - 1) / (n - 1)
        u[i] = amp * exp(-((x - center)^2) / (2 * width^2))
    end

    u[1] = 0.0
    u[end] = 0.0
    return u
end

function wave_note(freq; seconds=0.45, sample_rate=44_100,
                   nx=96, pickup_pos=0.8, pluck_pos=0.,
                   amp=0.25f0, damping=0.99992)

    nsteps = round(Int, seconds * sample_rate)

    # string on x in [0, 1]
    dx = 1.0 / (nx - 1)
    dt = 1.0 / sample_rate

    # choose c so that the fundamental is about freq:
    # fixed-fixed string fundamental is c / 2
    c = 2.0 * freq

    λ = c * dt / dx
    λ >= 1 && error("unstable discretization: need c*dt/dx < 1, got $λ")

    u_nm1 = zeros(Float64, nx)
    u_n   = zeros(Float64, nx)
    u_np1 = zeros(Float64, nx)

    tri_pluck!(u_n, pluck_pos, 0.8)
    # gaussian_pluck!(u_n, pluck_pos, 0.1, 0.9)
    copyto!(u_nm1, u_n)   # zero initial velocity

    pickup = clamp(round(Int, pickup_pos * (nx - 1)) + 1, 2, nx - 1)
    out = Vector{Float32}(undef, nsteps)

    λ2 = λ^2

    @inbounds for k in 1:nsteps
        for i in 2:nx-1
            u_np1[i] = 2u_n[i] - u_nm1[i] +
                       λ2 * (u_n[i+1] - 2u_n[i] + u_n[i-1])
        end

        u_np1[1] = 0.0
        u_np1[end] = 0.0

        for i in 2:nx-1
            u_np1[i] *= damping
        end

        y = u_np1[pickup]

        # very light soft clipping to keep levels sane
        y = tanh(1.5 * y)

        out[k] = Float32(amp * y)

        u_nm1, u_n, u_np1 = u_n, u_np1, u_nm1
    end

    return out
end

function write_samples!(h, samples)
    i = 1
    n = length(samples)

    while i <= n
        ptr, space = begin_write(h)

        if ptr == C_NULL || space == 0
            sleep(0.002)
            continue
        end

        m = min(space, n - i + 1)
        buffer = unsafe_wrap(Vector{Float32}, ptr, m; own=false)

        @inbounds copyto!(buffer, 1, samples, i, m)
        end_write(h, m)

        i += m
    end
end

function play_major_scale_fd()
    sample_rate = 44_100
    h = create_audio(sample_rate)
    set_volume(h, 100)

    # A major: A B C# D E F# G# A
    notes = [
        493.88,

        493.88,
        554.37,
        622.30,

        415.30,

        493.88,
        554.37,
        622.30,

        329.60,

        493.88,
        554.37,
        622.30,

        493.88
    ]

    beats = [
        2.5,

        0.5,
        1,
        2,

        2.5,
        
        0.5,
        1,
        2,
        
        2.5,

        0.5,
        1,
        2,   
        
        2.5
    ]

    

    #    440.00,
    #    493.88,
    #    554.37,
    #    587.33,
    #    659.25,
    #    739.99,
    #    830.61,
    #    880.00,

    try
        for (f,b) in zip(notes, beats)
            samples = wave_note(f;
                seconds=0.4 * b,
                sample_rate=sample_rate,
                nx=24,
                pickup_pos=0.78,
                pluck_pos=0.18,
                amp=0.22f0,
                damping=0.99993,
            )
            write_samples!(h, samples)
        end

        wait_until_drained(h)
    finally
        destroy_audio(h)
    end
end

play_major_scale_fd()