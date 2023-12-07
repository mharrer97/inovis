#include "rgbCameras.h"
#include "../util/xml/pugixml.hpp"
namespace fs = std::filesystem;
//#define STB_IMAGE_IMPLEMENTATION
#include "../external/advancedcppgl/external/cppgl/src/stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/advancedcppgl/external/cppgl/src/stb_image_write.h"
#include <fstream>
#include "../util/json/json11.hpp"


ReferenceFrameRGBCameraImpl::ReferenceFrameRGBCameraImpl(std::string name):name(name) {

}

/// <summary>
/// Parse the sensor_frame.xml file to get calibrations of the cam
/// </summary>
/// <param name="file_path">path to the sensor_frame.xml</param>
/// <returns>struct containing the calibration parameters</returns>
ReferenceFrameRGBCamera Parser::parseReferenceFile(std::filesystem::path& file_path)
{
    ReferenceFrameRGBCamera complete_cam_system("rgbCameras_"+ file_path.string());
    pugi::xml_document doc;
    auto res = doc.load_file(file_path.string().c_str());
    if (!res) throw std::runtime_error(std::string("cannot parse file") + file_path.string());


    pugi::xpath_node_set camera_head =
        doc.select_nodes("/SensorFrame/CameraHead");
    for (pugi::xpath_node node : camera_head) {
        pugi::xml_node xnode = node.node();
        auto pose = xnode.select_node("Pose");
        auto pos = pose.node().select_node("position");
        complete_cam_system->head_pose.position.x =
            pos.node().select_node("x").node().text().as_double();
        complete_cam_system->head_pose.position.y =
            pos.node().select_node("y").node().text().as_double();
        complete_cam_system->head_pose.position.z =
            pos.node().select_node("z").node().text().as_double();
        auto quat = pose.node().select_node("orientation");
        complete_cam_system->head_pose.orientation.x =
            quat.node().select_node("x").node().text().as_double();
        complete_cam_system->head_pose.orientation.y =
            quat.node().select_node("y").node().text().as_double();
        complete_cam_system->head_pose.orientation.z =
            quat.node().select_node("z").node().text().as_double();
        complete_cam_system->head_pose.orientation.w =
            quat.node().select_node("w").node().text().as_double();
    }


    pugi::xpath_node_set camera_models =
        doc.select_nodes("/SensorFrame/CameraHead/CameraModel");

    int i = 0;
    for (pugi::xpath_node node : camera_models) {
        ReferenceFrameRGBCameraImpl::Camera c;
        pugi::xml_node xnode = node.node();
        auto sensor_name = xnode.select_node("SensorName").node().text().get();
        if (sensor_name != "cam" + std::to_string(i))
            throw std::runtime_error(std::string("expected sensor name cam") +
                std::to_string(i) + " got " + sensor_name);

        auto pose = xnode.select_node("Pose");
        auto pos = pose.node().select_node("position");
        c.original_pose.position.x =
            pos.node().select_node("x").node().text().as_double();
        c.original_pose.position.y =
            pos.node().select_node("y").node().text().as_double();
        c.original_pose.position.z =
            pos.node().select_node("z").node().text().as_double();

        auto quat = pose.node().select_node("orientation");
        c.original_pose.orientation.x =
            quat.node().select_node("x").node().text().as_double();
        c.original_pose.orientation.y =
            quat.node().select_node("y").node().text().as_double();
        c.original_pose.orientation.z =
            quat.node().select_node("z").node().text().as_double();
        c.original_pose.orientation.w =
            quat.node().select_node("w").node().text().as_double();

        auto size = xnode.select_node("ImageSize");
        c.model.image_size.x =
            size.node().select_node("Width").node().text().as_int();
        c.model.image_size.y =
            size.node().select_node("Height").node().text().as_int();

        auto ocmodel = xnode.select_node("OCamModel");
        c.model.c = ocmodel.node().select_node("c").node().text().as_double();
        c.model.d = ocmodel.node().select_node("d").node().text().as_double();
        c.model.e = ocmodel.node().select_node("e").node().text().as_double();
        c.model.cx = ocmodel.node().select_node("cx").node().text().as_double();
        c.model.cy = ocmodel.node().select_node("cy").node().text().as_double();

        auto cam2worldCoeffs = ocmodel.node()
            .select_node("cam2world")
            .node()
            .select_nodes("coeff");
        for (pugi::xpath_node coeff : cam2worldCoeffs)
            c.model.cam2world.push_back(coeff.node().text().as_double());

        auto world2camCoeffs = ocmodel.node()
            .select_node("world2cam")
            .node()
            .select_nodes("coeff");
        for (pugi::xpath_node coeff : world2camCoeffs)
            c.model.world2cam.push_back(coeff.node().text().as_double());
        if (i >= sizeof(complete_cam_system->cams) / sizeof(complete_cam_system->cams[0])) {
            std::cerr << "File " << file_path.string() << " contains more than "
                << NUMBER_OF_CAMERAS_PER_CP << " cameras! Skipping "
                << sensor_name << std::endl;
        }
        else {
            complete_cam_system->cams[i] = c;
            ++i;
        }
    }




    return complete_cam_system;
}


