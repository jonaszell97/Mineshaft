#include "OBJParser.h"
#include "utils.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SaveAndRestore.h>

#include <unordered_map>

namespace {

class OBJParser {
   /// Memory buffer of the object file.
   llvm::MemoryBuffer *Buf;

   /// The path to the file.
   llvm::StringRef Path;

   /// Current buffer pointer.
   const char *CurPtr;

   /// Buffer end pointer.
   const char *EndPtr;

   /// Map of parsed materials.
   llvm::StringMap<Material> Materials;

   /// Map of parsed materials textures.
   llvm::StringMap<llvm::StringRef> MaterialTextures;

   /// Return the next 'word' in the object file.
   llvm::StringRef nextWord();

   /// Skip a potential comment.
   void skipComment();

   /// Skip the current line.
   void skipLine();

   /// Advance to the next character.
   /// \return The number of skipped characters.
   unsigned advanceChar();

   // Parse a vertex description.
   void parseVertex(llvm::StringRef Str,
                    std::vector<unsigned> &Indices);

   // Create a mesh in the format needed by OpenGL.
   void createMesh(const std::vector<unsigned> &Indices,
                   const std::vector<glm::vec3> &Coordinates,
                   const std::vector<glm::vec2> &Textures,
                   const std::vector<glm::vec3> &Normals,
                   Mesh &M);

   // Create a mesh in the format needed by OpenGL.
   void createMeshSimple(const std::vector<unsigned> &Indices,
                         const std::vector<glm::vec3> &Coordinates,
                         const std::vector<glm::vec2> &Textures,
                         const std::vector<glm::vec3> &Normals,
                         Mesh &M);

   // Parse a material lib file.
   void parseMaterialLib(llvm::StringRef File);
   void parseMaterialLib();

public:
   explicit OBJParser(llvm::MemoryBuffer *Buf, llvm::StringRef Path)
      : Buf(Buf), Path(Path),
        CurPtr(Buf->getBufferStart()),
        EndPtr(Buf->getBufferEnd())
   {}

   Mesh parseMesh();
};

bool is_word_separator(char c)
{
   switch (c) {
   case ' ':
   case '\t':
   case '\n':
   case '\r':
      return true;
   default:
      return false;
   }
}

bool is_newline(char c)
{
   switch (c) {
   case '\n':
   case '\r':
      return true;
   default:
      return false;
   }
}

struct Vertex {
   glm::vec3 Coord;
   glm::vec2 Tex;
   glm::vec3 Norm;

   bool operator==(const Vertex &RHS) const
   {
      return Coord == RHS.Coord
         && Tex == RHS.Tex
         && Norm == RHS.Norm;
   }

   bool operator!=(const Vertex &RHS) const { return !(*this == RHS); }
};

} // anonymous namespace

namespace std {

template<>
struct hash<Vertex> {
   std::size_t operator()(const Vertex &k) const noexcept
   {
      std::size_t hashVal = 0;
      hash_combine(hashVal, k.Coord);
      hash_combine(hashVal, k.Tex);
      hash_combine(hashVal, k.Norm);

      return hashVal;
   }
};

} // namespace std

unsigned OBJParser::advanceChar()
{
   if (CurPtr[0] == '\r' && CurPtr[1] == '\n') {
      CurPtr += 2;
      return 2;
   }

   ++CurPtr;
   return 1;
}

void OBJParser::skipComment()
{
   if (*CurPtr == '#') {
      advanceChar();
      skipLine();

      return skipComment();
   }
}

void OBJParser::skipLine()
{
   while (CurPtr < EndPtr && !is_newline(*CurPtr)) {
      advanceChar();
   }
}

llvm::StringRef OBJParser::nextWord()
{
   skipComment();

   while (is_word_separator(*CurPtr)) {
      advanceChar();
      skipComment();
   }

   const char *BeginPtr = CurPtr;
   unsigned Size = 0;

   while (CurPtr < EndPtr && !is_word_separator(*CurPtr)) {
      Size += advanceChar();
      skipComment();
   }

   return llvm::StringRef(BeginPtr, Size);
}

