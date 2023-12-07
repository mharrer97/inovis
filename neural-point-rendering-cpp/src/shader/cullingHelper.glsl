
// plane_normal must be normalized!!
// returns the distance of a point to a plane
// > 0 on side, the normal is pointing to
// < 0 on side, the normal is not pointing to
float intersectSpherePlane(vec3 sphere_c, vec3 plane_point, vec3 plane_normal)
{
    //vec3 plane_normal_normalized = normalize(plane_normal); // normalize 
    float plane_d = dot(plane_point, plane_normal); // get distance to the origin (projected to n)
    return dot(sphere_c, plane_normal) - plane_d; // return distance of sphere_center to plane
}