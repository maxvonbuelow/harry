enum MtlColorType { RGB, XYZ, SPECTRAL };
MtlColorType read_mtl_color(std::istream &is, float *col)
{
	if (is.peek() == 'x' || is.peek() == 's') {
		std::string type;
		is >> type;
		if (type == "xyz") {
			is >> col[0] >> col[1] >> col[2];
			return XYZ;
		} else if (type == "spectral") {
			return SPECTRAL;
		} else {
			throw std::runtime_error("unsupported color type");
		}
	} else {
		is >> col[0] >> col[1] >> col[2];
		return RGB;
	}
}

enum ReflType { SPHERE, CUBE_TOP, CUBE_BOTTOM, CUBE_FRONT, CUBE_BACK, CUBE_LEFT, CUBE_RIGHT };
enum ImfchanType { R, G, B, M, L, Z };
struct MapArgs {
	bool has_blendu, has_blendv, has_cc, has_clamp, has_imfchan, has_mm, has_o, has_s, has_t, has_texres, has_bm, has_boost;

	bool blendu, blendv, cc, clamp;
	ImfchanType imfchan;
	float mm_base, mm_gain, o_u, o_v, o_w, s_u, s_v, s_w, t_u, t_v, t_w, texres, bm, boost;
	int o_c, s_c, t_c; // mumber of channels: 1, 2 or 3

	std::string filename;

	MapArgs() : has_blendu(false), has_blendv(false), has_cc(false), has_clamp(false), has_imfchan(false), has_mm(false), has_o(false), has_s(false), has_t(false), has_texres(false), has_bm(false), has_boost(false)
	{}

	void print() const
	{
		std::cout << filename;
		if (has_blendu) std::cout << " [blendu=" << blendu << "]";
		if (has_blendv) std::cout << " [blendv=" << blendv << "]";
		if (has_cc) std::cout << " [cc=" << cc << "]";
		if (has_clamp) std::cout << " [clamp=" << clamp << "]";
		if (has_imfchan) std::cout << " [imfchan=" << imfchan << "]";
		if (has_mm) std::cout << " [mm=" << mm_base << "," << mm_gain << "]";
		if (has_o) {
			std::cout << " [o=";
			for (int i = 0; i < o_c; ++i) std::cout << (i ? "," : "") << (&o_u)[i];
			std::cout << "]";
		}
		if (has_s) {
			std::cout << " [s=";
			for (int i = 0; i < s_c; ++i) std::cout << (i ? "," : "") << (&s_u)[i];
			std::cout << "]";
		}
		if (has_t) {
			std::cout << " [t=";
			for (int i = 0; i < t_c; ++i) std::cout << (i ? "," : "") << (&t_u)[i];
			std::cout << "]";
		}
		if (has_texres) std::cout << " [texres=" << texres << "]";
		if (has_bm) std::cout << " [bm=" << bm << "]";
		if (has_boost) std::cout << " [boost=" << boost << "]";
	}
};
bool read_arglist(std::istream &is, float *dst, int *chan, std::string &fn)
{
	is >> dst[0];
	*chan = 1;
	std::string sval;
	for (int i = 1; i < 3; ++i) {
		util::skip_ws(is);
		if (is.peek() == '-') break;
		is >> sval;
		if (is.peek() == '\n') {
			fn = sval;
			return true;
		}
		*chan = i + 1;
		std::stringstream(sval) >> dst[i];		
	}
	return false;
}
MapArgs read_map_args(std::istream &is)
{
	MapArgs args;
	bool fn_read = false;
	util::skip_ws(is);
	while (is.peek() == '-') {
		is.get();
		std::string id;
		is >> id;
		std::string sval;
		if (id == "blendu") {
			args.has_blendu = true;
			is >> sval;
			if (sval == "on") args.blendu = true;
			else args.blendu = false;
		} else if (id == "blendv") {
			args.has_blendv = true;
			is >> sval;
			if (sval == "on") args.blendv = true;
			else args.blendv = false;
		} else if (id == "cc") {
			args.has_cc = true;
			is >> sval;
			if (sval == "on") args.cc = true;
			else args.cc = false;
		} else if (id == "clamp") {
			args.has_clamp = true;
			is >> sval;
			if (sval == "on") args.clamp = true;
			else args.clamp = false;
		} else if (id == "imfchan") {
			args.has_imfchan = true;
			std::string sval;
			is >> sval;
			if (sval == "r")      args.imfchan = R;
			else if (sval == "g") args.imfchan = G;
			else if (sval == "b") args.imfchan = B;
			else if (sval == "m") args.imfchan = M;
			else if (sval == "l") args.imfchan = L;
			else if (sval == "z") args.imfchan = Z;
			else args.has_imfchan = false;
		} else if (id == "mm") {
			args.has_mm = true;
			is >> args.mm_base >> args.mm_gain;
		} else if (id == "o") {
			args.has_o = true;
			fn_read = read_arglist(is, &args.o_u, &args.o_c, args.filename);
		} else if (id == "s") {
			args.has_s = true;
			fn_read = read_arglist(is, &args.s_u, &args.s_c, args.filename);
		} else if (id == "t") {
			args.has_t = true;
			fn_read = read_arglist(is, &args.t_u, &args.t_c, args.filename);
		} else if (id == "texres") {
			args.has_texres = true;
			is >> args.texres;
		} else if (id == "bm") {
			args.has_bm = true;
			is >> args.bm;
		} else if (id == "boost") {
			args.has_boost = true;
			is >> args.boost;
		}
		util::skip_ws(is);
	}
	if (!fn_read) is >> args.filename;
	util::skip_line(is);
	return args;
}

