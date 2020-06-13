
#include "mineshaft/Application.h"

using namespace mc;

int main()
{
   Application Ctx;
   if (Ctx.initialize()) {
      return -1;
   }

   return Ctx.runGameLoop();
}