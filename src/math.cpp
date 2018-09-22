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

#include "mesh.h"


namespace XMLMesh
{
    /**
     * source: http://www.terathon.com/code/tangent.html
     */
    void CalculateCornerTangentBitangent(const MeshCorner &corner,
                                         vec3 &tangent, vec3 &bitangent)
    {
        vec3 deltaPosition1 = corner.pPrev->pVertex->position - corner.pVertex->position,
             deltaPosition2 = corner.pNext->pVertex->position - corner.pVertex->position;
        vec2 deltaTexCoords1 = corner.pPrev->texCoords - corner.texCoords,
             deltaTexCoords2 = corner.pNext->texCoords - corner.texCoords;

        tangent = Unit((deltaTexCoords2.v * deltaPosition1 - deltaTexCoords1.v * deltaPosition2)
                       / (deltaTexCoords1.u * deltaTexCoords2.v - deltaTexCoords2.u * deltaTexCoords1.v));
        bitangent = Unit((deltaTexCoords2.u * deltaPosition1 - deltaTexCoords1.u * deltaPosition2)
                         / (deltaTexCoords1.v * deltaTexCoords2.u - deltaTexCoords2.v * deltaTexCoords1.u));
    }

    /**
      * source: http://courses.washington.edu/arch481/1.Tapestry%20Reader/1.3D%20Data/5.Surface%20Normals/0.default.html
      */
    vec3 CalculateCornerNormal(const MeshCorner &corner)
    {
        return Unit(Cross(corner.pVertex->position - corner.pPrev->pVertex->position,
                          corner.pNext->pVertex->position - corner.pVertex->position));
    }

    vec3 CalculateFaceNormal(const MeshFace &face)
    {
        vec3 sum(0.0f, 0.0f, 0.0f);
        for (const MeshCorner &corner : face.IterCorners())
        {
            sum += CalculateCornerNormal(corner);
        }
        return Unit(sum);
    }

    std::tuple<vec3, vec3> CalculateFaceTangentBitangent(const MeshFace &face)
    {
        vec3 sumTangent(0.0f, 0.0f, 0.0f),
             sumBitangent(0.0f, 0.0f, 0.0f),
             t, b;
        for (const MeshCorner &corner : face.IterCorners())
        {
            CalculateCornerTangentBitangent(corner, t, b);
            sumTangent += t;
            sumBitangent += b;
        }
        return std::make_tuple(Unit(sumTangent), Unit(sumBitangent));
    }

    vec3 CalculateVertexNormal(const MeshVertex &vertex)
    {
        vec3 sum(0.0f, 0.0f, 0.0f);
        for (const MeshCorner *pCorner : vertex.cornersInvolvedPs)
        {
            sum += CalculateCornerNormal(*pCorner);
        }
        return Unit(sum);
    }

    std::tuple<vec3, vec3> CalculateVertexTangentBitangent(const MeshVertex &vertex)
    {
        vec3 sumTangent(0.0f, 0.0f, 0.0f),
             sumBitangent(0.0f, 0.0f, 0.0f),
             t, b;
        for (const MeshCorner *pCorner : vertex.cornersInvolvedPs)
        {
            CalculateCornerTangentBitangent(*pCorner, t, b);
            sumTangent += t;
            sumBitangent += b;
        }
        return std::make_tuple(Unit(sumTangent), Unit(sumBitangent));
    }
}
