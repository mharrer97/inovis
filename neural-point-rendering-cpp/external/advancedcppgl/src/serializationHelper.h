#pragma once
#include <cppgl.h>
#include <cereal/archives/portable_binary.hpp>
#include "glm/glm.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>

namespace glm
{

	template<class Archive> void serialize(Archive& archive, glm::vec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::vec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::vec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::ivec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::ivec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::ivec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::uvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::uvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::uvec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::dvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::dvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::dvec4& v) { archive(v.x, v.y, v.z, v.w); }

	// glm matrices serialization
	template<class Archive> void serialize(Archive& archive, glm::mat2& m) { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, glm::dmat2& m) { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, glm::mat3& m) { archive(m[0], m[1], m[2]); }
	template<class Archive> void serialize(Archive& archive, glm::mat4& m) { archive(m[0], m[1], m[2], m[3]); }
	template<class Archive> void serialize(Archive& archive, glm::dmat4& m) { archive(m[0], m[1], m[2], m[3]); }

	template<class Archive> void serialize(Archive& archive, glm::quat& q) { archive(q.x, q.y, q.z, q.w); }
	template<class Archive> void serialize(Archive& archive, glm::dquat& q) { archive(q.x, q.y, q.z, q.w); }
}

//namespace cereal
//{
//
//	template<class Archive, class F, class S>
//	void save(Archive& ar, const std::pair<F, S>& pair)
//	{
//		ar(pair.first, pair.second);
//	}
//	template<class Archive, class F, class S>
//	void load(Archive& ar, const std::pair<F, S>& pair)
//	{
//
//		ar(pair.first, pair.second);
//	}
//	template<class Archive, class F, class S>
//	void serialize(Archive& ar, const std::pair<F, S>& pair)
//	{
//
//		ar(pair.first, pair.second);
//	}
//	template <class Archive, class F, class S>
//	struct specialize<Archive, std::pair<F, S>, cereal::specialization::non_member_load_save> {};
//}

//template<class Archive, class Impl>
//void serialize(Archive& archive,
//	NamedHandle<Impl>& m)
//{
//	archive(m->ptr);
//}

//namespace cereal
//{
//	template <> struct LoadAndConstruct<Geometry>
//	{
//		template <class Archive>
//		static void load_and_construct(Archive& ar, cereal::construct<Geometry>& construct)
//		{
//
//			std::string name;
//			glm::vec3 bbmin;
//			glm::vec3 bbmax;
//			std::vector<glm::vec3> pos;
//			std::vector<uint32_t> ind;
//			std::vector<glm::vec3> norms;
//			std::vector<glm::vec2> tc;
//
//			ar(name, bbmin, bbmax, pos, ind, tc);
//			construct(name,pos,ind,norms,tc);
//		}
//
//	};
//}
//
//template <class Archive>
//static void load_and_construct(Archive& ar, cereal::construct<Geometry>& construct)
//{
//
//	std::string name;
//	glm::vec3 bbmin;
//	glm::vec3 bbmax;
//	std::vector<glm::vec3> pos;
//	std::vector<uint32_t> ind;
//	std::vector<glm::vec3> norms;
//	std::vector<glm::vec2> tc;
//
//	ar(name, bbmin, bbmax, pos, ind, tc);
//	construct(name, pos, ind, norms, tc);
//}


template<class Archive>
void save(Archive& archive,
	const Geometry& m)
{
	archive(m->name, m->bb_min, m->bb_max, m->positions, m->indices, m->normals, m->texcoords);

}

