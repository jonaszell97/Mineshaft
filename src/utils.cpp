//
// Created by Jonas Zell on 2019-01-19.
//

#include "mineshaft/utils.h"

#include <llvm/Support/FileSystem.h>
#include <chrono>
#include <ctime>

namespace mc {

float float_rand(float min, float max)
{
   float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
   return min + scale * (max - min);      /* [min, max] */
}

glm::vec4 randomColor()
{
   return { float_rand(0.0f, 1.0f),
      float_rand(0.0f, 1.0f),
      float_rand(0.0f, 1.0f),
       1.0f, };
}

llvm::StringRef getPath(llvm::StringRef fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == std::string::npos
      || (period < slash && slash != std::string::npos)) {
      return fullPath;
   }

   if (slash == std::string::npos) {
      return "";
   }

   return fullPath.substr(0, slash + 1);
}

llvm::StringRef getFileNameAndExtension(llvm::StringRef fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == std::string::npos)
      return "";

   if (slash == std::string::npos)
      return fullPath;

   if (period > slash)
      return fullPath.substr(slash + 1);

   return "";
}

bool fileExists(llvm::StringRef name)
{
   return llvm::sys::fs::is_regular_file(name);
}

std::string findFileInDirectories(llvm::StringRef fileName,
                                  llvm::ArrayRef<std::string> directories) {
   if (fileName.front() == PathSeperator) {
      if (fileExists(fileName))
         return fileName;

      return "";
   }

   auto Path = getPath(fileName);
   if (!Path.empty()) {
      fileName = getFileNameAndExtension(fileName);
   }

   using iterator = llvm::sys::fs::directory_iterator;
   using Kind = llvm::sys::fs::file_type;

   std::error_code ec;
   iterator end_it;

   for (std::string dirName : directories) {
      if (!Path.empty()) {
         if (dirName.back() != PathSeperator) {
            dirName += PathSeperator;
         }

         dirName += Path;
      }

      iterator it(dirName, ec);
      while (it != end_it) {
         auto &entry = *it;

         auto errOrStatus = entry.status();
         if (!errOrStatus)
            break;

         auto &st = errOrStatus.get();
         switch (st.type()) {
         case Kind::regular_file:
         case Kind::symlink_file:
         case Kind::character_file:
            if (getFileNameAndExtension(entry.path()) == fileName.str())
               return entry.path();

            break;
         default:
            break;
         }

         it.increment(ec);
      }

      ec.clear();
   }

   return "";
}

Timer::Timer(llvm::StringRef taskName, llvm::raw_ostream *OS)
   : taskName(taskName), beginTime(getTime()), OS(OS ? *OS : llvm::errs())
{

}

Timer::~Timer()
{
   long long endTime = getTime();
   long long timeElapsed = endTime - beginTime;

   OS << "Finished task '" << taskName << "' after " << (timeElapsed / 1000) << "ms.\n";
}

long long Timer::getTime() const
{
   return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

} // namespace mc