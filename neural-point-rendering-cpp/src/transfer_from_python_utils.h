#pragma once
#include <torch/torch.h>

namespace from_pytorch {
	std::vector<torch::Tensor> read_tensors_from_container_file(std::string& path, std::vector<std::string>& container_item_strings, bool no_debug_print=false);

	std::vector<torch::Tensor> read_iteratively_named_tensors_from_container_file(std::string& path, bool no_debug_print=false);
}