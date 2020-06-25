#ifndef MINESHAFT_OBJPARSER_H
#define MINESHAFT_OBJPARSER_H

#include "Mesh.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>

llvm::Optional<Mesh> parseMesh(llvm::StringRef ObjFile);

#endif //MINESHAFT_OBJPARSER_H
