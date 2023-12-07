#pragma once
#include <cppgl.h>
#include "platform_adv.h"

//wrapper class to create and store a bindless handle
//careful: do not add mipmap level to Texture, as it resizes 
//	and moves the texture, thus invalidating the handle
class BindlessTextureImpl {
public: //data
	std::string name;
	Texture2D tex;
	uint64_t handle = 0;

public: //methods
	BindlessTextureImpl(std::string name);
	BindlessTextureImpl(std::string name, Texture2D tex);
	~BindlessTextureImpl();

	uint64_t getHandle();
	void wrapTextureWithHandle(Texture2D tex);

	// prevent copies and moves, since GL buffers aren't reference counted
	BindlessTextureImpl(const BindlessTextureImpl&) = delete;
	BindlessTextureImpl& operator=(const BindlessTextureImpl&) = delete;
	BindlessTextureImpl& operator=(const BindlessTextureImpl&&) = delete;
};

using BindlessTexture = NamedHandle<BindlessTextureImpl>;
template class _API_ADV NamedHandle<BindlessTextureImpl>; //needed for Windows DLL export
