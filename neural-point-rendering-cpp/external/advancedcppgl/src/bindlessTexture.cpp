#include "bindlessTexture.h"

BindlessTextureImpl::BindlessTextureImpl(std::string name): name(name) {

}

BindlessTextureImpl::BindlessTextureImpl(std::string name,Texture2D tex): name(name), tex(tex) {
	handle = glGetTextureHandleARB(tex->id);
	glMakeTextureHandleResidentARB(handle);
}

BindlessTextureImpl::~BindlessTextureImpl() {
	//clear previous handle
	if (handle != 0)
		glMakeTextureHandleNonResidentARB(handle);
}

uint64_t BindlessTextureImpl::getHandle() {
	if (handle == 0)
		throw std::runtime_error("handle not initialized");
	return handle;
}

void BindlessTextureImpl::wrapTextureWithHandle(Texture2D tex) {
	//clear previous handle
	if(handle!=0)
		glMakeTextureHandleNonResidentARB(handle);

	//make handle
	handle = glGetTextureHandleARB(tex->id);
	glMakeTextureHandleResidentARB(handle);
}
