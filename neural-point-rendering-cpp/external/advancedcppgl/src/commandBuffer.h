#pragma once
#include <cppgl.h>
#include "shaderStructs.h"
#include "platform_adv.h"


/*  Command Buffer Wrapper: for indirect draw
	Internally represented as a flip-flop stucture with original copy.
	Common usage:
		init:
			addElements with DrawElementIndirectCommands: add vector to buffers

		render loop:
			reset(): sets original copy as front buffer
			
			Command buffer manipulation: in-front -> out-back
			swap(): swaps buffers (leaves original untouched)
			.. repeat for each manipulation

			Render with getFrontBuffer()
			
*/
class DrawCommandBufferImpl {
//structs
public: 
	//CPU struct for indirect draw buffers, defined in shaderStructs.h
	//	struct DrawElementsIndirectCommand
	//	{
	//		GLuint  count;
	//		GLuint  instanceCount;
	//		GLuint  first;
	//		GLuint  baseVertex;
	//		GLuint  baseInstance;
	//
	//	};

//data
public:
	std::string name;

	//GL_DRAW_INDIRECT_BUFFER objects
	DIBO frontBuffer;
	DIBO backBuffer;
	DIBO originalBuffer;
	std::vector<DrawElementsIndirectCommand> cpu_shadow_buffer;


	//used for flip-flop buffers
	bool frontBackBool = true;
	bool resetToOriginal = false;

//methods
public:
	DrawCommandBufferImpl(std::string name);
	virtual ~DrawCommandBufferImpl();

	void addElements(std::vector<DrawElementsIndirectCommand>& elements);
	void clearElements();

	// swap the internal front/back buffers, usually called after a culling pass
	void swap();

	// used to reset the flip-flop structure, returning the original buffer as front-buffer, 
	// usually called at the beginning of each frame if command buffer manipulation (i.e. culling) occurs 
	void reset();

	//front buffer used for rendering and input for command buffer manipulation
	DIBO getFrontBuffer();

	DIBO getBackBuffer();
	size_t getSize();

	// prevent copies and moves, since GL buffers aren't reference counted
	DrawCommandBufferImpl(const DrawCommandBufferImpl&) = delete;
	DrawCommandBufferImpl& operator=(const DrawCommandBufferImpl&) = delete;
	DrawCommandBufferImpl& operator=(const DrawCommandBufferImpl&&) = delete;

};

using DrawCommandBuffer = NamedHandle<DrawCommandBufferImpl>;
template class _API_ADV NamedHandle<DrawCommandBufferImpl>; //needed for Windows DLL export