void OBJParser::parseVertex(llvm::StringRef Str,
                            std::vector<unsigned> &Indices) {
   // 'Str' should have the format CoordIdx/TextureIdx/NormalIdx
   unsigned Idx = 0;
   unsigned Start = Idx;

   // Coordinate
   while (Str[Idx] != '/') {
      ++Idx;
   }

   auto CoordIdxStr = Str.substr(Start, Idx - Start);
   int CoordIdx = std::stoi(CoordIdxStr);

   // Texture
   Start = ++Idx;
   while (Str[Idx] != '/') {
      ++Idx;
   }

   auto TexIdxStr = Str.substr(Start, Idx - Start);
   int TexIdx = std::stoi(TexIdxStr);

   // Normal
   Start = ++Idx;
   while (Str.size() > Idx && !is_word_separator(Str[Idx])) {
      ++Idx;
   }

   auto NormIdxStr = Str.substr(Start, Idx - Start);
   int NormIdx = std::stoi(NormIdxStr);

   Indices.push_back(CoordIdx - 1);
   Indices.push_back(TexIdx - 1);
   Indices.push_back(NormIdx - 1);
}

Mesh OBJParser::parseMesh()
{
   Mesh M;

   std::vector<glm::vec3> Coordinates;
   std::vector<glm::vec2> Textures;
   std::vector<glm::vec3> Normals;
   std::vector<unsigned> Indices;

   while (CurPtr < EndPtr) {
      enum class DirectiveKind {
         MTLLIB, USEMTL, V, VT, VN, S, F, UNKNOWN,
      };

      if (is_newline(*CurPtr)) {
         advanceChar();
      }

      llvm::StringRef DirectiveStr = nextWord();
      auto Directive = llvm::StringSwitch<DirectiveKind>(DirectiveStr)
         .Case("mtllib", DirectiveKind::MTLLIB)
         .Case("usemtl", DirectiveKind::USEMTL)
         .Case("v", DirectiveKind::V)
         .Case("vt", DirectiveKind::VT)
         .Case("vn", DirectiveKind::VN)
         .Case("s", DirectiveKind::S)
         .Case("f", DirectiveKind::F)
         .Default(DirectiveKind::UNKNOWN);

      switch (Directive) {
      case DirectiveKind::UNKNOWN: {
         printf("Skipping unknown directive '%s' in .obj file...\n",
                DirectiveStr.str().c_str());
         skipLine();
         break;
      }
      case DirectiveKind::MTLLIB: {
         auto File = nextWord();
         parseMaterialLib(File);

         break;
      }
      case DirectiveKind::USEMTL: {
         auto Name = nextWord();
         auto It = Materials.find(Name);

         if (It != Materials.end()) {
            auto Tex = BasicTexture::fromFile(MaterialTextures[Name]);
            if (Tex) {
               Tex->setMaterial(It->getValue());
               M.Texture = std::move(*Tex);
            }
         }

         break;
      }
      case DirectiveKind::S:
         // FIXME Unhandled!
         skipLine();
         break;
      case DirectiveKind::V: {
         // X coordinate.
         auto xStr = nextWord();
         float x = std::stof(xStr);

         // Y coordinate.
         auto yStr = nextWord();
         float y = std::stof(yStr);

         // Z coordinate.
         auto zStr = nextWord();
         float z = std::stof(zStr);

         Coordinates.emplace_back(x, y, z);
         break;
      }
      case DirectiveKind::VT: {
         // X coordinate.
         auto xStr = nextWord();
         float x = std::stof(xStr);

         // Y coordinate.
         auto yStr = nextWord();
         float y = std::stof(yStr);

         Textures.emplace_back(x, y);
         break;
      }
      case DirectiveKind::VN: {
         // X coordinate.
         auto xStr = nextWord();
         float x = std::stof(xStr);

         // Y coordinate.
         auto yStr = nextWord();
         float y = std::stof(yStr);

         // Z coordinate.
         auto zStr = nextWord();
         float z = std::stof(zStr);

         Normals.emplace_back(x, y, z);
         break;
      }
      case DirectiveKind::F: {
         // First vertex (C/T/N)
         auto V1Str = nextWord();
         parseVertex(V1Str, Indices);

         // Second vertex (C/T/N)
         auto V2Str = nextWord();
         parseVertex(V2Str, Indices);

         // Third vertex (C/T/N)
         auto V3Str = nextWord();
         parseVertex(V3Str, Indices);

         break;
      }
      }
   }

   // Transform multiple-indices format to single index format needed by OpenGL.
   createMesh(Indices, Coordinates, Textures, Normals, M);

   return M;
}

