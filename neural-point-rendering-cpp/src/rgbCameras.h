#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <cppgl.h>
#include <filesystem>

#include "parserConfig.h"


/*
* Organisation structure for rgb camera data
* adapted in part from Florian Guethlein's project
*/

struct ReferenceFrameRGBCameraImpl {
    std::string name;
    ReferenceFrameRGBCameraImpl(std::string name);
    struct Camera {
        //struct pose

        struct Pose {
            glm::vec3 position;
            glm::quat orientation;
        } original_pose;
        struct ocam_model {
            glm::ivec2 image_size;
            double c;
            double d;
            double e;
            double cx;
            double cy;
            std::vector<double> cam2world;
            std::vector<double> world2cam;
            glm::vec3 projectToWorld(const glm::vec2&) const;
            glm::vec2 projectToCamera(const glm::vec3&) const;
        } model;

    };

    struct CamHeadPose {        
        glm::vec3 position;
        glm::quat orientation;
    }head_pose;
    

    Camera cams[NUMBER_OF_CAMERAS_PER_CP];

};
using ReferenceFrameRGBCamera = NamedHandle<ReferenceFrameRGBCameraImpl>;

struct RGBCamerasImpl {
    std::string name;
    ReferenceFrameRGBCamera reference_frame;
    RGBCamerasImpl(std::string name);
    struct Complete_Camera {
        struct RGBImage {
            RGBImage() :w(0), h(0), channels(0) {}
            void loadFromFile(std::filesystem::path& file_path, std::filesystem::path& depth_path);
            //void loadDepthFromFile(std::filesystem::path& file_path);
            int w, h, channels;
          //  std::vector<glm::vec4> tex_cpu;
            Texture2D tex_gpu;
            //Texture2D tex_depth_gpu;
        } rgbImage;
        struct Pose {
            glm::vec3 position;
            glm::quat orientation;
            //  Pose(glm::vec3 pos, glm::quat orient) :position(pos), orientation(orient) {}
        } pose, optimized_pose;
        float exposure = 0.f;
    };
    bool darius_optimized_pose_present = false;

    Complete_Camera cams[NUMBER_OF_CAMERAS_PER_CP];

};

using RGBCameras = NamedHandle<RGBCamerasImpl>;


namespace Parser {
    RGBCameras parseCams(std::filesystem::path& model_path, std::string file_prefix, ReferenceFrameRGBCamera ref, int numOfCams=NUMBER_OF_CAMERAS_PER_CP, bool noTex = false);
    ReferenceFrameRGBCamera parseReferenceFile(std::filesystem::path& file_path);


};