enum Attrs { VERTEX, TEX, NORMAL };

template <typename T>
void read_mtllib(std::istream &is, T &handle)
{
	std::string name;
	float color[3];
	float val; int ival;
	while (!is.eof()) {
		std::string id;
		util::skip_ws(is);
		if (is.peek() != '\n') is >> id;

		if (id == "#") {
			util::skip_line(is);
		} else if (id.empty()) {
			// empty line
			if ( is.tellg()==-1) break;
			util::skip_line(is);
		} else if (id == "newmtl") {
			util::skip_ws(is);
			util::getline(is, name);
			handle.newmtl(name);
		} else if (id == "Ka") {
			switch (read_mtl_color(is, color)) {
			case RGB: handle.col_ambient_rgb(color); break;
			case XYZ: handle.col_ambient_xyz(color); break;
			case SPECTRAL: std::cout << "WARNING: Spectral colors currently unsupported" << std::endl; break;
			}
		} else if (id == "Kd") {
			switch (read_mtl_color(is, color)) {
			case RGB: handle.col_diffuse_rgb(color); break;
			case XYZ: handle.col_diffuse_xyz(color); break;
			case SPECTRAL: std::cout << "WARNING: Spectral colors currently unsupported" << std::endl; break;
			}
		} else if (id == "Ks") {
			switch (read_mtl_color(is, color)) {
			case RGB: handle.col_specular_rgb(color); break;
			case XYZ: handle.col_specular_xyz(color); break;
			case SPECTRAL: std::cout << "WARNING: Spectral colors currently unsupported" << std::endl; break;
			}
		} else if (id == "Tf") {
			switch (read_mtl_color(is, color)) {
			case RGB: handle.col_tf_rgb(color); break;
			case XYZ: handle.col_tf_xyz(color); break;
			case SPECTRAL: std::cout << "WARNING: Spectral colors currently unsupported" << std::endl; break;
			}
		} else if (id == "d") {
			bool halo = false;
			util::skip_ws(is);
			if (is.peek() == '-') { // -halo
				std::string arg;
				is >> arg;
				if (arg == "-halo") halo = true;
			}
			is >> val;
			handle.tr(1.0f - val, halo);
		} else if (id == "Tr") {
			is >> val;
			handle.tr(val, false);
		} else if (id == "illum") {
			std::string illum;
			is >> illum;
			std::stringstream ss(illum.substr(illum.find_last_of('_') + 1));
			ss >> ival;
			handle.illum(ival);
		} else if (id == "Ns") {
			is >> ival;
			handle.exp(ival);
		} else if (id == "sharpness") {
			is >> ival;
			handle.sharpness(ival);
		} else if (id == "Ni") {
			is >> val;
			handle.density(val);
		} else if (id == "map_Ka") { // TEX HERE
			MapArgs args = read_map_args(is);
			handle.map_ambient(args);
		} else if (id == "map_Kd") {
			MapArgs args = read_map_args(is);
			handle.map_diffuse(args);
		} else if (id == "map_Ks") {
			MapArgs args = read_map_args(is);
			handle.map_specular(args);
		} else if (id == "map_Ns") {
			MapArgs args = read_map_args(is);
			handle.map_exp(args);
		} else if (id == "map_d") {
			MapArgs args = read_map_args(is);
			handle.map_tr(args);
		} else if (id == "map_aat") {
			std::string val;
			is >> val;
			handle.map_aat(val == "on");
		} else if (id == "decal") {
			MapArgs args = read_map_args(is);
			handle.map_decal(args);
		} else if (id == "disp") {
			MapArgs args = read_map_args(is);
			handle.map_disp(args);
		} else if (id == "bump") {
			MapArgs args = read_map_args(is);
			handle.map_bump(args);
		} else if (id == "refl") { // REFL HERE
			std::string type, refltype;
			is >> type >> refltype;
			if (type != "-type") throw std::runtime_error("Refl has no -type option");
			ReflType rt;
			if      (refltype == "sphere")      rt = SPHERE;
			else if (refltype == "cube_top")    rt = CUBE_TOP;
			else if (refltype == "cube_bottom") rt = CUBE_BOTTOM;
			else if (refltype == "cube_front")  rt = CUBE_FRONT;
			else if (refltype == "cube_back")   rt = CUBE_BACK;
			else if (refltype == "cube_left")   rt = CUBE_LEFT;
			else if (refltype == "cube_right")  rt = CUBE_RIGHT;

			MapArgs args = read_map_args(is);
			handle.map_refl(rt, args);
		}
	}
}