glm::vec3 ReferenceFrameRGBCameraImpl::Camera::ocam_model::projectToWorld(
    const glm::vec2& point2D) const
{
    double invdet =
        1 / (c - d * e);  // 1/det(A), where A = [c,d;e,1] as in the Matlab file

    double xp = invdet * ((point2D[0] - cx) - d * (point2D[1] - cy));
    double yp = invdet * (-e * (point2D[0] - cx) + c * (point2D[1] - cy));

    double r =
        sqrt(xp * xp +
            yp * yp);  // distance [pixels] of  the point from the image center
    double zp = cam2world[0];
    double r_i = 1;
    int i;

    for (i = 1; i < cam2world.size(); i++) {
        r_i *= r;
        zp += r_i * cam2world[i];
    }

    // normalize to unit norm
    double invnorm = 1 / sqrt(xp * xp + yp * yp + zp * zp);

    glm::vec3 point3D;
    point3D[0] = invnorm * xp;
    point3D[1] = invnorm * yp;
    point3D[2] = invnorm * zp;
    return point3D;
}

glm::vec2 ReferenceFrameRGBCameraImpl::Camera::ocam_model::projectToCamera(
    const glm::vec3& point3D) const
{
    double norm = sqrt(point3D[0] * point3D[0] + point3D[1] * point3D[1]);
    double theta = atan(point3D[2] / norm);
    double t, t_i;
    double rho, x, y;
    double invnorm;
    int i;

    glm::vec2 point2D;
    if (norm != 0) {
        invnorm = 1 / norm;
        t = theta;
        rho = world2cam[0];
        t_i = 1;

        for (i = 1; i < world2cam.size(); i++) {
            t_i *= t;
            rho += t_i * world2cam[i];
        }

        x = point3D[0] * invnorm * rho;
        y = point3D[1] * invnorm * rho;

        point2D[0] = x * c + y * d + cx;
        point2D[1] = x * e + y + cy;
    }
    else {
        point2D[0] = cx;
        point2D[1] = cy;
    }
    return point2D;
}

/// <summary>
/// parse a json file containing the positions and orientations of the 4 cams + the head
/// </summary>
/// <param name="file">file path to the json file</param>
/// <returns>vector of poses from the info json file</returns>
static std::vector<RGBCamerasImpl::Complete_Camera::Pose> parseJsonFile(fs::path file)
{
    std::vector<RGBCamerasImpl::Complete_Camera::Pose> poses;
    std::fstream in(file);
    if (!in) throw std::runtime_error(std::string("no such file") + file.string());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    std::string err;
    auto json = json11::Json::parse(contents, err);
    if (json == json11::Json())
        throw std::runtime_error(std::string("cannot parse file") + file.string());

    if (!json.is_object()) throw std::runtime_error("unexpected type");
    std::vector<std::string> strings;
    for (int i = 0; i != NUMBER_OF_CAMERAS_PER_CP; ++i) {
        strings.push_back("cam" + std::to_string(i));
    }
    strings.push_back("cam_head");
    strings.push_back("footprint");
    int cntr = 0;
    for (auto s : strings) {
        auto& o = json.object_items().at(s);
        RGBCamerasImpl::Complete_Camera::Pose c;
        auto pos = o.object_items().at("position");
        c.position.x = pos.array_items()[0].number_value();
        c.position.y = pos.array_items()[1].number_value();
        c.position.z = pos.array_items()[2].number_value();
        auto ori = o.object_items().at("quaternion");
        c.orientation.w = ori.array_items()[0].number_value();
        c.orientation.x = ori.array_items()[1].number_value();
        c.orientation.y = ori.array_items()[2].number_value();
        c.orientation.z = ori.array_items()[3].number_value();

        poses.push_back(c);
    }

    return poses;
}

