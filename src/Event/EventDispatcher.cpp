#include "mineshaft/Event/EventDispatcher.h"

using namespace mc;

EventDispatcher::EventDispatcher(Application &Ctx) : Ctx(Ctx)
{

}

void EventDispatcher::keyPressed(int pressedKey, bool rescheduled)
{
   Event e;
   e.eventKind = Event::KeyPressed;
   e.keyboardData.pressedKey = pressedKey;
   e.keyboardData.rescheduled = rescheduled;

   scheduledEvents.push(e);
}

void EventDispatcher::keyReleased(int pressedKey)
{
   Event e;
   e.eventKind = Event::KeyReleased;
   e.keyboardData.pressedKey = pressedKey;
   e.keyboardData.rescheduled = false;

   scheduledEvents.push(e);
}

void EventDispatcher::mouseButtonPressed(int pressedKey, bool rescheduled)
{
   Event e;
   e.eventKind = Event::MouseButtonPressed;
   e.mouseButtonData.pressedButton = pressedKey;
   e.mouseButtonData.rescheduled = rescheduled;

   scheduledEvents.push(e);
}

void EventDispatcher::mouseButtonReleased(int pressedKey)
{
   Event e;
   e.eventKind = Event::MouseButtonReleased;
   e.mouseButtonData.pressedButton = pressedKey;
   e.mouseButtonData.rescheduled = false;

   scheduledEvents.push(e);
}

void EventDispatcher::registerEventHandler(Event::EventKind eventKind,
                                           EventHandler handler) {
   eventHandlers[eventKind].push_back(handler);
}

void EventDispatcher::dispatchEvents()
{
   auto scheduledEvents = std::move(this->scheduledEvents);
   assert(this->scheduledEvents.empty());

   while (!scheduledEvents.empty()) {
      Event next = scheduledEvents.front();
      scheduledEvents.pop();

      for (auto *handler : eventHandlers[next.getKind()]) {
         handler(*this, next);
      }
   }
}