struct MtlHandle {
	enum AttrName { COL_AMBIENT, COL_DIFFUSE, COL_SPECULAR, COL_TF, TR, ILLUM, EXP, SHARPNESS, DENSITY, MAP_AMBIENT, MAP_DIFFUSE, MAP_SPECULAR, MAP_EXP, MAP_TR, MAP_DECAL, MAP_DISP, MAP_BUMP, MAP_AAT, MAP_REFL, ATTRNUM };
	std::vector<bool> attr_set;

	void newmtl(const std::string &name)
	{
		attr_set.clear();
		attr_set.resize(ATTRNUM, false);
		std::cout << "========= New mtl: " << name << std::endl;
	}

	void col_ambient_rgb(float *col)
	{
		if (attr_set[COL_AMBIENT]) {
			std::cout << "WARNING: Attribute \"COL_AMBIENT\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_AMBIENT] = true;
		std::cout << "Color ambient RGB: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_ambient_xyz(float *col)
	{
		if (attr_set[COL_AMBIENT]) {
			std::cout << "WARNING: Attribute \"COL_AMBIENT\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_AMBIENT] = true;
		std::cout << "Color ambient XYZ: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_diffuse_rgb(float *col)
	{
		if (attr_set[COL_DIFFUSE]) {
			std::cout << "WARNING: Attribute \"COL_DIFFUSE\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_DIFFUSE] = true;
		std::cout << "Color diffuse RGB: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_diffuse_xyz(float *col)
	{
		if (attr_set[COL_DIFFUSE]) {
			std::cout << "WARNING: Attribute \"COL_DIFFUSE\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_DIFFUSE] = true;
		std::cout << "Color diffuse XYZ: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_specular_rgb(float *col)
	{
		if (attr_set[COL_SPECULAR]) {
			std::cout << "WARNING: Attribute \"COL_SPECULAR\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_SPECULAR] = true;
		std::cout << "Color specular RGB: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_specular_xyz(float *col)
	{
		if (attr_set[COL_SPECULAR]) {
			std::cout << "WARNING: Attribute \"COL_SPECULAR\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_SPECULAR] = true;
		std::cout << "Color specular XYZ: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_tf_rgb(float *col)
	{
		if (attr_set[COL_TF]) {
			std::cout << "WARNING: Attribute \"COL_TF\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_TF] = true;
		std::cout << "Color tf RGB: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void col_tf_xyz(float *col)
	{
		if (attr_set[COL_TF]) {
			std::cout << "WARNING: Attribute \"COL_TF\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[COL_TF] = true;
		std::cout << "Color tf XYZ: " << col[0] << " " << col[1] << " " << col[2] << std::endl;
	}
	void tr(float val, bool halo)
	{
		if (attr_set[TR]) {
			std::cout << "WARNING: Attribute \"TR\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[TR] = true;
		std::cout << "Tr: " << val << (halo ? " +halo" : "") << std::endl;
	}

	void illum(int val)
	{
		if (attr_set[ILLUM]) {
			std::cout << "WARNING: Attribute \"ILLUM\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[ILLUM] = true;
		std::cout << "Illum model: " << val << std::endl;
	}
	void exp(int val)
	{
		if (attr_set[EXP]) {
			std::cout << "WARNING: Attribute \"EXP\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[EXP] = true;
		std::cout << "Exponent: " << val << std::endl;
	}
	void sharpness(int val)
	{
		if (attr_set[SHARPNESS]) {
			std::cout << "WARNING: Attribute \"SHARPNESS\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[SHARPNESS] = true;
		std::cout << "Sharpness: " << val << std::endl;
	}
	void density(float val)
	{
		if (attr_set[DENSITY]) {
			std::cout << "WARNING: Attribute \"DENSITY\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[DENSITY] = true;
		std::cout << "Density: " << val << std::endl;
	}

	void map_ambient(const MapArgs &args)
	{
		if (attr_set[MAP_AMBIENT]) {
			std::cout << "WARNING: Attribute \"MAP_AMBIENT\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_AMBIENT] = true;
		std::cout << "Map ambient: ";
		args.print(); std::cout << std::endl;
	}
	void map_diffuse(const MapArgs &args)
	{
		if (attr_set[MAP_DIFFUSE]) {
			std::cout << "WARNING: Attribute \"MAP_DIFFUSE\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_DIFFUSE] = true;
		std::cout << "Map diffuse: ";
		args.print(); std::cout << std::endl;
	}
	void map_specular(const MapArgs &args)
	{
		if (attr_set[MAP_SPECULAR]) {
			std::cout << "WARNING: Attribute \"MAP_SPECULAR\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_SPECULAR] = true;
		std::cout << "Map specular: ";
		args.print(); std::cout << std::endl;
	}
	void map_exp(const MapArgs &args)
	{
		if (attr_set[MAP_EXP]) {
			std::cout << "WARNING: Attribute \"MAP_EXP\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_EXP] = true;
		std::cout << "Map exp: ";
		args.print(); std::cout << std::endl;
	}
	void map_tr(const MapArgs &args)
	{
		if (attr_set[MAP_TR]) {
			std::cout << "WARNING: Attribute \"MAP_TR\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_TR] = true;
		std::cout << "Map tr: ";
		args.print(); std::cout << std::endl;
	}
	void map_decal(const MapArgs &args)
	{
		if (attr_set[MAP_DECAL]) {
			std::cout << "WARNING: Attribute \"MAP_DECAL\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_DECAL] = true;
		std::cout << "Map decal: ";
		args.print(); std::cout << std::endl;
	}
	void map_disp(const MapArgs &args)
	{
		if (attr_set[MAP_DISP]) {
			std::cout << "WARNING: Attribute \"MAP_DISP\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_DISP] = true;
		std::cout << "Map disp: ";
		args.print(); std::cout << std::endl;
	}
	void map_bump(const MapArgs &args)
	{
		if (attr_set[MAP_BUMP]) {
			std::cout << "WARNING: Attribute \"MAP_BUMP\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_BUMP] = true;
		std::cout << "Map bump: ";
		args.print(); std::cout << std::endl;
	}
	void map_aat(bool aat)
	{
		if (attr_set[MAP_AAT]) {
			std::cout << "WARNING: Attribute \"MAP_AAT\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_AAT] = true;
		std::cout << "Aat: " << aat << std::endl;
	}
	void map_refl(ReflType rt, const MapArgs &args)
	{
		if (attr_set[MAP_REFL]) {
			std::cout << "WARNING: Attribute \"MAP_REFL\" already set. Won't overwrite." << std::endl;
			return;
		}
		attr_set[MAP_REFL] = true;
		std::cout << "Refl: " << rt << " "	;
		args.print(); std::cout << std::endl;
	}
};


// 	for (int i = 0; i < mtllibs.size(); ++i) {
// 		std::string file = util::join_path(dir, mtllibs[i]);
// 		std::ifstream mtlis(file);
// 		if (mtlis.fail()) {
// 			std::cout << "WARNING: MTL file " << file << " not found" << std::endl;
// 			return;
// 		}
// 		read_mtllib(mtlis, mtlhandle);
// 	}
