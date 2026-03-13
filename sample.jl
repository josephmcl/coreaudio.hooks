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
    n   = ccall((:jlca_begin_write_frames, lib), Cint, (AudioHandle,), h)
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
    sleep(0.15)  # let Core Audio finish whatever it already pulled
end

function fill_sine!(buffer, freq, sample_rate, phase; amp=0.25f0)
    dphase = 2π * freq / sample_rate

    @inbounds for i in eachindex(buffer)
        buffer[i] = amp * sin(phase)
        phase += dphase
        if phase ≥ 2π
            phase -= 2π
        end
    end

    return phase
end

function write_tone!(h, freq; seconds=0.45, sample_rate=44_100, amp=0.25f0, phase=0.0)
    frames_left = round(Int, seconds * sample_rate)

    while frames_left > 0
        ptr, space = begin_write(h)

        if ptr == C_NULL || space == 0
            sleep(0.002)
            continue
        end

        n = min(frames_left, space)
        buffer = unsafe_wrap(Vector{Float32}, ptr, n; own=false)

        phase = fill_sine!(buffer, freq, sample_rate, phase; amp=amp)
        end_write(h, n)

        frames_left -= n
    end

    return phase
end

function play_major_scale()
    sample_rate = 44_100
    h = create_audio(sample_rate)
    set_volume(h, 100)

    # A major: A B C# D E F# G# A
    notes = [
        440.00,
        493.88,
        554.37,
        587.33,
        659.25,
        739.99,
        830.61,
        880.00,
    ]

    phase = 0.0

    try
        for f in notes
            phase = write_tone!(h, f; seconds=0.1, sample_rate=sample_rate, amp=0.25f0, phase=phase)
        end

        wait_until_drained(h)
    finally
        destroy_audio(h)
    end
end

play_major_scale()