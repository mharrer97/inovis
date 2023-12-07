#include "plyPointCloudParser.h"

/// <summary>
/// returns the size in bytes for a given string
/// </summary>
/// <param name="t"> string containing the type of the property</param>
/// <returns></returns>
int PLYPointCloudParser::sizeoftype(std::string t) {
	if (t == "float" || t == "int") {
		return 4;
	}
	if (t == "uchar") {
		return 1;
	}
	std::cerr << "PLYPointCloudParser::sizeoftype: Invalid property type string -- not registered!: "<< t << std::endl;
	//throw std::runtime_error("PLYPointCloudParser::sizeoftype: Invalid property type string -- not registered!");
	return -1;
}


std::pair<vec3, vec3> PLYPointCloudParser::getBoundingBox() {
	constexpr float fmin = std::numeric_limits<float>::min();
	constexpr float fmax = std::numeric_limits<float>::max();
	vec3 min = vec3(fmax, fmax, fmax);
	vec3 max = vec3(fmin, fmin, fmin);
	for (int i = 0; i < points.size(); ++i) {
		// each 10000 points, give an update
		if (log && i % 1000000 == 0) {
			std::cerr << "[PLYPointCloudParser:getBoundingBox] Bounding Box Computation: " << i << " Points of " << points.size() << " done: " << (float(i) / float(points.size())) * 100 << "% \r";
			std::cerr.flush();
		}
		vec3 pos = points[i].pos;
		min.x = (pos.x < min.x) ? pos.x : min.x;
		min.y = (pos.y < min.y) ? pos.y : min.y;
		min.z = (pos.z < min.z) ? pos.z : min.z;
		max.x = (pos.x > max.x) ? pos.x : max.x;
		max.y = (pos.y > max.y) ? pos.y : max.y;
		max.z = (pos.z > max.z) ? pos.z : max.z;
	}
	return { min, max };
}
/// <summary>
/// Open PLY filer and try to parse
/// </summary>
/// <param name="file">path to ply file to be opened</param>
PLYPointCloudParser::PLYPointCloudParser(const std::string& _file, std::vector<Capture_View> _captured_views, std::string _setType, bool _log) : log(_log), setType(_setType) {
	if (log)
		std::cerr << "[PLYPointCloudParser] Start - try to open " << _file << std::endl;

	//auto file = SearchPathes::model(_file);

	if (_file == "") {
		std::cerr << "[PLYPointCloudParser] Could not open file " << _file << std::endl;
		//std::cerr << SearchPathes::model << std::endl;
		return;
	}
	if (log)
		std::cerr << "[PLYPointCloudParser] Loading " << _file << std::endl;
	std::ifstream stream(_file, std::ios::binary);

	if (!stream.is_open()) {
		//std::cerr << "Could not open file " << file << std::endl;
		throw std::runtime_error("PLYPointCloudParser::PLYPointCloudParser: invalid file: " + _file);
		return;
	}
	else {
		if (log)
			std::cerr << "[PLYPointCloudParser] File opened " << std::endl;
	}

	//// open the file:
	////         std::ifstream file(filename, std::ios::binary);


	if (log)
		std::cerr << "[PLYPointCloudParser] cast into vector<char> " << std::endl;
	// parse into char to get data byte wise
	data = std::vector<char>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

	// Print first few characters 

	//auto it = std::istreambuf_iterator<char>(stream);
	//// read the data:
	//int size = 10000;
	//data.resize(size);
	//for (int i = 0; i < size; ++i) {
	//    data[i] = *it;
	//    ++it;
	//}
	/*if (log)
		std::cerr << "[PLYPointCloudParser] Cast into string " << std::endl;
	std::string str(data.begin(), std::min(data.end(), data.begin()+size));
	if (log) {
		std::cerr << "[PLYPointCloudParser] File contains " << std::endl << str << std::endl;
		std::cerr << "[PLYPointCloudParser] Printed first " << data.size() << "characters" << std::endl;
	}*/

	// set cleared flag to false to signal that ddata has been filled
	cleared = false;
	captured_views = _captured_views;
	parseHeader();
	parsePointCloud();



	//std::cout << "[PLYLoaderPointCloud] Done.  "
	//    << "Points " << points.size() << std::endl;


	data.clear();
	if (log)
		std::cerr << "[PLYPointCloudParser] Finished " << _file << std::endl;
}

