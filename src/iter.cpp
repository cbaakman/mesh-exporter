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
