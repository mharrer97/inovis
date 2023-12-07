#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

// ----------------------------
// Definition

class CamPathRenderer {
public:
    void draw_path(){
        draw_internal();
    }

    CamPathRenderer(){
        float points[6] = {0.f,0.f,0.f,1.f,1.f,1.f};

        //for(int i = 0; i<24; ++i) std::cout << frustum[i] << std::endl;
        uint32_t idx[20] = {0, 1}; //67
        //uint32_t idx[2] = {0, 0,
        //                           }; //67
        //std::cout << "f vao before " << f_vao << std::endl;

        glGenVertexArrays(1, &f_vao);
        glBindVertexArray(f_vao);
        //std::cout << "f vao after " << f_vao << std::endl;
        glGenBuffers(1, &f_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, f_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
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

    void set_path(std::vector<std::pair<glm::vec3, glm::quat>> & path){
        std::vector<float> data_buf;
        std::vector<uint32_t> idx;
        for(auto& pose : path){
            data_buf.push_back(pose.first.x);
            data_buf.push_back(pose.first.y);
            data_buf.push_back(pose.first.z);
            idx.push_back(idx.size());
        }
//        for(int i=0; i<(data_buf.size()/3)-1; ++i){
//            idx.push_back(i);
//            idx.push_back(i+1);
//        }
        idx_size = idx.size();
//        std::cout << "path_length: " << path.size() << ", data_length: " << data_buf.size() << ", idx length: " << idx.size() << std::endl;

        glBindVertexArray(f_vao);
        //std::cout << "f vao after " << f_vao << std::endl;
        glBindBuffer(GL_ARRAY_BUFFER, f_vbo);
        glBufferData(GL_ARRAY_BUFFER, data_buf.size() * sizeof(float), data_buf.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint32_t), idx.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    ~CamPathRenderer(){
        glDeleteVertexArrays(1, &f_vao);
        glDeleteBuffers(1, &f_ibo);
        glDeleteBuffers(1, &f_vbo);
    }
private:

    inline void draw_internal(){
        //std::cout << "f vao render " << vao << std::endl;
        glBindVertexArray(f_vao);
        //glPointSize(5);
        glDrawElements( GL_LINE_STRIP, idx_size, GL_UNSIGNED_INT, 0);
        //glPointSize(1);
        glBindVertexArray(0);
    }

    uint32_t idx_size = 0;
    GLuint f_vao;
    GLuint f_vbo;
    GLuint f_ibo;
};