/// <summary>
/// parse the header starting at the begin of the ply file with "ply" and ending with "end_header"
/// </summary>
void PLYPointCloudParser::parseHeader() {
	if (cleared) throw std::runtime_error("[PLYPointCloudParser] ERROR: PointCloud has been cleared before parsing!");

	if (log)
		std::cerr << "[PLYPointCloudParser] Start parsing header " << std::endl;
	// header si in ascii format, begins with ply and ends with end_header
	// construct string containing the file data to find end_header keyword
	std::string str2(data.begin(), data.end());
	auto pos = str2.find("end_header");

	if (pos == std::string::npos)
		throw std::runtime_error("PLYPointCloudParser::parseHeader: Could not find end_header!");

	// set datastart index to where the header ends for pointcloud parsing
	dataStart = pos;
	// go until next newline
	while (data[dataStart] != '\n') {
		dataStart++;
	}
	dataStart++;

	//extract header data
	std::string header(str2.begin(), str2.begin() + pos); // string containing the header
	header_data = std::vector<char>(data.begin(), data.begin() + dataStart); // cast to vector<char>
	// remove '\r' from the header (shifts the remaining values and returns a past end iterator). remove the now redundant last characters
	header.erase(std::remove(header.begin(), header.end(), '\r'), header.end());

	// split headder into the lines
	std::vector<std::string> headerLines = StringHelper::split(header, '\n');

	if (log)
		std::cerr << "[PLYPointCloudParser] Split header lines " << std::endl;

	if (headerLines.size() == 0 || headerLines[0] != "ply")
		throw std::runtime_error("PLYPointCloudParser::parseHeader: No PLY file detected!");

	if (log) {
		std::cerr << "[PLYPointCloudParser] Header contains: " << std::endl;
		for (int i = 0; i < headerLines.size(); ++i) {
			std::cerr << "[" << i << "] " << headerLines[i] << std::endl;
		}
	}


	// parse each header line
	if (log)
		std::cerr << "[PLYPointCloudParser] Start parsing header lines " << std::endl;
	int elementIndex = -1;
	for (auto line : headerLines) {
		// split line into its content
		std::vector<std::string> content = StringHelper::split(line, ' ');

		// skip empty lines
		if (content.size() == 0) continue;
		// get type of the line, i.e. first word in the line
		std::string& type = content[0];

		// we expect format, element and property line types
		if (type == "format") {
			// assume binary_little_endian format
			if (content[1] != "binary_little_endian")
				std::cout << "PLYPointCloudParser::parseHeader: format not binary_little_endian!" << std::endl;
				//throw std::runtime_error("PLYPointCloudParser::parseHeader: format not binary_little_endian!");
		}
		// expect two elements: vertex and camera and split up the following property informations. use index to split property
		if (type == "element") {
			if (type == "element") {
				std::string& ident = content[1];

				if (ident == "vertex") {
					vertexCount = StringHelper::to_int(content[2]);
					elementIndex = 1;
				}
				else if (ident == "camera") {
					if (StringHelper::to_int(content[2]) != 1)
						throw std::runtime_error("PLYPointCloudParser::parseHeader: Too much camera elements detected. Cannot be more than 1!");
					elementIndex = 2;
				} else {
					std::cout << "[PLYPointCloudParser::parseHeader] Detected additional element in .ply file: " << ident << std::endl;
					elementIndex = 3;
				}

			}
		}

		// if a property line is reached, we must have had a prior vertex or camera element
		// add property to corresponting element
		if (type == "property") {
			if (elementIndex == -1) {
				throw std::runtime_error("PLYPointCloudParser::parseHeader: No vertex or camera element propr to property. An element must preceed a property!");
			}

			if (elementIndex == 1) {
				// vertex property
				Property vp;
				vp.name = content[2];
				vp.type = content[1];
				vertexProperties.push_back(vp);
			}
			if (elementIndex == 2) {
				// camera property
				Property cp;
				cp.name = content[2];
				cp.type = content[1];
				cameraProperties.push_back(cp);
			}
		}
	}
	if (log) {
		std::cerr << "[PLYPointCloudParser] Finished parsing header lines " << std::endl;
		std::cerr << "[PLYPointCloudParser] PointCloud contains " << vertexCount << " points" << std::endl;
		std::cerr << "[PLYPointCloudParser] Vertex contains " << vertexProperties.size() << " properties" << std::endl;
		std::cerr << "[PLYPointCloudParser] Camera contains " << cameraProperties.size() << " properties" << std::endl;
	}
	// parse propertiers to get offsets to the corresponding property and sizes of the elements
	if (log)
		std::cerr << "[PLYPointCloudParser] Parsed vertex properties:" << std::endl;
	vertexOffsetType.resize(vertexProperties.size(), std::pair<int, int>(0, -1)); // resize towards the number of prperties parsed
	vertexSize = 0; // use as consecutivly growing index and therefore start offset
	for (int i = 0; i < vertexProperties.size(); ++i) { // look at each property of vertex
		int t = sizeoftype(vertexProperties[i].type); // size of current property
		if (log)
			std::cerr << "[" << i << "] " << vertexProperties[i].name << ": \t <" << vertexSize << "," << t << ">" << std::endl;
		vertexOffsetType[i] = std::make_pair(vertexSize, t); // add current vertexSize (offset) and t (size) of property
		vertexSize += t;
	}

	if (log)
		std::cerr << "[PLYPointCloudParser] Parsed camera properties:" << std::endl;
	cameraOffsetType.resize(cameraProperties.size(), std::pair<int, int>(0, -1)); // resize towards the number of properties parsed
	cameraSize = 0; // use as consecutivly growing index and therefore start offset
	for (int i = 0; i < cameraProperties.size(); ++i) { // look at each property of vertex
		int t = sizeoftype(cameraProperties[i].type); // size of current property
		if (log)
			std::cerr << "[" << i << "] " << cameraProperties[i].name << ": \t <" << cameraSize << "," << t << ">" << std::endl;
		cameraOffsetType[i] = std::make_pair(cameraSize, t); // add current vertexSize (offset) and t (size) of property
		cameraSize += t;
	}

	//set camera data
	if (vertexCount < 0 || vertexSize < 0)
		throw std::runtime_error("PLYPointCloudParser::parseHeader: vertexSize or vertexCount not initialized correctly!");
	camera_data = std::vector<char>(data.begin() + dataStart + static_cast<long long>(vertexCount) * static_cast<long long>(vertexSize), data.end());

	if (log) {
		std::cerr << "[PLYPointCloudParser] VertexSize: " << vertexSize << std::endl;
		std::cerr << "[PLYPointCloudParser] CameraSize: " << cameraSize << std::endl;
		std::cerr << "[PLYPointCloudParser] Finished parsing header " << std::endl;
	}
}

