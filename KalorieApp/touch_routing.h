#pragma once
#include "app_model.h"
#include "touch_indstillinger.h"

// Menuen faar en handling pr. swipe. Et talfelt maa stadig modtage mange trin.
// Derved aabner et swipe paa forsiden menuen uden samtidig at rulle gennem alle raekker.
class TouchRouting {
 public:
  void handle(const TouchEvent& event, KalorieAppModel& app, uint32_t now) {
    if (blocked && event.gesture == blockedGesture) return;
    bool swipe = event.handling == Input::SwipeOp || event.handling == Input::SwipeNed;
    bool redigererTal = (app.side == Side::Ugemaal || app.side == Side::Lysstyrke)
                       && app.fokus == Fokus::Vaerdi;
    app.input(event.handling, now);
    if (TouchConfig::MENU_ET_TRIN_PR_SWIPE && swipe && !redigererTal) {
      blocked = true; blockedGesture = event.gesture;
    }
  }
 private:
  bool blocked = false;
  uint32_t blockedGesture = 0;
};
