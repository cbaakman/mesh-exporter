/* Copyright (C) 2018 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#ifndef BUILD_H
#define BUILD_H

#include "mesh.h"


namespace XMLMesh
{
    class MeshDataBuilder
    {
        private:
            MeshData *pMeshData;
        public:
            MeshDataBuilder(void);

            void AddVertex(const std::string &id, const vec3 &position);
            void AddQuad(const std::string id, const bool smooth,
                         const MeshTexCoords *, const std::string *vertexIDs);
            void AddTriangle(const std::string id, const bool smooth,
                             const MeshTexCoords *, const std::string *vertexIDs);
            void AddSubset(const std::string &id);
            void AddQuadToSubset(const std::string &subsetID, const std::string &quadID);
            void AddTriangleToSubset(const std::string &subsetID, const std::string &triangleID);
            void AddBone(const std::string &id, const vec3 &headPosition, const float weight);
            void ConnectBoneToVertex(const std::string &boneID, const std::string &vertexID);
            void ConnectBones(const std::string &parentID, const std::string &childID);
            void AddKey(const std::string &animationID, const std::string &boneID,
                        const size_t frame, const MeshBoneTransformation &);
            void AddLayer(const std::string &animationID, const std::string &boneID);
            void AddAnimation(const std::string &animationID, const size_t length);

            MeshData *GetMeshData(void);
    };


    class MeshStateBuilder
    {
        private:
            MeshState *pMeshState;
        public:
            MeshStateBuilder(void);

            void AddVertex(const std::string &id, const vec3 &position);
            void AddQuad(const std::string id, const bool smooth,
                         const MeshTexCoords *, const std::string *vertexIDs);
            void AddTriangle(const std::string id, const bool smooth,
                             const MeshTexCoords *, const std::string *vertexIDs);
            void AddSubset(const std::string &id);
            void AddQuadToSubset(const std::string &subsetID, const std::string &quadID);
            void AddTriangleToSubset(const std::string &subsetID, const std::string &triangleID);

            MeshState *GetMeshState(void);
    };
}
#endif  // BUILD_H