void PLYPointCloudParser::parsePointCloud() {
	if (cleared) throw std::runtime_error("[PLYPointCloudParser] ERROR: PointCloud has been cleared before parsing!");

	if (log)
		std::cerr << "[PLYPointCloudParser] Start parsing cloud " << std::endl;

	if (log)
		std::cerr << "[PLYPointCloudParser] Parse points " << std::endl;
	points.clear();

	//rng for timestamp
	std::srand(std::time(nullptr));
	//go through all points
	for (int i = 0; i < vertexCount; ++i) {
		char* start = data.data() + dataStart + static_cast<long long>(i) * static_cast<long long>(vertexSize); // get pointet to first char of vertex i

		
		vec3 pos(0, 0, 0);
		vec3 color(0, 0, 0);
		vec3 normal(0, 0, 0);
		float curvature = 1.f;
		int timestamp = std::numeric_limits<int>::max();
		if (setType == "NavVis") {
			float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first); // reinterpret first 3 floats i.e. 12 chars to the first vec3
			pos = vec3(x[0], x[1], x[2]);
			if (vertexOffsetType[3].second == 1) { // parse clouds that contain color as 1 byte per channel i.e. as int in range 0 to 255
				unsigned char* c =
					reinterpret_cast<unsigned char*>(start + vertexOffsetType[3].first);
				color = vec3(c[0], c[1], c[2]);
				color /= 255.0f;
			}
			else if (vertexOffsetType[3].second == 4) { // parse clouds that contain color as 4 bytes per channel i.e. as float
				float* c = reinterpret_cast<float*>(start + vertexOffsetType[3].first);
				color = vec3(c[0], c[1], c[2]);
			}

			float* nx = reinterpret_cast<float*>(start + vertexOffsetType[6].first); // reinterpret next 3 floats i.e. 12 chars to the third vec3 i.e. normal
			vec3 normal(nx[0], nx[1], nx[2]);

			
			curvature =
				*reinterpret_cast<float*>(start + vertexOffsetType[9].first); // reinterpret last element i.e. float for curvature
		}
		else if (setType == "TanksAndTemples") {
			float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first); // reinterpret first 3 floats i.e. 12 chars to the first vec3
			//pos = vec3(x[1], -x[0], x[2]);
			pos = vec3(x[0], x[1], x[2]);
			float* nx = reinterpret_cast<float*>(start + vertexOffsetType[3].first); // reinterpret next 3 floats i.e. 12 chars to the third vec3 i.e. normal
			//vec3 normal(nx[1], -nx[0], nx[2]);
			vec3 normal(nx[0], nx[1], nx[2]);

			if (vertexOffsetType[6].second == 1) { // parse clouds that contain color as 1 byte per channel i.e. as int in range 0 to 255
				unsigned char* c =
					reinterpret_cast<unsigned char*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
				color /= 255.0f;
			}
			else if (vertexOffsetType[6].second == 4) { // parse clouds that contain color as 4 bytes per channel i.e. as float
				float* c = reinterpret_cast<float*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
			}

			

		} else if (setType == "KITTY-360") {
			float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first); // reinterpret first 3 floats i.e. 12 chars to the first vec3
			pos = vec3(x[0], x[1], x[2]);
			if (vertexOffsetType[3].second == 1) { // parse clouds that contain color as 1 byte per channel i.e. as int in range 0 to 255
				unsigned char* c =
					reinterpret_cast<unsigned char*>(start + vertexOffsetType[3].first);
				color = vec3(c[0], c[1], c[2]);
				color /= 255.0f;
			}
			else if (vertexOffsetType[3].second == 4) { // parse clouds that contain color as 4 bytes per channel i.e. as float
				float* c = reinterpret_cast<float*>(start + vertexOffsetType[3].first);
				color = vec3(c[0], c[1], c[2]);
			}
			//int last = -1;
			for (int cv = 0; cv < captured_views.size(); ++cv) {
				/*std::cout << "check cv " << cv << std::endl;
				std::cout << "point_pos " << pos << std::endl;
				std::cout << "pose_pos " << captured_views[cv].pos << std::endl;
				std::cout << "dist " << length(captured_views[cv].pos - pos )<< std::endl;*/
				float dist = length(captured_views[cv].pos - pos);
				if (dist < 10.f) {
					timestamp = cv;
					break;
				}
				if (dist < 100.f) {
					float rand_f = (float(std::rand()) / float(RAND_MAX));
					float weight = 1-((dist - 10.f) / 90.f); // between 10 and 100 -> linear scale from 1: 100.f to 0: 10.f
					
					if (rand_f < 0.07 * weight*weight*weight) {
						timestamp = cv;
						//std::cout << "accept" << std::endl;
						break;
					}
					//if (last < cv) last= cv;
				}
				
				//std::cout << "denied" << std::endl;
			}
			//if (last != -1 && timestamp == std::numeric_limits<int>::max()) timestamp = last; // if random value did not trigger the timestamp, add to the last timestamp/image that has seen the point
			//std::cout << "timestamp: " << timestamp << "\r";
			//timestamp = 1;
			//float* nx = reinterpret_cast<float*>(start + vertexOffsetType[6].first); // reinterpret next 3 floats i.e. 12 chars to the third vec3 i.e. normal
			//vec3 normal(nx[0], nx[1], nx[2]);


			//curvature =
			//	*reinterpret_cast<float*>(start + vertexOffsetType[9].first); // reinterpret last element i.e. float for curvature
		}
		else if (setType == "Redwood" || setType == "ScanNet"  || setType == "Generic") {
			float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first); // reinterpret first 3 floats i.e. 12 chars to the first vec3
			//pos = vec3(x[1], -x[0], x[2]);
			pos = vec3(x[0], x[1], x[2]);
			float* nx = reinterpret_cast<float*>(start + vertexOffsetType[3].first); // reinterpret next 3 floats i.e. 12 chars to the third vec3 i.e. normal
			//vec3 normal(nx[1], -nx[0], nx[2]);
			vec3 normal(nx[0], nx[1], nx[2]);

			if (vertexOffsetType[6].second == 1) { // parse clouds that contain color as 1 byte per channel i.e. as int in range 0 to 255
				unsigned char* c =
					reinterpret_cast<unsigned char*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
				color /= 255.0f;
			}
			else if (vertexOffsetType[6].second == 4) { // parse clouds that contain color as 4 bytes per channel i.e. as float
				float* c = reinterpret_cast<float*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
			}

			if (vertexProperties.size() >= 10 && vertexProperties[9].name == "timestamp") {
				uint32_t* ts = reinterpret_cast<uint32_t*>(start + vertexOffsetType[9].first);
				timestamp = ts[0];
			}
		}
		else if (setType == "L") {
			float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first); // reinterpret first 3 floats i.e. 12 chars to the first vec3
			//pos = vec3(x[1], -x[0], x[2]);
			pos = vec3(x[0], x[1], x[2]);
			float* nx = reinterpret_cast<float*>(start + vertexOffsetType[3].first); // reinterpret next 3 floats i.e. 12 chars to the third vec3 i.e. normal
			//vec3 normal(nx[1], -nx[0], nx[2]);
			vec3 normal(nx[0], nx[1], nx[2]);

			if (vertexOffsetType[6].second == 1) { // parse clouds that contain color as 1 byte per channel i.e. as int in range 0 to 255
				unsigned char* c =
					reinterpret_cast<unsigned char*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
				color /= 255.0f;
			}
			else if (vertexOffsetType[6].second == 4) { // parse clouds that contain color as 4 bytes per channel i.e. as float
				float* c = reinterpret_cast<float*>(start + vertexOffsetType[6].first);
				color = vec3(c[0], c[1], c[2]);
			}



		}
		// construct PointCloudPoint from parsed data
		PointCloudPoint v;
		v.pos = pos;
		v.color = color;
		v.normal = normal;
		v.curvature = 0;// curvature;
		v.timestamp = timestamp;
		points.push_back(v);
	}
	if (log)
		std::cerr << "[PLYPointCloudParser] Parsed " << points.size() << " points" << std::endl;

	if (log && points.size() >= 3) {
		std::cerr << "[PLYPointCloudParser] First 3 Points: \n";
		std::cerr << points[0] << points[1] << points[2];
		std::cerr << "[PLYPointCloudParser] Last 3 Points: \n";
		std::cerr << points[static_cast<long long>(vertexCount) - 3] << points[static_cast<long long>(vertexCount) - 2] << points[static_cast<long long>(vertexCount) - 1];
	}
	if (log)
		std::cerr << "[PLYPointCloudParser] Parse camera " << std::endl;

	// parse the camera parameters
	if (cameraOffsetType.size()) // only parse camera if detected in header
	{
		/* long long cameraStart = dataStart + static_cast<long long>(vertexCount) * static_cast<long long>(vertexSize);
		 std::cerr << " dataStart " << dataStart << " data_size " << data.size() << " header_data_size " << header_data.size() << " camera_data_size " << camera_data.size() << " camera_start " << cameraStart << std::endl;*/
		 // use constructed camera_data container as data source
		char* start = camera_data.data(); //data.data() + cameraStart;

		//reinterpret each property as needed and extract data from byte stream
		float* vp = reinterpret_cast<float*>(start + cameraOffsetType[0].first);
		vec3 view_p(vp[0], vp[1], vp[2]);

		float* x = reinterpret_cast<float*>(start + cameraOffsetType[3].first);
		vec3 x_axis(x[0], x[1], x[2]);

		float* y = reinterpret_cast<float*>(start + cameraOffsetType[6].first);
		vec3 y_axis(y[0], y[1], y[2]);

		float* z = reinterpret_cast<float*>(start + cameraOffsetType[9].first);
		vec3 z_axis(z[0], z[1], z[2]);

		float focal =
			*reinterpret_cast<float*>(start + cameraOffsetType[12].first);

		float* s = reinterpret_cast<float*>(start + cameraOffsetType[13].first);
		vec2 scale(s[0], s[1]);

		float* c = reinterpret_cast<float*>(start + cameraOffsetType[15].first);
		vec2 center(c[0], c[1]);

		int* v = reinterpret_cast<int*>(start + cameraOffsetType[17].first);
		ivec2 viewport(v[0], v[1]);

		float* k = reinterpret_cast<float*>(start + cameraOffsetType[19].first);
		vec2 ks(k[0], k[1]);

		// store data to camera parameter
		camera.view_p = view_p;
		camera.x_axis = x_axis;
		camera.y_axis = y_axis;
		camera.z_axis = z_axis;
		camera.focal = focal;
		camera.scale = scale;
		camera.center = center;
		camera.viewport = viewport;
		camera.k = ks;
	}
	if (log)
		std::cerr << "[PLYPointCloudParser] Parsed camera\n" << camera << std::endl;

	if (log)
		std::cerr << "[PLYPointCloudParser] Finished parsing cloud " << std::endl;
}

