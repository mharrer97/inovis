/*
Adapted from Florian Guthleins pointcloud data
*/


#pragma once


#include <iostream>
#include <glm/glm.hpp>

using vec3 = glm::vec3;
using vec2 = glm::vec2;
using ivec2 = glm::ivec2;





// vec3 for view position
//[0] view_px:  <0, 4> float
//[1] view_py : <4, 4> float
//[2] view_pz : <8, 4> float
// vec3 for x axis
//[3] x_axisx : <12, 4> float
//[4] x_axisy : <16, 4> float
//[5] x_axisz : <20, 4> float
// vec3 for y axis
//[6] y_axisx : <24, 4> float
//[7] y_axisy : <28, 4> float
//[8] y_axisz : <32, 4> float
// vec3 for z axis
//[9] z_axisx : <36, 4> float
//[10] z_axisy : <40, 4> float
//[11] z_axisz : <44, 4> float
// float for focal
//[12] focal : <48, 4> float
// vec2 for scale
//[13] scalex : <52, 4> float
//[14] scaley : <56, 4> float
// vec2 for center
//[15] centerx : <60, 4> float
//[16] centery : <64, 4> float
// ivec2 for viewport
//[17] viewportx : <68, 4> int
//[18] viewporty : <72, 4> int
// vec2 for k
//[19] k1 : <76, 4> float
//[20] k2 : <80, 4> float
struct PointCloudCameraData {
    vec3 view_p;
    vec3 x_axis;
    vec3 y_axis;
    vec3 z_axis;
    float focal;
    vec2 scale;
    vec2 center;
    ivec2 viewport;
    vec2 k;
};


// vec3 for position
//[0] x:   <0, 4> float
//[1] y : <4, 4> float
//[2] z : <8, 4> float
// vec3 for color (only 1 byte each but we dont care)
//[3] red : <12, 1> uchar
//[4] green : <13, 1> uchar
//[5] blue : <14, 1> uchar
// vec3 for normal
//[6] nx : <15, 4> float
//[7] ny : <19, 4> float
//[8] nz : <23, 4> float
// float for curvature
//[9] curvature : <27, 4> float

struct PointCloudPoint {
    vec3 pos;
    vec3 color;
    vec3 normal;    
    float curvature;
    int timestamp;
};

/// <summary>
/// An axis aligned Voxel is one element of a bounding structure used to structure the pointcloud for e.g. culling
/// To use these, pointclouds should be sorted such that all points of a voxel are stored in a region together
/// </summary>
struct PointCloudVoxel {
    vec3 center; // center of the voxel
    float radius; // radius, i.e. distance from center to a corner
    vec3 aabb_min; // min of the axis aligned voxel
    unsigned int start; // start index of the voxel in the pointcloud
    vec3 aabb_max; // max of the axis aligned voxel
    unsigned int size; // count of points in the voxel
};

/// <summary>
/// overloaded output operator: prints cameradata
/// </summary>
/// <param name="os">output stream</param>
/// <param name="data">camera data to output</param>
/// <returns></returns>
std::ostream& operator<<(std::ostream&, const PointCloudCameraData&);
/// <summary>
/// overloaded output operator: prints pointdata
/// </summary>
/// <param name="os">output stream</param>
/// <param name="data">point data to output</param>
/// <returns></returns>
std::ostream& operator<<(std::ostream&, const PointCloudPoint&);

//namespace glmio {
//    //overload needed vector output operators
//    std::ostream& operator<<(std::ostream& os, const vec3& v) {
//        os << "[" << v.x << "," << v.y << "," << v.z << "]" << std::endl;
//        return os;
//    }
//    std::ostream& operator<<(std::ostream& os, const vec2& v) {
//        os << "[" << v.x << "," << v.y << "]" << std::endl;
//        return os;
//    }
//
//    std::ostream& operator<<(std::ostream& os, const ivec2& v) {
//        os << "[" << v.x << "," << v.y << "]" << std::endl;
//        return os;
//    }
//    
//}