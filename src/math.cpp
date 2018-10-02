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
        vec3 deltaPosition1 = corner.GetPrev()->GetVertex()->GetPosition() - corner.GetVertex()->GetPosition(),
             deltaPosition2 = corner.GetNext()->GetVertex()->GetPosition() - corner.GetVertex()->GetPosition();
        vec2 deltaTexCoords1 = corner.GetPrev()->GetTexCoords() - corner.GetTexCoords(),
             deltaTexCoords2 = corner.GetNext()->GetTexCoords() - corner.GetTexCoords();

        tangent = normalize((deltaTexCoords2.y * deltaPosition1 - deltaTexCoords1.y * deltaPosition2)
                       / (deltaTexCoords1.x * deltaTexCoords2.y - deltaTexCoords2.x * deltaTexCoords1.y));
        bitangent = normalize((deltaTexCoords2.x * deltaPosition1 - deltaTexCoords1.x * deltaPosition2)
                         / (deltaTexCoords1.y * deltaTexCoords2.x - deltaTexCoords2.y * deltaTexCoords1.x));
    }

    /**
      * source: http://courses.washington.edu/arch481/1.Tapestry%20Reader/1.3D%20Data/5.Surface%20Normals/0.default.html
      */
    vec3 CalculateCornerNormal(const MeshCorner &corner)
    {
        return normalize(cross(corner.GetVertex()->GetPosition() - corner.GetPrev()->GetVertex()->GetPosition(),
                          corner.GetNext()->GetVertex()->GetPosition() - corner.GetVertex()->GetPosition()));
    }

    vec3 CalculateFaceNormal(const MeshFace *pFace)
    {
        vec3 sum(0.0f, 0.0f, 0.0f);
        for (const MeshCorner &corner : pFace->IterCorners())
        {
            sum += CalculateCornerNormal(corner);
        }
        return normalize(sum);
    }

    std::tuple<vec3, vec3> CalculateFaceTangentBitangent(const MeshFace *pFace)
    {
        vec3 sumTangent(0.0f, 0.0f, 0.0f),
             sumBitangent(0.0f, 0.0f, 0.0f),
             t, b;
        for (const MeshCorner &corner : pFace->IterCorners())
        {
            CalculateCornerTangentBitangent(corner, t, b);
            sumTangent += t;
            sumBitangent += b;
        }
        return std::make_tuple(normalize(sumTangent), normalize(sumBitangent));
    }

    vec3 CalculateVertexNormal(const MeshVertex *pVertex)
    {
        vec3 sum(0.0f, 0.0f, 0.0f);
        for (const MeshCorner *pCorner : pVertex->IterCorners())
        {
            sum += CalculateCornerNormal(*pCorner);
        }
        return normalize(sum);
    }

    std::tuple<vec3, vec3> CalculateVertexTangentBitangent(const MeshVertex *pVertex)
    {
        vec3 sumTangent(0.0f, 0.0f, 0.0f),
             sumBitangent(0.0f, 0.0f, 0.0f),
             t, b;
        for (const MeshCorner *pCorner : pVertex->IterCorners())
        {
            CalculateCornerTangentBitangent(*pCorner, t, b);
            sumTangent += t;
            sumBitangent += b;
        }
        return std::make_tuple(normalize(sumTangent), normalize(sumBitangent));
    }
}