void PLYPointCloudParser::savePointCloud(const std::string& name, const int count)
{
	if (cleared) throw std::runtime_error("[PLYPointCloudParser] ERROR: PointCloud has been cleared before saving!");
	if (log)
		std::cerr << "[PLYPointCloudParser] Start storing pointcloud" << std::endl;
	// if no count is given, store whole cloud
	int outputCount = (count == -1) ? vertexCount : count;

	// get correct file size
	std::vector<char> out;
	out.resize(dataStart + outputCount * vertexSize + cameraSize);

	// write header_data to the beginning of out
	// parse char vector into string to find correct position
	std::string header_str(header_data.begin(), header_data.end());
	//get position where the number of vertices belongs
	auto pos = header_str.find("vertex ");
	while (header_str[pos] != '0') {
		++pos;
	}
	// prepare 10 character sized string with new output size
	std::string strCount = std::to_string(outputCount);
	if (strCount.size() > 10) throw std::runtime_error("[PLYPointCloudParser] ERROR: count contains too much characters (more than 10)");
	while (strCount.size() < 10) strCount = '0' + strCount;
	for (int i = 0; i < 10; ++i) {
		header_str[pos + i] = strCount[i];
	}
	int og_header_size = header_data.size();
	//header_data = header_str.data();
	std::copy(header_str.begin(), header_str.end(), header_data.begin());
	std::cerr << "header sizes: old " << og_header_size << ", new " << header_data.size() << std::endl;
	std::cerr << "header is\n" << header_str << std::endl;
	for (int i = 0; i != dataStart; ++i) {
		out[i] = header_data[i];
	}
	//write camera_data to the end of out
	for (int i = 0; i != cameraSize; ++i) {
		out[dataStart + outputCount * vertexSize + i] = camera_data[i];
	}
	for (int i = 0; i != outputCount; ++i) {
		// get char* to start of current vertex
		char* start = out.data() + dataStart + i * vertexSize;
		PointCloudPoint v = points[i];

		// write pos
		float* x = reinterpret_cast<float*>(start + vertexOffsetType[0].first);
		for (int i = 0; i != 3; ++i) x[i] = v.pos[i];

		// adjust color according to the read data format (char or float)
		if (vertexOffsetType[3].second == 1) {
			unsigned char* c =
				reinterpret_cast<unsigned char*>(start + vertexOffsetType[3].first);
			v.color *= 255.f;
			for (int i = 0; i != 3; ++i) c[i] = v.color[i];

		}
		else if (vertexOffsetType[3].second == 4) {
			float* c = reinterpret_cast<float*>(start + vertexOffsetType[3].first);
			for (int i = 0; i != 3; ++i) c[i] = v.color[i];
		}

		// write normal 
		float* nx = reinterpret_cast<float*>(start + vertexOffsetType[6].first);
		for (int i = 0; i != 3; ++i) nx[i] = v.normal[i];
		// write curvature
		float* curvature =
			reinterpret_cast<float*>(start + vertexOffsetType[9].first);
		curvature[0] = v.curvature;
	}

	std::cout << "[PLYPointCloudParser] Saving Cloud as " << name << std::endl;
	std::ofstream stream(name, std::ios::binary);

	if (!stream.is_open()) {
		std::cerr << "Could not open file " << name << std::endl;
		throw std::runtime_error("invalid file: " + name);
	}
	stream.write(out.data(), out.size());

	if (log)
		std::cerr << "[PLYPointCloudParser] Finished storing pointcloud" << std::endl;

}



