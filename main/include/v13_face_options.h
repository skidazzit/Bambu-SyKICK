#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

class FaceOptions {
public:
    static constexpr int SadSleepy = 6;
    static constexpr uint32_t IdleFlipMs = 30000;
    enum Mode { Randomize, Mood, Single };
    enum PrinterMood { Standby, Printing, Paused, Failed, Complete };
    inline static constexpr std::array<const char*, 6> Names = {"neutral", "sad",    "happy",
                                                                "glossy",  "sleepy", "surprised"};
    inline static constexpr std::array<int, 7> Delays = {10, 30, 60, 120, 300, 600, 0};
    int mode = Single, main = 0, idle = 4, delay = 2;
    bool IsIdle() const { return was_idle_; }
    void Clamp() {
        mode = std::clamp(mode, 0, 2);
        main = std::clamp(main, 0, 5);
        idle = std::clamp(idle, 0, SadSleepy);
        delay = std::clamp(delay, 0, 6);
    }
    void Activity(uint32_t now) {
        last_input_ = now;
        last_random_ = now;
        was_idle_ = false;
    }
    int Select(uint32_t now, bool home, PrinterMood mood, uint32_t random) {
        Clamp();
        if (!home) {
            Activity(now);
            return current_;
        }
        if (Delays[delay] && now - last_input_ >= uint32_t(Delays[delay]) * 1000) {
            was_idle_ = true;
            if (idle == SadSleepy) {
                const uint32_t elapsed = now - last_input_ - uint32_t(Delays[delay]) * 1000;
                return (elapsed / IdleFlipMs) % 2 == 0 ? 1 : 4;
            }
            return idle;
        }
        if (was_idle_)
            Activity(now);
        if (mode == Single)
            current_ = main;
        else if (mode == Mood) {
            static constexpr int faces[] = {0, 3, 5, 1, 2};
            current_ = faces[std::clamp(int(mood), 0, 4)];
        } else if (now - last_random_ >= 30000) {
            // Pick another face without an immediate repeat.
            current_ = (current_ + 1 + random % 5) % 6;
            last_random_ = now;
        }
        return current_;
    }

private:
    uint32_t last_input_ = 0, last_random_ = 0;
    int current_ = 0;
    bool was_idle_ = false;
};
