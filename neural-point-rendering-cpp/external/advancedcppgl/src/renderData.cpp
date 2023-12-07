#include "renderData.h"
#include "bindlessTexture.h"

MaterialData RenderData::createMaterialData(Material mat) {
	MaterialData newMat;

	newMat.kDiffuse = mat->vec3_map.at("ambient_color");
	newMat.kSpecular = mat->vec3_map.at("diffuse_color");
	newMat.opacity = mat->float_map.at("opacity");
	newMat.roughness = mat->float_map.at("roughness");
	newMat.texDiff = 0;
	newMat.texSpec = 0;
	newMat.texAlpha = 0;
	newMat.texNormal = 0;

	if (mat->has_texture("diffuse") ){
		BindlessTexture diff = !BindlessTexture::valid(mat->name + "_diffuse_bindless") ?			
			BindlessTexture(mat->name + "_diffuse_bindless", mat->get_texture("diffuse")):
			BindlessTexture::find(mat->name + "_diffuse_bindless");
		newMat.texDiff = diff->getHandle();
	}
	if (mat->has_texture("specular") ) {
		BindlessTexture spec = !BindlessTexture::valid(mat->name + "_specular_bindless") ? 
			BindlessTexture(mat->name + "_specular_bindless", mat->get_texture("specular")) :
			BindlessTexture::find(mat->name + "_specular_bindless");
		newMat.texSpec = spec->getHandle();
	}
	if (mat->has_texture("alphamap") ) {
		BindlessTexture alpha = !BindlessTexture::valid(mat->name + "_alphamap_bindless") ? 
			BindlessTexture(mat->name + "_alphamap_bindless", mat->get_texture("alphamap")) :
			BindlessTexture::find(mat->name + "_alphamap_bindless");
		newMat.texAlpha = alpha->getHandle();
	}
	if (mat->has_texture("normalmap")) {
		BindlessTexture normal = !BindlessTexture::valid(mat->name + "_normalmap_bindless") ? 
			BindlessTexture(mat->name + "_normalmap_bindless", mat->get_texture("normalmap")) :
			BindlessTexture::find(mat->name + "_normalmap_bindless");
		newMat.texNormal = normal->getHandle();
	}

	return newMat;
}