void PLYPointCloudParser::reducePointCloudByRadius(float radius) {
	if (cleared) throw std::runtime_error("[PLYPointCloudParser:reducePointCloudByRadius] ERROR: PointCloud has been cleared before reducing!");
	int old_size = points.size();
	float cell_scale = 45.f; // scale how much radii one cell contains

	// put points into a cell structure
	// 1. get boundaries
	auto aabb = getBoundingBox();
	vec3 min = aabb.first;
	vec3 max = aabb.second;
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Bounding Box Computation: " << old_size << " Points of " << old_size << " done: " << "100%" << std::endl;
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Finished Bounding Box Computation: min: [" << min.x << ", " << min.y << ", " << min.z << "], max: [" << max.x << ", " << max.y << ", " << max.z << "]" << std::endl;

	// 2. sort points into cells depending on the dimensions of the bounding box and the radius
	int dimX = int((max.x - min.x) / (radius * cell_scale)) + 1;
	int dimY = int((max.y - min.y) / (radius * cell_scale)) + 1;
	int dimZ = int((max.z - min.z) / (radius * cell_scale)) + 1;
	// create the vectors for the cells
	std::vector<std::vector<std::vector<std::vector<int>>>> grid;
	grid.resize(dimX);
	for (int x = 0; x < dimX; ++x) {
		grid[x].resize(dimY);
		for (int y = 0; y < dimY; ++y) {
			grid[x][y].resize(dimZ);
		}
	}
	// go through points and add id to back of corresponding cell
	for (int i = 0; i < old_size; ++i) {
		if (i % 100000 == 0) {
			std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Sort Points into Cells: " << i << " Points of " << old_size << " done: " << (float(i) / float(old_size)) * 100 << "%\r";
			std::cerr.flush();
		}
		vec3 point = points[i].pos;
		// reposition point such that min is origin
		point = point - min;
		// rescale point such that max is dim
		point = point / (radius * cell_scale);
		// get id of cell
		int idx = int(point.x);
		int idy = int(point.y);
		int idz = int(point.z);
		//push id to vector of id
		grid[idx][idy][idz].push_back(i);
	}
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Sort Points into Cells: " << old_size << " Points of " << old_size << " done: " << "100%" << std::endl;
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Finished Sort Points into Cells: grid has dimension [" << dimX << "," << dimY << "," << dimZ << "]" << std::endl;



	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Start reducing pointcloud with " << old_size << " points with radius " << radius << std::endl;


	std::vector<bool> deleted(old_size);
	std::fill(deleted.begin(), deleted.end(), false);
	std::vector<PointCloudPoint> new_points;
	float squared_radius = radius * radius;
	// go through all cells
	int cell_counter = 0;
	int point_counter = 0;
	for (int x = 0; x < dimX; ++x) {
		for (int y = 0; y < dimY; ++y) {
			for (int z = 0; z < dimZ; ++z) {
				std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Check Points: " << cell_counter << " (" << point_counter << ") cells(points) of " << dimX * dimY * dimZ << " (" << old_size << ") done: "
					<< (float(cell_counter) / float(dimX * dimY * dimZ)) * 100 << " (" << (float(point_counter) / float(old_size)) * 100 << "%), new PointCloud has currently " << new_points.size() << " points \r";
				std::cerr.flush();
				cell_counter++;
				for (int k = 0; k < grid[x][y][z].size(); ++k) {
					if (point_counter % 1000 == 0) {
						std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Check Points: " << cell_counter << " (" << point_counter << ") cells(points) of " << dimX * dimY * dimZ << " (" << old_size << ") done: "
							<< (float(cell_counter) / float(dimX * dimY * dimZ)) * 100 << " (" << (float(point_counter) / float(old_size)) * 100 << "%), new PointCloud has currently " << new_points.size() << " points \r";
						std::cerr.flush();
					}
					point_counter++;
					int i = grid[x][y][z][k];

					if (deleted[i]) continue; // if point already deleted, skip
					new_points.push_back(points[i]); // add to new list and mark as deleted
					deleted[i] = true;
					//// go through neighboring buckets, such that 2 buckets are only compared once towards each other
					///*glm::ivec3  offsets[14] = { {0,0,0} ,{1,1,1},{1,1,0},{1,1,-1},{1,0,1},{1,0,0},{1,0,-1},{1,-1,1},{1,-1,0},{1,-1,-1},
					//	{0,0,1},{0,1,0},{0,1,1},{0,1,-1} }; // is that enough? */
					glm::ivec3  offsets[27] = { {0,0,0} , // 1 center
											{0,0,1}, {0,0,-1}, {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, // 6 surrounding the center
											{0,1,1}, {0,1,-1}, {0,-1,1}, {0,-1,-1}, {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1}, {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0}, // 12 edges
											{1,1,1}, {1,1,-1}, {1,-1,1}, {-1,1,1}, {-1,-1,1}, {-1,1,-1}, {1,-1,-1}, {-1,-1,-1}   // 8 corners
					};


					for (auto& offset : offsets) {
						int idx = x + offset.x;
						int idy = y + offset.y;
						int idz = z + offset.z;
						if (idx == -1 || idy == -1 || idz == -1 || idx == dimX || idy == dimY || idz == dimZ) continue; // if one id is -1, the bucket is out of bounds -> skip
						std::vector<int>& other_ids = grid[idx][idy][idz];
						for (int s = 0; s < other_ids.size(); ++s) {
							int j = other_ids[s];
							if (deleted[j]) continue; //if j is already deleted, skip j
							// dont use length, but squared length for performance
							vec3 dist = points[i].pos - points[j].pos;
							float squared_length = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
							if (squared_length < squared_radius) deleted[j] = true; // if j is too close to i, delete it
						}
					}

				}
			}
		}

	}


	//for (int i = 0; i < old_size - 1; ++i) {
	//	// each 10000 points, give an update
	//	if (i % 10000 == 0) {
	//		std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] " << i << " Points of " << old_size << " done: " << (float(i)/float(old_size)) * 100 << "%, new Cloud currently contains " << new_points.size() << " points\r" ;
	//		std::cerr.flush();
	//	}

	//	// if i already deleted, skip i
	//	if (deleted[i]) continue;
	//	//otherwise, add i to new list 
	//	new_points.push_back(points[i]);
	//	// remove all points in given
	//	for (int j = i + 1; j < old_size; ++j) {
	//		if (deleted[j]) continue; // if j is already deleted, skip j
	//		// dont use length, but squared length for performance
	//		vec3 dist = points[i].pos - points[j].pos;
	//		float squared_length = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
	//		if ( squared_length < squared_radius) deleted[j] = true; // if j is too close to i, delete it
	//	}
	//}
	std::cerr << std::endl;
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Finished reducing pointcloud with " << old_size << " points with radius " << radius << " to PointCloud with " <<
		new_points.size() << " points" << std::endl;
	points.clear();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Copy new points " << std::endl;
	points = new_points;
	vertexCount = points.size();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Finished " << std::endl;
}

void PLYPointCloudParser::reducePointCloudByBoundingBox(vec3& min, vec3& max) {
	std::cerr << "[PLYPointCloudParser:reducePointCloudByBoundingBox] Start " << std::endl;
	if (cleared) throw std::runtime_error("[PLYPointCloudParser:reducePointCloudByRadius] ERROR: PointCloud has been cleared before reducing!");
	int old_size = points.size();
	std::vector<PointCloudPoint> new_points;

	for (int i = 0; i < old_size; ++i) {
		if (i % 100000 == 0) {
			std::cerr << "[PLYPointCloudParser:reducePointCloudByBoundingBox] Check Points: " << i << " Points of " << old_size << " done: " << (float(i) / float(old_size)) * 100 << "%, new PointCloud has currently " << new_points.size() << " points\r";
			std::cerr.flush();
		}
		PointCloudPoint& point = points[i];
		vec3& pos = point.pos;
		if (pos.x<min.x || pos.y<min.y || pos.z<min.z || pos.x> max.x || pos.y > max.y || pos.z> max.z) continue;
		new_points.push_back(point);
	}

	points.clear();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByRadius] Copy new points " << std::endl;
	points = new_points;
	vertexCount = points.size();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByBoundingBox] Finished " << std::endl;
}


