#include "texture_copy.h"
#include <cuda_gl_interop.h>
#include <torch/torch.h>



cudaGraphicsResource* res1;
cudaGraphicsResource* res2;

cudaArray_t array1;
cudaArray_t array2;



void CHECK_CUDA(cudaError_t err) {
	if (err != cudaSuccess) {
		std::cerr << "Error: " << cudaGetErrorString(err) << std::endl;
		exit(-1);
	}
}

//#include "tensor_utils.h"



int texture_to_tensor(GLuint tex, torch::Tensor& tensor, size_t height, size_t width) {
	//cout << "Copying tensor into texture ..." << endl;

	cudaGraphicsResource_t res; cudaArray_t array;

	CHECK_CUDA(cudaSetDevice(0));

	//cout << "Device set" << endl;

	// Register Texture
	CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsNone));

	//cout << "Registered Images" << endl;

	// Map Texture
	CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
	CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, res, 0, 0));
	cudaDeviceSynchronize();
	//cout << "Mapped Images" << endl;

	//  CHECK_CUDA(cudaGraphicsResourceGetMappedPointer((void**)&data_ptr, &size, res));


	CHECK_CUDA(cudaMemcpy2DFromArray(tensor.data_ptr(), width, array, 0, 0, width, height, cudaMemcpyDeviceToDevice));
	// auto tensor2 = torch::from_blob(array, tensor.sizes(), torch::TensorOptions().device(torch::kCUDA).dtype(tensor.dtype()));
	 //tensor = tensor.contiguous(c10::MemoryFormat::ChannelsLast);
	 //cudaDeviceSynchronize();
	// using namespace torch::indexing;

	// tensor2 = tensor2.contiguous().clone().contiguous().permute({ 2,1,0 }).contiguous();

	// tensor_utils::screenshot_tensor_to_path(tensor2.index({ Slice(0,3),Slice(),Slice() }).contiguous(), "./tex_to_tensor-org.png");

	 // CleanUp
	CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
	CHECK_CUDA(cudaGraphicsUnregisterResource(res));

	return 0;
}
using namespace std;


int tensor_to_texture(GLuint tex, torch::Tensor& tensor, size_t height, size_t width) {
	//cout << "Copying tensor into texture ..." << endl;

	cudaGraphicsResource* res; cudaArray_t array;

	CHECK_CUDA(cudaSetDevice(0));

	//cout << "Device set" << endl;

	// Register Texture
	CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard));

	//cout << "Registered Images" << endl;

	// Map Texture
	CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
	CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, res, 0, 0));

	//cout << "Mapped Images" << endl;
	CHECK_CUDA(cudaMemcpy2DToArray(array, 0, 0, tensor.data_ptr(), width, width, height, cudaMemcpyDeviceToDevice));

	// CleanUp
	CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
	CHECK_CUDA(cudaGraphicsUnregisterResource(res));

	return 0;
}

int texture_to_texture(GLuint tex1, GLuint tex2, size_t height, size_t width) {
	glCopyImageSubData(tex1, GL_TEXTURE_2D, 0, 0, 0, 0, tex2, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
	return 0;
}

pair<torch::ScalarType, int> get_type_from_texture2D(Texture2D tex) {
	// GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_HALF_FLOAT, GL_FLOAT
	// dtype: kUInt8, kInt8, kInt16, kInt32, kInt64, kFloat32

	torch::ScalarType d_t = torch::kFloat32;
	int type_size = 4;
	switch (tex->type) {
	case GL_UNSIGNED_BYTE:
		d_t = torch::kUInt8;
		type_size = 1;
		break;
	case GL_BYTE:
		d_t = torch::kInt8;
		type_size = 1;
		break;
	case GL_UNSIGNED_SHORT:
		std::cerr << "Using kInt16 for GL_UNSIGNED_SHORT. This may lead to errors." << std::endl;
		d_t = torch::kInt16;
		type_size = 2;
		break;
	case GL_SHORT:
		d_t = torch::kInt16;
		type_size = 2;
		break;
	case GL_UNSIGNED_INT:
		std::cerr << "Using kInt32 for GL_UNSIGNED_INT. This may lead to errors." << std::endl;
		d_t = torch::kInt32;
		type_size = 4;
		break;
	case GL_INT:
		d_t = torch::kInt32;
		type_size = 4;
		break;
	case GL_HALF_FLOAT:
		d_t = torch::kFloat16;
		type_size = 2;
		break;
	case GL_FLOAT:
		d_t = torch::kFloat32;
		type_size = 4;
		break;

	default:
		std::cerr << "Warning: Texture uses unknown type with id " << tex->type << std::endl;
		std::cerr << "Using kFloat32 as type. This may lead to errors." << std::endl;
	}

	return make_pair(d_t, type_size);
}

torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width, int channels, bool overwrite_type, torch::ScalarType type, int type_size) {
	using namespace torch::indexing;
	// Determine parameters
	int h_tensor = height <= 0 ? tex->h : height;
	int w_tensor = width <= 0 ? tex->w : width;


	// Determine channels from format
	GLint data;

	int c = tex->format == GL_RGBA ? 4 : tex->format == GL_RGB ? 3 : tex->format == GL_RG ? 2 : 1;
	int c_tensor = c == 3 ? 4 : c; // Padding of vec3 to vec4
	if (channels > 0) {
		c_tensor = channels;
	}


	pair<torch::ScalarType, int> type_info = get_type_from_texture2D(tex);
	torch::ScalarType d_t = type_info.first;
	int type_size_used = type_info.second;

	if (overwrite_type) {
		d_t = type;
		type_size_used = type_size;
	}
	//std::cout << "transform texture of size [" << h_tensor << ", " << w_tensor << ", " << c_tensor << "]"<<std::endl;

	torch::Tensor tensor = torch::zeros({ h_tensor,w_tensor, c_tensor }, torch::TensorOptions().device(torch::kCUDA).dtype(d_t));
	texture_to_tensor(tex->id, tensor, (size_t)min(tex->h, h_tensor), (size_t)min(tex->w, w_tensor) * type_size_used * c_tensor);

	if (c_tensor != c) {
		tensor = tensor.index({ Slice(), Slice(), Slice(0, c) });
	}

	//permute to CxHxW and flip H dimension
	//print_tensor("tex2tensor direct output", tensor);
	tensor = tensor.permute({ 2,0,1 });
	tensor = tensor.flip({ 1 });
	//print_tensor("tex2tensor permuted output", tensor);

	return tensor;
}

torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width, int channels) {
	return texture2D_to_tensor(tex, height, width, channels, false, torch::kFloat32, -1);
}

torch::Tensor texture2D_to_tensor(Texture2D tex, int height, int width) {
	return texture2D_to_tensor(tex, height, width, -1, false, torch::kFloat32, -1);
}

// wxhx3
void tensor_to_texture2D(torch::Tensor t, Texture2D tex, int height, int width, bool ignore_type_check) {
	// Check dimensions of tensor
	int tensor_dim = t.dim();
	if (tensor_dim != 3) {
		cerr << "Tensor has 4 dimensions, but expected 3 dimensions [w, h, c]" << endl;
		cerr << "Aborting copy into: " << tex->name << endl;
		return;

	}

	if (!t.device().is_cuda()) {
		cerr << "Tensor is not on the GPU, please use .to(torch::kCUDA)" << std::endl;
		return;

	}
	//transfer to opengl format
	auto tensor = t.flip({ 1 });
	// tensor  should be	c x h x w
	// texture should be	w x h x c
	tensor = tensor.permute({ 1,2,0 });
	//tensor = tensor.flip({ 1 });
	tensor = tensor.contiguous();
	//std::cout << tensor.sizes() << std::endl;
	//print_tensor("tensor2tex pre conversion", tensor);
	if (tex->format == GL_RGB) {
		cerr << "Texture is of RGB format. This Format is not supported" << endl;
		cerr << "Aborting copy." << endl;
		return;

	}

	int width_tensor = tensor.size(1);
	int height_tensor = tensor.size(0);
	int c_tensor = tensor.size(2);

	if (c_tensor * width_tensor != tensor.stride(0) || c_tensor != tensor.stride(1) || 1 != tensor.stride(2)) {
		cerr << "Strides are not correctly aligned. Please call .contiguous() on the tensor." << endl;
		cerr << "S1: " << tensor.stride(0) << " , S2: " << tensor.stride(1) << ", S3: " << tensor.stride(2) << endl;

		cerr << "H1: " << c_tensor * height_tensor << ", H2 : " << c_tensor  << endl;
		cerr << "Aborting copy into: "<< tex->name << endl;
		return;
	}

	// Take height/width


	int width_used = max(0, min(width, min(tex->w, width_tensor)));
	int height_used = max(0, min(height, min(tex->h, height_tensor)));

	// Warnings if width/height was greater than width/height of tensor/texture;

	// Check channels
	int c = (tex->format == GL_RGBA) ? 4 : (tex->format == GL_RGB ? 3 : (tex->format == GL_RG ? 2 : 1));
	if (c != c_tensor) {
		cerr << "Number of channels in tensor is not equal to the number of channels in texture." << endl;
		cerr << "Aborting copy into: " << tex->name << endl;
		cerr << c << " != " << c_tensor << endl;

		return;
	}


	// Check types
	pair<torch::ScalarType, int> type_info = get_type_from_texture2D(tex);
	torch::ScalarType texture_type = type_info.first;
	torch::ScalarType tensor_type = tensor.scalar_type();
	int type_size = type_info.second;

	if (!ignore_type_check && texture_type != tensor_type) {
		cerr << "Types of tensor and texture do not match." << endl;
		cerr << "Aborting copy into: " << tex->name << endl;
	}

	//std::cout << "transformed to texture of size [" << height_used << ", " << width_used << ", " << c_tensor << "]" << std::endl;
	tensor_to_texture(tex->id, tensor, (size_t)height_used, (size_t)width_used * type_size * c_tensor);
}
