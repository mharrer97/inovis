#include "transfer_from_python_utils.h"
#include <torch/jit.h>
#include <torch/script.h>


std::vector<torch::Tensor> from_pytorch::read_tensors_from_container_file(std::string& path, std::vector<std::string>& container_item_strings, bool no_debug_print) {
	assert(container_item_strings.size() > 0 && "No Items Given");
	try {

		torch::jit::script::Module tensor = torch::jit::load(path);

		std::vector<torch::Tensor > tensors;
		if (!no_debug_print) std::cout << "Loading tensor file: " << path << std::endl;

		for (auto& st : container_item_strings) {
			torch::Tensor container_tensor_ = tensor.attr(st).toTensor();
			torch::Tensor container_tensor = container_tensor_.clone();
			tensors.push_back(container_tensor);
			if (!no_debug_print)
				std::cout << "load tensor: " << path + "-" + st
				<< ": type " << container_tensor.type()
				<< ", min " << container_tensor.min().item<float>()
				<< ", max " << container_tensor.max().item<float>() 
				<< ", shape: " << container_tensor.sizes() << std::endl;

		}
		return tensors;
	}
	catch (const c10::Error& e) {
		std::cerr << "error loading tensors\n" << e.msg() << std::endl;;
		exit(-1);
	}
}






std::vector<torch::Tensor> from_pytorch::read_iteratively_named_tensors_from_container_file(std::string& path, bool no_debug_print) {
	try {

		torch::jit::script::Module tensor = torch::jit::load(path);

		std::vector<torch::Tensor > tensors;
		if (!no_debug_print) std::cout << "Loading tensor file: " << path << std::endl;

		bool running = true;
		int index_nr = 0;
		while (running) {
			std::string str = std::to_string(index_nr);
			torch::Tensor container_tensor_;
			try {
				container_tensor_ = tensor.attr(str).toTensor();
			}
			catch( const torch::jit::ObjectAttributeError e){
				std::cout << "Loaded " << index_nr << " tensors from " << path << std::endl;
				running = false;
				break;
			}
			torch::Tensor container_tensor = container_tensor_.clone();
			tensors.push_back(container_tensor);
			if (!no_debug_print)
				std::cout << "load tensor: " << path + "-" + str
				<< ": type " << container_tensor.type()
				<< ", min " << container_tensor.min().item<float>()
				<< ", max " << container_tensor.max().item<float>()
				<< ", shape: " << container_tensor.sizes() << std::endl;

			++index_nr;
		}
		return tensors;
	}
	catch (const c10::Error& e) {
		std::cerr << "error loading tensors\n" << e.msg() << std::endl;;
		exit(-1);
	}
}