template<class Archive>
void load(Archive& archive,
	Geometry& m)
{
	//	archive(m->name, m->bb_min, m->bb_max, m->positions, m->indices, m->normals, m->texcoords);
	std::string name;
	glm::vec3 bbmin;
	glm::vec3 bbmax;
	std::vector<glm::vec3> pos;
	std::vector<uint32_t> ind;
	std::vector<glm::vec3> norms;
	std::vector<glm::vec2> tc;

	archive(name, bbmin, bbmax, pos, ind, norms, tc);

	m.ptr = std::make_shared<GeometryImpl>(name, pos, ind, norms, tc);
	Geometry::map[name] = m;
}
//template<class Archive>
//void load(Archive& archive,
//	const Geometry& m)
//{
//	//archive(m->name, m->bb_min, m->bb_max, m->positions, m->indices, m->normals, m->texcoords);
//	//std::string x;
//	//glm::vec3 bbmin;
//	//glm::vec3 bbmax;
//	//std::vector<glm::vec3> pos;
//	//std::vector<uint32_t> ind;
//	//std::vector<glm::vec2> tc;
//	//archive(x, bbmin,bbmax,pos,ind,tc);
//
//	archive(m.ptr);
//
//}


template<class Archive>
void save(Archive& archive,
	const Material& m)
{
	archive(m->name, m->int_map, m->float_map, m->vec2_map, m->vec3_map, m->vec4_map, m->texture_map);
}

template<class Archive>
void load(Archive& archive,
	Material& m)
{
	std::string name;
	std::map<std::string, int> int_map;
	std::map<std::string, float> float_map;
	std::map<std::string, glm::vec2> vec2_map;
	std::map<std::string, glm::vec3> vec3_map;
	std::map<std::string, glm::vec4> vec4_map;
	std::map<std::string, Texture2D> texture_map;
	archive(name, int_map, float_map, vec2_map, vec3_map, vec4_map, texture_map);

	m.ptr = std::make_shared<MaterialImpl>(name);
	Material::map[name] = m;

	m->int_map = int_map;
	m->float_map = float_map;
	m->vec2_map = vec2_map;
	m->vec3_map = vec3_map;
	m->vec4_map = vec4_map;
	m->texture_map = texture_map;

}

// ----------------------------------------------------
// helper funcs

inline uint32_t format_to_channels(GLint format) {
	return format == GL_RGBA ? 4 : format == GL_RGB ? 3 : format == GL_RG ? 2 : 1;
}
inline GLint channels_to_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : channels == 2 ? GL_RG : GL_RED;
}
inline GLint channels_to_float_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
}
inline GLint channels_to_ubyte_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
}

inline GLint type_to_bytes(GLint type) {
	return type == GL_FLOAT ? 4 : 1;
}

template<class Archive>
void save(Archive& archive,
	const Texture2D m)
{
	bool isMipmap = false;
	m->bind(0);

	GLfloat param;
	glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &param);
	if (param == GL_LINEAR_MIPMAP_LINEAR) isMipmap = true;

	//serialize gpu data only if texture was created, else write harddrive path
	std::vector<uint8_t> data;
	if (m->loaded_from_path.empty()) {
		data.reserve(m->w * m->h * format_to_channels(m->format) * type_to_bytes(m->type));
		glGetTexImage(GL_TEXTURE_2D, 0, m->format, m->type, (void*)data.data());
	}
	else {
		data.push_back(0);
	}

	m->unbind();
	archive(m->name, m->w, m->h, m->internal_format, m->format, m->type, isMipmap, m->loaded_from_path.string(), data);
}

template<class Archive>
void load(Archive& archive,
	Texture2D m)
{
	std::string name;
	int w, h;
	GLint internal_format;
	GLenum format, type;
	bool isMipmap;
	std::string path;
	std::vector<uint8_t> data;

	archive(name, w, h, internal_format, format, type, isMipmap, path, data);

	if (Texture2D::find("name")) {
		m = Texture2D::find("name");
		return;
	}
	if (path.empty()) {
		m.ptr = std::make_shared<Texture2DImpl>(name, w, h, internal_format, format, type, data.data(), isMipmap);
	}
	else {
		const fs::path path_fs(path);
		m.ptr = std::make_shared<Texture2DImpl>(name, path_fs, isMipmap);
	}
	Texture2D::map[name] = m;

}
