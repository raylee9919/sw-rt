// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct Entity {
    Vec3 position;

    AABB aabb;
    Triangle_Mesh mesh;
};
