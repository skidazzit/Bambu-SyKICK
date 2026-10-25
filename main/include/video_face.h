#pragma once
#include <lvgl.h>
#include <string>

// Animated geometry inspired by the approved oval-eye preview.
// No video decoding or full-screen canvas allocation.
class VideoFace {
public:
    explicit VideoFace(lv_obj_t* parent);
    ~VideoFace();
    void SetEmotion(const char* emotion);
    void SetVisible(bool visible);
    void Tickle();
    void Kiss();
    void Dizzy();
    void SetColor(uint32_t color, bool match_accents = false);

private:
    lv_obj_t* root_;
    lv_timer_t* timer_;
    std::string emotion_ = "neutral";
    uint32_t started_ = 0, tickled_at_ = 0;
    bool tickling_ = false;
    int reaction_ = 0;
    uint32_t reaction_at_ = 0;
    uint32_t color_ = 0xffde36;
    uint32_t accent_ = 0xff72b6;
    void Draw(lv_event_t* event);
};
