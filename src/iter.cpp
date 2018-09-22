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
    MeshCornerIterable::MeshCornerIterable(MeshCorner *array, const size_t length)
    {
        this->array = array;
        this->length = length;
    }
    MeshCorner *MeshCornerIterable::begin(void) const
    {
        return array;
    }
    MeshCorner *MeshCornerIterable::end(void) const
    {
        return array + length;
    }

    ConstMeshCornerIterable::ConstMeshCornerIterable(const MeshCorner *array, const size_t length)
    {
        this->array = array;
        this->length = length;
    }
    const MeshCorner *ConstMeshCornerIterable::begin(void) const
    {
        return array;
    }
    const MeshCorner *ConstMeshCornerIterable::end(void) const
    {
        return array + length;
    }

    MeshFace::MeshFace(const size_t n)
    {
        mCorners = new MeshCorner[n];
        countCorners = n;
    }
    MeshFace::MeshFace(const MeshFace &face)
    {
        mCorners = new MeshCorner[face.countCorners];
        countCorners = face.countCorners;

        size_t i;
        for (i = 0; i < countCorners; i++)
            mCorners[i] = face.mCorners[i];
    }
    MeshFace::~MeshFace(void)
    {
        delete[] mCorners;
    }
    MeshCornerIterable MeshFace::IterCorners(void)
    {
        return MeshCornerIterable(mCorners, countCorners);
    }
    ConstMeshCornerIterable MeshFace::IterCorners(void) const
    {
        return ConstMeshCornerIterable(mCorners, countCorners);
    }
    MeshQuadFace::MeshQuadFace(void): MeshFace(4)
    {
    }
    MeshTriangleFace::MeshTriangleFace(void): MeshFace(3)
    {
    }
}
