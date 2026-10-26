#include "video_face.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265f;
// Enlarge all face artwork by 10% while keeping its center fixed on the round screen.
constexpr float kFaceSizeScale = 1.10f;
constexpr float kPainterBaseScale = 1.16f;
struct Painter {
    lv_layer_t* layer;
    int x, y;
    lv_point_precise_t Point(float px, float py) {
        lv_point_precise_t p{};
        p.x = static_cast<int>(x + 233 + (px - 240) * kPainterBaseScale * kFaceSizeScale);
        p.y = static_cast<int>(y + 233 + (py - 265) * kPainterBaseScale * kFaceSizeScale);
        return p;
    }
    void Polygon(const lv_point_precise_t* points, int count, float, float, uint32_t color) {
        // Fill shared polygon edges as scan lines; antialiased triangle fans leave seams.
        int low = 10000, high = -10000;
        for (int i = 0; i < count; ++i) {
            low = std::min(low, static_cast<int>(points[i].y));
            high = std::max(high, static_cast<int>(points[i].y));
        }
        low = std::max(low, static_cast<int>(layer->_clip_area.y1));
        high = std::min(high, static_cast<int>(layer->_clip_area.y2));
        lv_draw_rect_dsc_t d;
        lv_draw_rect_dsc_init(&d);
        d.bg_color = lv_color_hex(color);
        for (int row = low; row <= high; ++row) {
            std::array<float, 48> crossings;
            int n = 0;
            float scan = row + .5f;
            for (int i = 0; i < count; ++i) {
                auto a = points[i], b = points[(i + 1) % count];
                if ((a.y <= scan && b.y > scan) || (b.y <= scan && a.y > scan))
                    crossings[n++] = a.x + (scan - a.y) * (b.x - a.x) / (b.y - a.y);
            }
            std::sort(crossings.begin(), crossings.begin() + n);
            for (int i = 0; i + 1 < n; i += 2) {
                lv_area_t rect{static_cast<int>(std::ceil(crossings[i])), row,
                               static_cast<int>(std::floor(crossings[i + 1])), row};
                if (rect.x1 <= rect.x2)
                    lv_draw_rect(layer, &d, &rect);
            }
        }
    }
    void Oval(float cx, float cy, float w, float h, uint32_t color) {
        std::array<lv_point_precise_t, 48> points;
        for (int i = 0; i < 48; ++i) {
            float a = i * 2 * Pi / 48;
            points[i] = Point(cx + w * .5f * std::cos(a), cy + h * .5f * std::sin(a));
        }
        Polygon(points.data(), points.size(), cx, cy, color);
    }
    void Line(float x1, float y1, float x2, float y2, int width, uint32_t color) {
        lv_draw_line_dsc_t d;
        lv_draw_line_dsc_init(&d);
        d.p1 = Point(x1, y1);
        d.p2 = Point(x2, y2);
        d.width = std::max(1, static_cast<int>(std::lround(width * kFaceSizeScale)));
        d.color = lv_color_hex(color);
        d.round_start = d.round_end = true;
        lv_draw_line(layer, &d);
    }
    void Curve(float x, float y, bool smile, uint32_t color) {
        for (int i = 0; i < 20; ++i) {
            float a = i / 20.f, b = (i + 1) / 20.f;
            Line(x - 43 + 86 * a, y + (smile ? 20 : -24) * std::sin(Pi * a), x - 43 + 86 * b,
                 y + (smile ? 20 : -24) * std::sin(Pi * b), smile ? 10 : 15, color);
        }
    }
    void Heart(float x, float y, float radius, uint32_t color) {
        std::array<lv_point_precise_t, 48> points;
        for (int i = 0; i < 48; ++i) {
            float a = i * 2 * Pi / 48;
            float s = std::sin(a);
            points[i] =
                Point(x + radius * s * s * s, y - radius *
                                                      (13 * std::cos(a) - 5 * std::cos(2 * a) -
                                                       2 * std::cos(3 * a) - std::cos(4 * a)) /
                                                      16);
        }
        Polygon(points.data(), points.size(), x, y, color);
    }
};
}  // namespace
VideoFace::VideoFace(lv_obj_t* parent) {
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_pos(root_, 0, 0);
    lv_obj_set_size(root_, 466, 466);
    lv_obj_remove_flag(root_,
                       static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_add_event_cb(
        root_, [](lv_event_t* e) { static_cast<VideoFace*>(lv_event_get_user_data(e))->Draw(e); },
        LV_EVENT_DRAW_MAIN, this);
    started_ = lv_tick_get();
    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            auto self = static_cast<VideoFace*>(lv_timer_get_user_data(timer));
            // Skip repainting while the Settings/game overlay covers the home face.
            if (lv_obj_is_visible(self->root_))
                lv_obj_invalidate(self->root_);
        },
        50, this);
}
VideoFace::~VideoFace() {
    lv_timer_delete(timer_);
    lv_obj_delete(root_);
}
void VideoFace::SetEmotion(const char* emotion) {
    tickling_ = false;
    reaction_ = 0;
    std::string next = emotion ? emotion : "neutral";
    if (next != emotion_) {
        emotion_ = next;
        started_ = lv_tick_get();
    }
    lv_obj_invalidate(root_);
}
void VideoFace::Tickle() {
    reaction_ = 0;
    tickled_at_ = lv_tick_get();
    tickling_ = true;
    lv_obj_invalidate(root_);
}
void VideoFace::Kiss() {
    tickling_ = false;
    reaction_ = 1;
    reaction_at_ = lv_tick_get();
    lv_obj_invalidate(root_);
}
void VideoFace::Dizzy() {
    tickling_ = false;
    reaction_ = 2;
    reaction_at_ = lv_tick_get();
    lv_obj_invalidate(root_);
}
void VideoFace::SetVisible(bool visible) {
    if (!visible) {
        tickling_ = false;
        reaction_ = 0;
    }
    if (visible)
        lv_obj_remove_flag(root_, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}
void VideoFace::SetColor(uint32_t color, bool match_accents) {
    color_ = color;
    accent_ = match_accents ? color : 0xff72b6;
    lv_obj_invalidate(root_);
}
void VideoFace::Draw(lv_event_t* event) {
    lv_area_t area;
    lv_obj_get_coords(root_, &area);
    Painter p{lv_event_get_layer(event), area.x1, area.y1};
    uint32_t elapsed = lv_tick_get() - started_;
    float t = elapsed / 1000.f;
    float bob = 3 * std::sin(t * 2 * Pi / 3);
    float phase = std::fmod(t, 3.f);
    float blink = std::fmax(.06f, std::fmin(1.f, std::fabs(phase - 1.65f) / .13f));
    bool happy = emotion_ == "happy" || emotion_ == "laughing" || emotion_ == "funny";
    bool love = emotion_ == "loving" || emotion_ == "kissy" || emotion_ == "winking";
    bool sleepy = emotion_ == "sleepy" || emotion_ == "relaxed";
    bool celebrate = emotion_ == "celebrate" || emotion_ == "excited" || emotion_ == "confident";
    bool curious = emotion_ == "curious" || emotion_ == "thinking" || emotion_ == "confused" ||
                   emotion_ == "silly" || emotion_ == "delicious";
    bool sad = emotion_ == "sad" || emotion_ == "crying";
    bool shocked = emotion_ == "shocked" || emotion_ == "surprised";
    if (tickling_ && lv_tick_get() - tickled_at_ >= 2200)
        tickling_ = false;
    if (reaction_ && lv_tick_get() - reaction_at_ >= 2200)
        reaction_ = 0;
    float reaction_time = (lv_tick_get() - reaction_at_) / 1000.f;
    if (reaction_ == 1) {
        p.Oval(161, 239, 100, 132, color_);
        p.Curve(319, 239, false, color_);
        for (int i = 0; i < 24; ++i) {
            float a = -Pi / 2 + i * Pi / 24;
            float c = -Pi / 2 + (i + 1) * Pi / 24;
            for (float y : {310.f, 331.f})
                p.Line(235 + 14 * std::cos(a), y + 12 * std::sin(a), 235 + 14 * std::cos(c),
                       y + 12 * std::sin(c), 7, color_);
        }
        float progress = std::min(1.f, reaction_time / 1.8f);
        p.Heart(268 + 74 * progress, 315 - 110 * progress, 12 + 14 * progress, accent_);
    } else if (reaction_ == 2) {
        for (float x : {161.f, 319.f}) {
            for (int i = 0; i < 70; ++i) {
                float a = i * .2f + reaction_time * 6;
                float c = (i + 1) * .2f + reaction_time * 6;
                p.Line(x + i * .58f * std::cos(a), 239 + i * .58f * std::sin(a),
                       x + (i + 1) * .58f * std::cos(c), 239 + (i + 1) * .58f * std::sin(c), 6,
                       color_);
            }
        }
        for (int i = 0; i < 30; ++i)
            p.Line(205 + i * 2.4f, 325 + 7 * std::sin(i * .45f + reaction_time * 8),
                   205 + (i + 1) * 2.4f, 325 + 7 * std::sin((i + 1) * .45f + reaction_time * 8), 6,
                   color_);
    } else if (tickling_) {
        float giggle = (lv_tick_get() - tickled_at_) / 1000.f;
        float bounce = 4 * std::sin(giggle * 8 * Pi);
        p.Curve(160, 207 + bounce, false, color_);
        p.Curve(320, 207 + bounce, false, color_);
        p.Oval(240, 288 + bounce, 110, 80, color_);
        p.Oval(240, 239 + bounce, 130, 50, 0);
    } else if (love) {
        p.Curve(151, 277 + bob, false, color_);
        p.Curve(329, 277 + bob, false, color_);
        p.Heart(240, 254, 57 * (1 + .08f * std::sin(t * 2 * Pi)), accent_);
        constexpr float positions[4][2] = {{150, 190}, {330, 180}, {170, 334}, {315, 329}};
        for (auto& pos : positions)
            p.Heart(pos[0], pos[1] - 8 * std::sin(t * 2 * Pi / 3), 18, accent_);
    } else if (celebrate) {
        p.Curve(156, 300 + bob, false, color_);
        p.Curve(324, 300 + bob, false, color_);
        p.Oval(240, 346 + bob, 50, 55, color_);
        constexpr float positions[3][2] = {{183, 179}, {242, 155}, {302, 188}};
        uint32_t colors[] = {0xffde36, accent_, color_};
        for (int i = 0; i < 3; ++i) {
            float x = positions[i][0], y = positions[i][1] + 8 * std::sin(t * 2 * Pi / 3 + i);
            p.Line(240, 271, x, y + 32, 2, colors[i]);
            p.Oval(x, y, 46, 64, colors[i]);
        }
    } else if (emotion_ == "look_around") {
        // Preview-approved five-second gaze: the eye facing the turn grows 35%.
        // The base eyes retain the approved 10% face-size increase.
        const float seconds = std::fmod(t, 5.2f);
        auto ease = [seconds](float start, float end) {
            const float progress = std::clamp((seconds - start) / (end - start), 0.f, 1.f);
            return progress * progress * (3.f - 2.f * progress);
        };
        const float left_focus = ease(0.42f, 0.88f) * (1.f - ease(1.92f, 2.18f));
        const float right_focus = ease(2.60f, 3.06f) * (1.f - ease(4.10f, 4.36f));
        const float look_offset = 22.f * (right_focus - left_focus);
        float eye_blink = 1.f;
        for (const float blink_center : {1.92f, 4.10f}) {
            const float distance = std::fabs(seconds - blink_center);
            if (distance < 0.13f)
                eye_blink = std::min(eye_blink, 0.12f + 0.88f * distance / 0.13f);
        }
        auto draw_eye = [&](float cx, float focus) {
            const float growth = 1.f + 0.35f * focus;
            const float width = 91.f * kFaceSizeScale * growth;
            const float height = 119.f * kFaceSizeScale * growth * eye_blink;
            const float center_x = 233.f + (cx - 233.f) * kFaceSizeScale + look_offset;
            const float center_y = 233.f + bob * kFaceSizeScale;
            lv_draw_rect_dsc_t d;
            lv_draw_rect_dsc_init(&d);
            d.bg_color = lv_color_hex(color_);
            d.radius = static_cast<int>(std::min(width, height) / 2.f);
            lv_area_t rect{
                static_cast<int>(area.x1 + center_x - width / 2.f),
                static_cast<int>(area.y1 + center_y - height / 2.f),
                static_cast<int>(area.x1 + center_x + width / 2.f - 1.f),
                static_cast<int>(area.y1 + center_y + height / 2.f - 1.f)};
            lv_draw_rect(lv_event_get_layer(event), &d, &rect);
        };
        draw_eye(163.f, left_focus);
        draw_eye(303.f, right_focus);
    } else {
        // Orbit's original eyes, centered in a 392 x 224 band on the 466px screen.
        // The existing LVGL display rotation applies to menus and faces together.
        const float ox = 44.f, oy = 110.f;
        float w = 91, h = 119;
        if (sad) {
            w = 105;
            h = 66;
        }
        if (happy) {
            w = 104;
            h = 77;
        }
        if (sleepy)
            h = 23;
        if (shocked) {
            w = 105;
            h = 142;
        }
        bool glossy = emotion_ == "glossy" || curious;
        if (glossy) {
            w = 108;
            h = 112;
        }
        w *= kFaceSizeScale;
        h *= kFaceSizeScale;
        float gaze = 12 * kFaceSizeScale * std::sin(t * .65f);
        auto ovalEye = [&](float cx) {
            cx = 233.f + (cx - 233.f) * kFaceSizeScale + gaze;
            float cy = 233 + bob * kFaceSizeScale;
            // Rounded rectangle preserves Orbit's broad capsules rather than ovals.
            auto roundRect = [&](float x, float y, float width, float height, uint32_t color) {
                lv_draw_rect_dsc_t d;
                lv_draw_rect_dsc_init(&d);
                d.bg_color = lv_color_hex(color);
                d.radius = static_cast<int>(std::min(width, height) / 2);
                lv_area_t rect{static_cast<int>(area.x1 + ox + x),
                               static_cast<int>(area.y1 + oy + y),
                               static_cast<int>(area.x1 + ox + x + width - 1),
                               static_cast<int>(area.y1 + oy + y + height - 1)};
                lv_draw_rect(lv_event_get_layer(event), &d, &rect);
            };
            float height = h * blink;
            float x = cx - w / 2, y = cy - height / 2;
            // Logical coordinates below map directly into Orbit's original eye band.
            x -= 44;
            y -= 110;
            roundRect(x, y, w, height, color_);
            if (height < 12)
                return;
            if (happy) {
                // Original broad closed Happy eyes, with no mouth or cheeks.
                // Mask the lower capsule using the same centered circular cutout.
                roundRect(x - 16, y + height * .30f, w + 32, w + 32, 0);
            } else if (sad) {
                std::array<lv_point_precise_t, 3> cut;
                float left = area.x1 + ox + x, top = area.y1 + oy + y;
                cut[0] = {static_cast<int>(left - 1), static_cast<int>(top - 1)};
                cut[1] = {static_cast<int>(left + w), static_cast<int>(top - 1)};
                cut[2] = {static_cast<int>(cx < 233 ? left - 1 : left + w),
                          static_cast<int>(top + height / 2)};
                p.Polygon(cut.data(), 3, 0, 0, 0);
            } else if (glossy) {
                roundRect(x + 5, y + 5, w - 10, height - 10, 0x686c68);
                // Highlights use direct circles in the same coordinate system.
                roundRect(x + w * .15f, y + height * .1f, w * .4f, w * .4f, 0xd8ffff);
                roundRect(x + w * .6f, y + height * .6f, w * .2f, w * .2f, 0xffffff);
            }
        };
        ovalEye(163);
        ovalEye(303);
    }
}
