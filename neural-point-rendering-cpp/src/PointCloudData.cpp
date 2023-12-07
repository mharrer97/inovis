#include "PointCloudData.h"

std::ostream& operator<<(std::ostream& os, const PointCloudCameraData& data)
{
    /*os << data.view_p << '\n'
        << data.x_axis << '\n'
        << data.y_axis << '\n'
        << data.z_axis << '\n'
        << data.focal << '\n'
        << data.scale << '\n'
        << data.center << '\n'
        << data.viewport << '\n'
        << data.k << std::endl;*/
    os << "view_p\t[" << data.view_p.x << "," << data.view_p.y << "," << data.view_p.z << "]" << '\n'
        << "x_axis\t[" << data.x_axis.x << "," << data.x_axis.y << "," << data.x_axis.z << "]" << '\n'
        << "y_axis\t[" << data.y_axis.x << "," << data.y_axis.y << "," << data.y_axis.z << "]" << '\n'
        << "z_axis\t[" << data.z_axis.x << "," << data.z_axis.y << "," << data.z_axis.z << "]" << '\n'
        << "focal\t" << data.focal << '\n'
        << "scale\t[" << data.scale.x << "," << data.scale.y  << "]" << '\n'
        << "center\t[" << data.center.x << "," << data.center.y << "]" << '\n'
        << "viewport\t[" << data.viewport.x << "," << data.viewport.y << "]" << '\n'
        << "k\t[" << data.k.x << "," << data.k.y << "]" <<  std::endl;
    return os;
};

std::ostream& operator<<(std::ostream& os, const PointCloudPoint& data)
{
    /* os << data.pos << '\n'
        << data.color << '\n'
        << data.normal << '\n'
        << data.curvature << std::endl;*/
    os << "pos\t[" << data.pos.x << "," << data.pos.y << "," << data.pos.z << "]" << '\n'
        << "color\t[" << data.color.x << "," << data.color.y << "," << data.color.z << "]" << '\n'
        << "normal\t[" << data.normal.x << "," << data.normal.y << "," << data.normal.z << "]" << '\n'
        << "curvature\t" << data.curvature << '\n'
        << "timestamp\t" << data.timestamp << std::endl;
    return os;
};