void OBJParser::parseMaterialLib()
{
   Material M;
   llvm::StringRef Name;
   llvm::StringRef Texture;
   bool FoundNew = false;

   while (CurPtr < EndPtr) {
      enum class DirectiveKind {
         NEWMTL, KA, KD, KS, NS, D, TR, ILLUM, MAP_KD, UNKNOWN
      };

      if (is_newline(*CurPtr)) {
         advanceChar();
      }

      llvm::StringRef DirectiveStr = nextWord();
      auto Directive = llvm::StringSwitch<DirectiveKind>(DirectiveStr)
         .Case("newmtl", DirectiveKind::NEWMTL)
         .Case("Ka", DirectiveKind::KA)
         .Case("Kd", DirectiveKind::KD)
         .Case("Ks", DirectiveKind::KS)
         .Case("Ns", DirectiveKind::NS)
         .Case("d", DirectiveKind::D)
         .Case("Tr", DirectiveKind::TR)
         .Case("illum", DirectiveKind::ILLUM)
         .Case("map_Kd", DirectiveKind::MAP_KD)
         .Default(DirectiveKind::UNKNOWN);

      switch (Directive) {
      case DirectiveKind::UNKNOWN: {
         printf("Skipping unknown directive '%s' in .mtl file...\n",
                DirectiveStr.str().c_str());
         skipLine();
         break;
      }
      case DirectiveKind::NEWMTL: {
         if (FoundNew) {
            Materials[Name] = M;
            MaterialTextures[Name] = Texture;
         }

         Name = nextWord();
         M = Material();
         Texture = "";
         FoundNew = true;

         break;
      }
      case DirectiveKind::KA: {
         auto R = nextWord();
         auto G = nextWord();
         auto B = nextWord();

         M.AmbientColor = glm::vec3(std::stof(R), std::stof(G), std::stof(B));
         break;
      }
      case DirectiveKind::KD: {
         auto R = nextWord();
         auto G = nextWord();
         auto B = nextWord();

         M.DiffuseColor = glm::vec3(std::stof(R), std::stof(G), std::stof(B));
         break;
      }
      case DirectiveKind::KS: {
         auto R = nextWord();
         auto G = nextWord();
         auto B = nextWord();

         M.SpecularColor = glm::vec3(std::stof(R), std::stof(G), std::stof(B));
         break;
      }
      case DirectiveKind::NS: {
         M.SpecularExponent = std::stof(nextWord());
         break;
      }
      case DirectiveKind::D: {
         M.Alpha = 1.0f - std::stof(nextWord());
         break;
      }
      case DirectiveKind::TR: {
         M.Alpha = std::stof(nextWord());
         break;
      }
      case DirectiveKind::ILLUM: {
         M.IllumMode = std::stoi(nextWord());
         break;
      }
      case DirectiveKind::MAP_KD: {
         while (is_word_separator(*CurPtr)) {
            advanceChar();
            skipComment();
         }

         const char *BeginPtr = CurPtr;
         unsigned Size = 0;

         while (CurPtr < EndPtr && !is_newline(*CurPtr)) {
            Size += advanceChar();
         }

         Texture = llvm::StringRef(BeginPtr, Size);
         break;
      }
      }
   }

   if (FoundNew) {
      Materials[Name] = M;
      MaterialTextures[Name] = Texture;
   }
}

