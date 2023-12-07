#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

// ----------------------------
// Definition

class Frustum {
public:
    void draw_frustum(){
        draw_internal();
    }

    Frustum(const mat4& proj, float farPlaneDistance){
        float d = 1.0f;
        glm::vec4 bl(-1, -1, d, 1);
        glm::vec4 br(1, -1, d, 1);
        glm::vec4 tl(-1, 1, d, 1);
        glm::vec4 tr(1, 1, d, 1);

        mat4 projInv = glm::inverse(proj);



        tl = projInv * tl;
        tr = projInv * tr;
        bl = projInv * bl;
        br = projInv * br;

        tl /= tl[3];
        tr /= tr[3];
        bl /= bl[3];
        br /= br[3];

        if (farPlaneDistance > 0)
        {
            tl[3] = -tl[2] / farPlaneDistance;
            tr[3] = -tr[2] / farPlaneDistance;
            bl[3] = -bl[2] / farPlaneDistance;
            br[3] = -br[2] / farPlaneDistance;

            tl /= tl[3];
            tr /= tr[3];
            bl /= bl[3];
            br /= br[3];
        }


        //    std::vector<VertexNC> vertices;

        glm::vec4 positions[] = {glm::vec4(0, 0, 0, 1),
                                 tl,
                                 tr,
                                 br,
                                 bl,
                                 0.4f * tl + 0.6f * tr,
                                 0.6f * tl + 0.4f * tr,
                                 0.5f * tl + 0.5f * tr + glm::vec4(0, (tl[1] - bl[1]) * 0.1f, 0, 0)};

        float frustum[24];
        for (int i = 0; i < 8; ++i) {
            glm::vec4 v = positions[i];
            frustum[i*3 + 0] = v[0];
            frustum[i*3 + 1] = v[1];
            frustum[i*3 + 2] = v[2];
            std::cout << v << std::endl;
        }
        //for(int i = 0; i<24; ++i) std::cout << frustum[i] << std::endl;
        uint32_t idx[20] = {0, 1,
                           0, 2,
                           0, 3,
                           0, 4,
                           1, 2, //12
                           3, 4, //34
                           1, 4, //14
                           2, 3, //23
                           5, 7, //57
                           6, 7}; //67

        //uint32_t idx[2] = {0, 0,
        //                           }; //67
        //std::cout << "f vao before " << f_vao << std::endl;

        glGenVertexArrays(1, &f_vao);
        glBindVertexArray(f_vao);
        //std::cout << "f vao after " << f_vao << std::endl;
        glGenBuffers(1, &f_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, f_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(frustum), frustum, GL_STATIC_DRAW);
        glGenBuffers(1, &f_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        /*glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(sizeof(float)*3));*/
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void frustum_data(const mat4& proj, float farPlaneDistance){
        float d = 1.0f;
        glm::vec4 bl(-1, -1, d, 1);
        glm::vec4 br(1, -1, d, 1);
        glm::vec4 tl(-1, 1, d, 1);
        glm::vec4 tr(1, 1, d, 1);

        mat4 projInv = glm::inverse(proj);



        tl = projInv * tl;
        tr = projInv * tr;
        bl = projInv * bl;
        br = projInv * br;

        tl /= tl[3];
        tr /= tr[3];
        bl /= bl[3];
        br /= br[3];

        if (farPlaneDistance > 0)
        {
            tl[3] = -tl[2] / farPlaneDistance;
            tr[3] = -tr[2] / farPlaneDistance;
            bl[3] = -bl[2] / farPlaneDistance;
            br[3] = -br[2] / farPlaneDistance;

            tl /= tl[3];
            tr /= tr[3];
            bl /= bl[3];
            br /= br[3];
        }


        //    std::vector<VertexNC> vertices;

        glm::vec4 positions[] = {glm::vec4(0, 0, 0, 1),
                                 tl,
                                 tr,
                                 br,
                                 bl,
                                 0.4f * tl + 0.6f * tr,
                                 0.6f * tl + 0.4f * tr,
                                 0.5f * tl + 0.5f * tr + glm::vec4(0, (tl[1] - bl[1]) * 0.1f, 0, 0)};

        float frustum[24];
        for (int i = 0; i < 8; ++i) {
            glm::vec4 v = positions[i];
            frustum[i*3 + 0] = v[0];
            frustum[i*3 + 1] = v[1];
            frustum[i*3 + 2] = v[2];
            //std::cout << v << std::endl;
        }
        //for(int i = 0; i<24; ++i) std::cout << frustum[i] << std::endl;
        uint32_t idx[20] = {0, 1,
                            0, 2,
                            0, 3,
                            0, 4,
                            1, 2, //12
                            3, 4, //34
                            1, 4, //14
                            2, 3, //23
                            5, 7, //57
                            6, 7}; //67

        //uint32_t idx[2] = {0, 0,
        //                           }; //67
        //std::cout << "f vao before " << f_vao << std::endl;

        glBindVertexArray(f_vao);
        //std::cout << "f vao after " << f_vao << std::endl;
        glBindBuffer(GL_ARRAY_BUFFER, f_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(frustum), frustum, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    ~Frustum(){
        glDeleteVertexArrays(1, &f_vao);
        glDeleteBuffers(1, &f_ibo);
        glDeleteBuffers(1, &f_vbo);
    }
private:

    inline void draw_internal(){
        //std::cout << "f vao render " << vao << std::endl;
        glBindVertexArray(f_vao);
        //glPointSize(5);
        glDrawElements( GL_LINES, 20, GL_UNSIGNED_INT, 0);
        //glPointSize(1);
        glBindVertexArray(0);
    }

    GLuint f_vao;
    GLuint f_vbo;
    GLuint f_ibo;
};