RGBCamerasImpl::RGBCamerasImpl(std::string name):name(name){}

inline GLint channels_to_float_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
}
inline GLint channels_to_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : channels == 2 ? GL_RG : GL_RED;
}
inline GLint channels_to_ubyte_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
}


void RGBCamerasImpl::Complete_Camera::RGBImage::loadFromFile(std::filesystem::path& file_path, std::filesystem::path& depth_path) {
    // load image from disk
    stbi_set_flip_vertically_on_load(1);
    int channels;
    int tex_channels = 4;
    GLint internal_format;
    GLenum format;
    GLenum type;
    uint8_t* data = 0;

    int w = 0;
    int h = 0;
    data = stbi_load(file_path.string().c_str(), &w, &h, &channels, 0);
    internal_format = channels_to_float_format(tex_channels);
    format = channels_to_format(tex_channels);
    type = GL_FLOAT;

    if (!data) {
        std::cerr << "[View::loadFromFile_RGB_D] FATAL ERROR: File not found: " << file_path << std::endl;
        throw std::runtime_error("Failed to load image file: " + file_path.string());
        return;
    }
    if (channels < 1 || channels>4)
        throw std::runtime_error("Image " + file_path.string() +
            " has unexpected number of channels: " + std::to_string(channels));

    std::vector<float> flipped_data;
    //write tex to cpu buffer

    flipped_data.reserve(w * h * tex_channels);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int index = (x + y * w) * channels;
            for (int i = 0; i < channels; ++i) {
                flipped_data.push_back(float(data[index + i]) / 255.f);
            }
            flipped_data.push_back(1);
        }
    }



    //opengl by default needs 4 byte alignment after every row
    //stbi loaded data is not aligned that way -> pixelStore attributes need to be set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);



    // free data
    stbi_image_free(data);

    data = stbi_load(depth_path.string().c_str(), &w, &h, &channels, 0);

    if (!data) {
        throw std::runtime_error("Failed to load image file: " + file_path.string());
        return;
    }
    if (channels < 1 || channels>4)
        throw std::runtime_error("Image " + file_path.string() +
            " has unexpected number of channels: " + std::to_string(channels));
    //write tex to cpu buffer

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int index = (x + y * w) * channels;
            int tex_index = (x + y * w) * tex_channels;

            flipped_data[tex_index + 3] = (float(data[index]) / 255.f);

        }
    }



    //opengl by default needs 4 byte alignment after every row
    //stbi loaded data is not aligned that way -> pixelStore attributes need to be set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);



    // free data
    stbi_image_free(data);

    tex_gpu = Texture2D(file_path.string(), w, h, internal_format, format, type, flipped_data.data(), true);

}

//void RGBCamerasImpl::Complete_Camera::RGBImage::loadDepthFromFile(fs::path& file_path) {
//    // load image from disk
//    stbi_set_flip_vertically_on_load(1);
//    int channels;
//    GLint internal_format;
//    GLenum format;
//    GLenum type;
//    uint8_t* data = 0;
//
//
//    data = stbi_load(file_path.string().c_str(), &w, &h, &channels, 0);
//    int tex_channels = 1;
//    internal_format = channels_to_float_format(tex_channels);
//    format = channels_to_format(tex_channels);
//    type = GL_FLOAT;
//
//    if (!data) {
//        throw std::runtime_error("Failed to load image file: " + file_path.string());
//        return;
//    }
//    if (channels < 1 || channels>4)
//        throw std::runtime_error("Image " + file_path.string() +
//            " has unexpected number of channels: " + std::to_string(channels));
//
//    std::vector<float> flipped_data;
//    //write tex to cpu buffer
//
//
//    flipped_data.reserve(w * h * tex_channels);
//
//    for (int y = 0; y < h; ++y) {
//        for (int x = 0; x < w; ++x) {
//            int index = (x + y * w) * channels;
//            for (int i = 0; i < tex_channels; ++i) {
//                flipped_data.push_back(float(data[index + i]) / 255.f);
//            }
//        }
//    }
//
//
//
//    //opengl by default needs 4 byte alignment after every row
//    //stbi loaded data is not aligned that way -> pixelStore attributes need to be set
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
//    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
//
//    tex_gpu = Texture2D(file_path.string(), w, h, internal_format, format, type, flipped_data.data(), true);
//
//    // free data
//    stbi_image_free(data);
//}