/// <summary>
/// teke every divisor point and delete every other to split up point clopud into smaller ones
/// </summary>
/// <param name="offset">start index for the division</param>
/// <param name="divisor">step size</param>
void PLYPointCloudParser::reducePointCloudByFactor(int offset, int factor) {
	std::cerr << "[PLYPointCloudParser:reducePointCloudByFactor] Start " << std::endl;
	if (cleared) throw std::runtime_error("[PLYPointCloudParser:reducePointCloudByFactor] ERROR: PointCloud has been cleared before reducing!");
	int old_size = points.size();
	std::vector<PointCloudPoint> new_points;
	for (uint32_t i = offset; i < old_size; i += factor) {
		if (i % 100000 <= 4) {
			std::cerr << "[PLYPointCloudParser:reducePointCloudByFactor] Fetch Point: " << i << " of " << old_size << " done: " << (float(i) / float(old_size)) * 100 << "%, new PointCloud has currently " << new_points.size() << " points\r";
			std::cerr.flush();
		}
		PointCloudPoint& point = points[i];
		new_points.push_back(point);
	}

	points.clear();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByFactor] Copy new points " << std::endl;
	points = new_points;
	vertexCount = points.size();
	std::cerr << "[PLYPointCloudParser:reducePointCloudByFactor] Finished " << std::endl;
}

