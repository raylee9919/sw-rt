// Copyright Seong Woo Lee. All Rights Reserved.

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "base.h"
#include "math.h"
#include "string.h"
#include "geometry.h"
#include "asset.h"

void load_and_parse_obj(String filename, Triangle_Mesh *mesh, AABB *aabb)
{
    assert(mesh);
    assert(aabb);

    std::string inputfile = std::string((const char *)filename.data);
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        assert(0);
    }

    if (!reader.Warning().empty()) {
        printf("%s\n", reader.Warning().c_str());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    // Get vertex count.
    //
    int num_vert = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            for (size_t v = 0; v < fv; v++) {
                num_vert++;
            }
        }
    }

    mesh->num = num_vert;
    mesh->vertices = new Vec3[num_vert];
    mesh->normals  = new Vec3[num_vert];

    f32 min_x =  f32_max;
    f32 min_y =  f32_max;
    f32 min_z =  f32_max;
    f32 max_x = -f32_max;
    f32 max_y = -f32_max;
    f32 max_z = -f32_max;


    assert(shapes.size() == 1);
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            assert(fv == 3); // Triangle only.

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                auto *vert = &mesh->vertices[f*3 + v];
                vert->x = vx;
                vert->y = vy;
                vert->z = vz;

                min_x = min(min_x, vx);
                min_y = min(min_y, vy);
                min_z = min(min_z, vz);
                max_x = max(max_x, vx);
                max_y = max(max_y, vy);
                max_z = max(max_z, vz);

                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];

                    auto *normal = &mesh->normals[f*3 + v];
                    normal->x = nx;
                    normal->y = ny;
                    normal->z = nz;
                } else {
                    tinyobj::index_t idx1 = shapes[s].mesh.indices[index_offset + 0];
                    tinyobj::real_t x1 = attrib.vertices[3*size_t(idx1.vertex_index)+0];
                    tinyobj::real_t y1 = attrib.vertices[3*size_t(idx1.vertex_index)+1];
                    tinyobj::real_t z1 = attrib.vertices[3*size_t(idx1.vertex_index)+2];

                    tinyobj::index_t idx2 = shapes[s].mesh.indices[index_offset + 1];
                    tinyobj::real_t x2 = attrib.vertices[3*size_t(idx2.vertex_index)+0];
                    tinyobj::real_t y2 = attrib.vertices[3*size_t(idx2.vertex_index)+1];
                    tinyobj::real_t z2 = attrib.vertices[3*size_t(idx2.vertex_index)+2];

                    tinyobj::index_t idx3 = shapes[s].mesh.indices[index_offset + 2];
                    tinyobj::real_t x3 = attrib.vertices[3*size_t(idx3.vertex_index)+0];
                    tinyobj::real_t y3 = attrib.vertices[3*size_t(idx3.vertex_index)+1];
                    tinyobj::real_t z3 = attrib.vertices[3*size_t(idx3.vertex_index)+2];

                    Vec3 vert1 = Vec3(x1, y1, z1);
                    Vec3 vert2 = Vec3(x2, y2, z2);
                    Vec3 vert3 = Vec3(x3, y3, z3);
                    Vec3 n = normalize(cross(vert3 - vert1, vert2 - vert1));

                    auto *normal = &mesh->normals[f*3 + v];
                    normal->x = n.x;
                    normal->y = n.y;
                    normal->z = n.z;
                }
            }
            index_offset += fv;
        }
    }


    f32 eps = 0.001f;
    min_x -= eps;
    min_y -= eps;
    min_z -= eps;
    max_x += eps;
    max_y += eps;
    max_z += eps;
    aabb->min = Vec3(min_x, min_y, min_z);
    aabb->max = Vec3(max_x, max_y, max_z);
}