void OBJParser::createMesh(const std::vector<unsigned> &Indices,
                           const std::vector<glm::vec3> &Coordinates,
                           const std::vector<glm::vec2> &Textures,
                           const std::vector<glm::vec3> &Normals,
                           Mesh &M) {
   // The OBJ format uses one index buffer per attribute, while the OpenGL
   // format uses a single index buffer.
   auto BufferSize = Indices.size();

   std::unordered_map<Vertex, unsigned> InsertedVertices;
   for (size_t i = 0; i < BufferSize; i += 3) {
      const glm::vec3 &Coord = Coordinates[Indices[i]];
      const glm::vec2 &Tex   = Textures[Indices[i + 1]];
      const glm::vec3 &Norm  = Normals[Indices[i + 2]];

      Vertex Vert{ Coord, Tex, Norm };

      // Check if an equal vertex is already in the vector.
      auto It = InsertedVertices.find(Vert);
      if (It != InsertedVertices.end()) {
         M.Indices.push_back(It->second);
         continue;
      }

      // Insert the new vertex.
      M.Coordinates.push_back(Coord);
      M.UVs.push_back(Tex);
      M.Normals.push_back(Norm);

      unsigned CurIdx = M.Coordinates.size() - 1;
      M.Indices.push_back(CurIdx);
      InsertedVertices[Vert] = CurIdx;
   }
}

void OBJParser::createMeshSimple(const std::vector<unsigned> &Indices,
                                 const std::vector<glm::vec3> &Coordinates,
                                 const std::vector<glm::vec2> &Textures,
                                 const std::vector<glm::vec3> &Normals,
                                 Mesh &M) {
// The OBJ format uses one index buffer per attribute, while the OpenGL
   // format uses a single index buffer.
   auto BufferSize = Indices.size();

   for (size_t i = 0, CurIdx = 0; i < BufferSize; i += 3, ++CurIdx) {
      const glm::vec3 &Coord = Coordinates[Indices[i]];
      const glm::vec2 &Tex   = Textures[Indices[i + 1]];
      const glm::vec3 &Norm  = Normals[Indices[i + 2]];

      M.Coordinates.push_back(Coord);
      M.UVs.push_back(Tex);
      M.Normals.push_back(Norm);
      M.Indices.push_back(CurIdx);
   }
}

#ifdef _WIN32
static char PATH_SEPARATOR = '\\';
#else
static char PathSeperator = '/';
#endif

static llvm::StringRef getPath(llvm::StringRef fullPath)
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

static llvm::StringRef getFileNameAndExtension(llvm::StringRef fullPath)
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

static bool fileExists(llvm::StringRef name)
{
   return llvm::sys::fs::is_regular_file(name);
}

static std::string findFileInDirectories(llvm::StringRef fileName,
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

void OBJParser::parseMaterialLib(llvm::StringRef File)
{
   using namespace llvm;

   auto RealFile = findFileInDirectories(File, Path.str());
   if (RealFile.empty()) {
      return;
   }

   auto MaybeBuf = MemoryBuffer::getFile(RealFile);
   if (!MaybeBuf) {
      return;
   }

   OBJParser Parser(MaybeBuf.get().get(), getPath(File));
   Parser.parseMaterialLib();

   this->Materials = std::move(Parser.Materials);
   this->MaterialTextures = std::move(Parser.MaterialTextures);
}

llvm::Optional<Mesh> parseMesh(llvm::StringRef ObjFile)
{
   auto MaybeBuf = llvm::MemoryBuffer::getFile(ObjFile);
   if (!MaybeBuf) {
      return llvm::None;
   }

   OBJParser Parser(MaybeBuf.get().get(), getPath(ObjFile));
   return Parser.parseMesh();
}