/// <summary>
/// transforms the coordinates of the points from navvis to gl space, i.e. switch the coordinates
/// </summary>
void PLYPointCloudParser::transformPointCloudToGL() {
	for (PointCloudPoint& p : points) {
		p.pos = vec3(p.pos.y, p.pos.z, p.pos.x);
		p.normal = vec3(p.normal.y, p.normal.z, p.normal.x);
	}
}
/// <summary>
/// transforms the coordinates of the points from gl to navvis space, i.e. switch the coordinates
/// </summary>
void PLYPointCloudParser::transformPointCloudToNavvis() {
	for (PointCloudPoint& p : points) {
		p.pos = vec3(p.pos.z, p.pos.x, p.pos.y);
		p.normal = vec3(p.normal.z, p.normal.x, p.normal.y);
	}
}
/// <summary>
/// creates a bounding structure Grid with given cell size and stores results to membervariable bounding_structure
/// </summary>
/// <param name="cell_size">size of the cells in the grid</param>
void PLYPointCloudParser::createBoundingStructureGrid(float cell_size) {
	if (cleared) throw std::runtime_error("[PLYPointCloudParser:createBoundingStructureGrid] ERROR: PointCloud has been cleared before reducing!");
	int old_size = points.size();

	// put points into a cell structure
	// 1. get boundaries
	auto aabb = getBoundingBox();
	vec3 aabb_min = aabb.first;
	vec3 aabb_max = aabb.second;
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Bounding Box Computation: " << old_size << " Points of " << old_size << " done: " << "100%" << std::endl;
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Finished Bounding Box Computation: min: [" << aabb_min.x << ", " << aabb_min.y << ", " << aabb_min.z << "], max: [" << aabb_max.x << ", " << aabb_max.y << ", " << aabb_max.z << "]" << std::endl;

	// 2. sort points into cells depending on the dimensions of the bounding box and the cell_size
	unsigned int dimX = int(float(aabb_max.x - aabb_min.x) / (cell_size)) + 1;
	unsigned int dimY = int((aabb_max.y - aabb_min.y) / (cell_size)) + 1;
	unsigned int dimZ = int((aabb_max.z - aabb_min.z) / (cell_size)) + 1;
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Bounding Structure Dim: " << dimX << ", " << dimY << ", " << dimZ << std::endl;
	// create the vectors for the cells
	std::vector<std::vector<std::vector<std::vector<int>>>> grid;
	grid.resize(dimX);
	for (int x = 0; x < dimX; ++x) {
		grid[x].resize(dimY);
		for (int y = 0; y < dimY; ++y) {
			grid[x][y].resize(dimZ);
		}
	}
	// go through points and add id to back of corresponding cell
	for (int i = 0; i < old_size; ++i) {
		//if (log && i % 100000 == 0) {
		//	std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Sort Points into Cells: " << i << " Points of " << old_size << " done: " << (float(i) / float(old_size)) * 100 << "%\r";
		//	std::cerr.flush();
		//}
		vec3 point = points[i].pos;
		// reposition point such that min is origin
		point = point - aabb_min;
		// rescale point such that max is dim
		point = point / (cell_size);
		// get id of cell
		int idx = int(point.x);
		int idy = int(point.y);
		int idz = int(point.z);
		//push id to vector of id
		grid[idx][idy][idz].push_back(i);
	}
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Sort Points into Cells: " << old_size << " Points of " << old_size << " done: " << "100%" << std::endl;
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Finished Sort Points into Cells: grid has dimension [" << dimX << "," << dimY << "," << dimZ << "]" << std::endl;



	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Start sorting pointcloud" << std::endl;

	std::vector<PointCloudPoint> new_points;
	// delete previous bounding structure
	bounding_structure.clear();
	// go through all cells
	int cell_counter = 0;
	int point_counter = 0;
	constexpr float fmin = std::numeric_limits<float>::min();
	constexpr float fmax = std::numeric_limits<float>::max();
	for (int x = 0; x < dimX; ++x) {
		for (int y = 0; y < dimY; ++y) {
			for (int z = 0; z < dimZ; ++z) {
				//if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Sort Points: " << cell_counter << " (" << point_counter << ") cells(points) of " << dimX * dimY * dimZ << " (" << old_size << ") done: "
				//	<< (float(cell_counter) / float(dimX * dimY * dimZ)) * 100 << " (" << (float(point_counter) / float(old_size)) * 100 << "%)\r";
				//if (log) std::cerr.flush();
				cell_counter++;

				uint32_t voxel_start = new_points.size();

				vec3 cur_min = vec3(fmax, fmax, fmax);
				vec3 cur_max = vec3(fmin, fmin, fmin);

				// go through all points in the current cell
				for (int k = 0; k < grid[x][y][z].size(); ++k) {
					//if (log && point_counter % 1000 == 0) {
					//	std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Sort Points: " << cell_counter << " (" << point_counter << ") cells(points) of " << dimX * dimY * dimZ << " (" << old_size << ") done: "
					//		<< (float(cell_counter) / float(dimX * dimY * dimZ)) * 100 << " (" << (float(point_counter) / float(old_size)) * 100 << "%)\r";
					//	std::cerr.flush();
					//}
					point_counter++;
					int i = grid[x][y][z][k];
					// push current point
					new_points.push_back(points[i]);

					vec3 pos = points[i].pos;
					cur_min.x = (pos.x < cur_min.x) ? pos.x : cur_min.x;
					cur_min.y = (pos.y < cur_min.y) ? pos.y : cur_min.y;
					cur_min.z = (pos.z < cur_min.z) ? pos.z : cur_min.z;
					cur_max.x = (pos.x > cur_max.x) ? pos.x : cur_max.x;
					cur_max.y = (pos.y > cur_max.y) ? pos.y : cur_max.y;
					cur_max.z = (pos.z > cur_max.z) ? pos.z : cur_max.z;

				}
				vec3 cur_center = cur_min + (cur_max - cur_min) * 0.5f;
				// push new voxel in bounding structure
				uint32_t voxel_size = new_points.size() - voxel_start;
				if (voxel_size) { // only add to structue if there are points in the voxel
					/*vec3 test_center = ;
					vec3 test_min = aabb_min + vec3(x*cell_size, y*cell_size, z*cell_size);
					vec3 test_max;
					PointCloudVoxel voxel{ test_center, test_min, test_max, length(test_center - test_min), voxel_start, voxel_size };*/
					PointCloudVoxel voxel{ cur_center, length(cur_center - cur_min),cur_min, voxel_start, cur_max, voxel_size };
					bounding_structure.emplace_back(voxel);
				}
			}
		}

	}


	if (log) {
		std::cerr << std::endl;
		std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Finished sorting Points according to Bounding Grid with cell_size " << cell_size << std::endl;
	}
	points.clear();

	if (log) {
		std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Bounding Structure contains " << bounding_structure.size() << " voxels:" << std::endl;
		//for (auto& b : bounding_structure) {
		//	std::cerr << " start: " << b.start << " size: " << b.size << "\t center: [" << b.center.x << ", " << b.center.y << ", " << b.center.z << "]\t min : [" << b.aabb_min.x << ", " << b.aabb_min.y << ", " << b.aabb_min.z << "]\t max : [" << b.aabb_max.x << ", " << b.aabb_max.y << ", " << b.aabb_max.z << "] radius : " << b.radius << std::endl;
		//}


		std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Copy new points " << std::endl;
	}
	points = new_points;
	vertexCount = points.size();
	if (log) std::cerr << "[PLYPointCloudParser:createBoundingStructureGrid] Finished " << std::endl;
}

/// <summary>
/// clears vectors of the parser to reduce ram usage. DO NOT call before savepointcloud
/// </summary>
void PLYPointCloudParser::clear() {
	cleared = true;
	data.clear();
	header_data.clear();
	camera_data.clear();
	points.clear();
	bounding_structure.clear();
	captured_views.clear();
}
