#include "stdio.h"
#include <cppgl.h>
#include <iostream>
#include <utility>
#include "renderer_util.h"

#include <torch/torch.h>

int copy_texture(unsigned int tex1, unsigned int tex2,  size_t width, size_t height);

int texture_to_tensor(unsigned int tex,  torch::Tensor& tensor,  size_t height, size_t width);
int tensor_to_texture(unsigned int tex, torch::Tensor& tensor, size_t height, size_t width);
int texture_to_texture(GLuint tex1, GLuint tex2, size_t height, size_t width);

std::pair<torch::ScalarType, int> get_type_from_texture2D(Texture2D tex);

//output: CxHxW tensor (y dimension is flipped afterwords to convert from opengl)
torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width, int channels, bool overwrite_type, torch::ScalarType type, int type_size);
//output: CxHxW tensor (y dimension is flipped afterwords to convert from opengl)
torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width, int channels);
//output: CxHxW tensor (y dimension is flipped afterwords to convert from opengl)
torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width);


//input: CxHxW tensor, channels should not be three, will internally flip y to convert to opengl
void tensor_to_texture2D(torch::Tensor tensor, Texture2D tex, int height, int width, bool ignore_type_check);
