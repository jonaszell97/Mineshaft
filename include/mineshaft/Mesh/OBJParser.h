//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_OBJPARSER_H
#define MINEKAMPF_OBJPARSER_H

#include "Mesh.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>

llvm::Optional<Mesh> parseMesh(llvm::StringRef ObjFile);

#endif //MINEKAMPF_OBJPARSER_H
