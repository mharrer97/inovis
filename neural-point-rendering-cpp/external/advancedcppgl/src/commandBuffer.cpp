#include "commandBuffer.h"

DrawCommandBufferImpl::DrawCommandBufferImpl(std::string name):name(name) {
	frontBuffer = DIBO(name + "_frontDIBO"); 
	backBuffer = DIBO(name + "_backDIBO");
	originalBuffer = DIBO(name + "_originalDIBO");
	cpu_shadow_buffer.clear();
}

DrawCommandBufferImpl::~DrawCommandBufferImpl() {
	cpu_shadow_buffer.clear();
}

void DrawCommandBufferImpl::addElements(std::vector<DrawElementsIndirectCommand>& elements) {
	cpu_shadow_buffer.insert(std::end(cpu_shadow_buffer), std::begin(elements), std::end(elements));

	size_t new_buffer_size = cpu_shadow_buffer.size() * sizeof(DrawElementsIndirectCommand);

	//add to all buffers
	for (auto commandBuffer : { frontBuffer,backBuffer,originalBuffer }) {
		commandBuffer->resize(new_buffer_size, GL_DYNAMIC_DRAW);

		commandBuffer->bind();
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * cpu_shadow_buffer.size(), cpu_shadow_buffer.data(), GL_DYNAMIC_DRAW);
		commandBuffer->unbind();
	}
}

void DrawCommandBufferImpl::clearElements() {
	size_t new_buffer_size = 0;

	//clear all buffers
	for (auto commandBuffer : { frontBuffer,backBuffer,originalBuffer }) {
		commandBuffer->resize(new_buffer_size);
	}
	cpu_shadow_buffer.clear();
}


void DrawCommandBufferImpl::swap(){
	frontBackBool = !frontBackBool;
	resetToOriginal = false;
}

void DrawCommandBufferImpl::reset(){
	frontBackBool = true;
	resetToOriginal = true;
}

DIBO DrawCommandBufferImpl::getFrontBuffer(){
	DIBO res = resetToOriginal ? originalBuffer :
		(frontBackBool ? frontBuffer : backBuffer);
	return res;
}

DIBO DrawCommandBufferImpl::getBackBuffer(){
	DIBO res = frontBackBool ? backBuffer : frontBuffer;
	return res;
}

size_t DrawCommandBufferImpl::getSize() {
	return cpu_shadow_buffer.size();
}