static std::vector<RGBCamerasImpl::Complete_Camera::Pose> getOptimizedPose(std::ifstream& opti_file, int cap_pos_num, int numOfCams) {
    std::vector<RGBCamerasImpl::Complete_Camera::Pose> result;
    //skip n lines
    std::string line;
    for (int i = 0; i < (cap_pos_num * numOfCams); ++i)
        std::getline(opti_file, line);

    //get correct line
    //std::getline(opti_file, line);
    for (int i = 0; i < numOfCams; ++i) {
        float q_w, q_x, q_y, q_z, p_x, p_y, p_z;
        opti_file >> q_w >> q_x >> q_y >> q_z >> p_x >> p_y >> p_z;
        //remove newline
        std::getline(opti_file, line);
        RGBCamerasImpl::Complete_Camera::Pose pose_opti;
        pose_opti.position = glm::vec3(p_x, p_y, p_z);
        pose_opti.orientation = glm::quat(q_w, q_x, q_y, q_z);

        result.emplace_back(pose_opti);
    }


    return result;
}


RGBCameras Parser::parseCams(std::filesystem::path& model_path, std::string file_prefix, ReferenceFrameRGBCamera ref, int numOfCams, bool noTex) {
    RGBCameras out_cams("RGB_Cameras_" + file_prefix + model_path.filename().string());
    out_cams->reference_frame = ref;

    for (int i = 0; i < numOfCams; ++i) {
        fs::path jpg_path = model_path;
        jpg_path /= "cam";
        jpg_path /= file_prefix + "-cam" + std::to_string(i) + ".jpg";
        fs::path jpg_depth_path = model_path;
        jpg_depth_path /= "cam_depth";
        jpg_depth_path /= file_prefix + "-cam" + std::to_string(i) + ".jpg";
        if(!noTex)
            out_cams->cams[i].rgbImage.loadFromFile(jpg_path, jpg_path);
            //out_cams->cams[i].rgbImage.loadDepthFromFile(jpg_depth_path);
    }


    //Darius Camera Optimizations
    auto opti_path = model_path / "darius_optimization" / "poses_quat_wxyz_pos_xyz.txt";
    //std::string opti_path = model_path + "/darius_optimization/poses_quat_wxyz_pos_xyz.txt";

    std::ifstream opti_file(opti_path);
    bool optimization_exists = opti_file.is_open();
    int cap_pos_num = stoi(file_prefix);
    //if (optimization_exists) std::cerr << "[RGBCAMERAS::PARSER] poses optimization found at " << opti_path << " !" << std::endl;
    //else std::cerr << "[RGBCAMERAS::PARSER] NO optimization found at " << opti_path << " ! " << std::endl;

    fs::path json_path = model_path;
    json_path /= "info";
    json_path /= file_prefix + "-info.json";

    auto parsed_poses = parseJsonFile(json_path);
    std::vector<RGBCamerasImpl::Complete_Camera::Pose> optimized_pose;
    if (optimization_exists) optimized_pose = getOptimizedPose(opti_file, cap_pos_num, numOfCams);
    for (int i = 0; i < numOfCams; ++i) {
        parsed_poses[i].orientation = navVis_rgbCamOrient_to_Opengl(parsed_poses[i].orientation);
        parsed_poses[i].position = navVis_pos_to_Opengl(parsed_poses[i].position);

        if (optimization_exists) {
            out_cams->darius_optimized_pose_present = true;
            out_cams->cams[i].optimized_pose.orientation = navVis_rgbCamOrient_to_Opengl(optimized_pose[i].orientation);
            out_cams->cams[i].optimized_pose.position = navVis_pos_to_Opengl(optimized_pose[i].position);

        }
        else {

        }

        out_cams->cams[i].pose = parsed_poses[i];


    }
    if(optimization_exists)     opti_file.close();

    
    return out_cams;
}
