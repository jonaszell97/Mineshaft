#include "mineshaft/Event/Event.h"

#include <cassert>

using namespace mc;

const Event::KeyPressedData& Event::getKeyboardEventData() const
{
   assert(eventKind == KeyPressed || eventKind == KeyReleased);
   return keyboardData;
}

const Event::MouseBottonPressedData& Event::getMouseButtonEventData() const
{
   assert(eventKind == MouseButtonPressed || eventKind == MouseButtonReleased);
   return mouseButtonData;
}