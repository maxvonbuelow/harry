#pragma once

#include "../../structs/mixing.h"
#include "models.h"
#include "transform.h"

enum AttrType { DATA, HIST, LHIST };

struct writer {
	HryModels &models;
	arith::Encoder<> &coder;

	writer(HryModels &_models, arith::Encoder<> &_coder) : models(_models), coder(_coder)
	{}

	void order(int i)
	{
		i=0;
		models.order(i);
	}

	std::ofstream os = std::ofstream("dbg.op");

	// Connectivity
	void initial(int ntri)
	{
		iop(CutBorderBase::INIT);
		numtri(ntri);
	}
	void tri100(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, CutBorderBase::TRI100); }
	void tri010(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, CutBorderBase::TRI010); }
	void tri001(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, CutBorderBase::TRI001); }

	void tri110(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, CutBorderBase::TRI110); }
	void tri101(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, CutBorderBase::TRI101); }
	void tri011(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, CutBorderBase::TRI011); }

	void tri111(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, mesh::vtxidx_t v2) { tri3(ntri, v0, v1, v2, CutBorderBase::TRI111); }

	void end()
	{
		iop(CutBorderBase::EOM);
	}
	void border(CutBorderBase::OP bop = CutBorderBase::BORDER)
	{
		op(bop);
	}
	void newvertex(int ntri)
	{
		op(CutBorderBase::ADDVTX);
		numtri(ntri);
	}
	void connectforward(int ntri)
	{
		op(CutBorderBase::CONNFWD);
		numtri(ntri);
	}
	void connectbackward(int ntri)
	{
		op(CutBorderBase::CONNBWD);
		numtri(ntri);
	}
	void splitcutborder(int ntri, int i)
	{
		op(CutBorderBase::SPLIT);
		elem(i);
		numtri(ntri);
	}
	void cutborderunion(int ntri, int i, int p)
	{
		op(CutBorderBase::UNION);
		elem(i); part(p);
		numtri(ntri);
	}
	void nm(int ntri, mesh::vtxidx_t idx)
	{
		op(CutBorderBase::NM);
		vertid(idx);
		numtri(ntri);
	}
	void reg_face(mesh::regidx_t r)
	{
		models.conn_regface.template encode<uint16_t>(coder, r);
	}
	void reg_vtx(mesh::regidx_t r)
	{
		models.conn_regvtx.template encode<uint16_t>(coder, r);
	}

// 	int attrc = 0;
	// Attributes
	void attr_data(mixing::View e, mesh::listidx_t l)
	{
		std::cout << l << std::endl;
// 		++attrc;
// 		std::cout << attrc << std::endl;
		models.attr_data[l]->enc(coder, e);
	}

private:
	void tri1(int ntri, mesh::vtxidx_t v0, CutBorderBase::INITOP op)
	{
		iop(op);
		vertid(v0);
		numtri(ntri);
	}
	void tri2(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, CutBorderBase::INITOP op)
	{
		iop(op);
		vertid(v0);
		vertid(v1);
		numtri(ntri);
	}
	void tri3(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, mesh::vtxidx_t v2, CutBorderBase::INITOP op)
	{
		iop(op);
		vertid(v0);
		vertid(v1);
		vertid(v2);
		numtri(ntri);
	}

	void iop(CutBorderBase::INITOP op)
	{
		os << "i " << op << std::endl;
		models.conn_iop.encode(coder, op);
	}
	void op(CutBorderBase::OP op)
	{
		os << "o " << op << std::endl;
		models.conn_op.encode(coder, op);
	}

	void elem(int i)
	{
		models.conn_elem.template encode<uint32_t>(coder, transform::zigzag_encode(i));
	}
	void part(int p)
	{
		models.conn_part.template encode<uint16_t>(coder, p);
	}
	void vertid(mesh::vtxidx_t v)
	{
		std::cout << "Encoding: " << std::hex << v << std::endl;
		models.conn_vert.template encode<uint32_t>(coder, v);
	}
	void numtri(int n)
	{
		if (n != 0) models.conn_numtri.template encode<uint16_t>(coder, n);
	}
};

struct reader {
	HryModels &models;
	arith::Decoder<> &coder;

	std::ifstream is = std::ifstream("dbg.op");

	reader(HryModels &_models, arith::Decoder<> &_coder) : models(_models), coder(_coder)
	{}

	void order(int i)
	{
		i=0;
		models.order(i);
	}

	// Connectivity
	CutBorderBase::INITOP iop()
	{
		auto x = models.conn_iop.template decode<CutBorderBase::INITOP>(coder);
		std::string id; int y;
		is >> id >> y;
		if (id != "i" || x != y) { std::cout <<std::endl << "ERR@i: " << id << " || " << CutBorderBase::iop2str(x) << " " << CutBorderBase::iop2str((decltype(x))y) << std::endl; std::exit(1); }
		std::cout << "CURIOP: " << CutBorderBase::iop2str(x) << std::endl;
// 		if (x == CutBorderBase::TRI111) std::cout << "HERE" << std::endl;
		return x;
	}
	CutBorderBase::OP op()
	{
		std::cout << "decoding op" << std::endl;
		auto x = models.conn_op.template decode<CutBorderBase::OP>(coder);
		std::string id; int y;
		is >> id >> y;
		if (id != "o" || x != y) { std::cout << "ERR@o: " << id << " " << CutBorderBase::op2str(x) << " " << CutBorderBase::op2str((decltype(x))y) << std::endl; std::exit(1); }
		std::cout << "CUROP: " << CutBorderBase::op2str(x) << std::endl;
		return x;
	}
	uint32_t elem()
	{
		return transform::zigzag_decode(models.conn_elem.template decode<uint32_t>(coder));
	}
	uint16_t part()
	{
		return models.conn_part.template decode<uint16_t>(coder);
	}
	mesh::vtxidx_t vertid()
	{
		auto x = models.conn_vert.template decode<uint32_t>(coder);
		std::cout << "Decoding: " << std::hex << x << std::endl;
		return x;
	}
	uint16_t numtri()
	{
		return models.conn_numtri.template decode<uint16_t>(coder);
	}

	mesh::regidx_t reg_face()
	{
		return models.conn_regface.template decode<uint16_t>(coder);
	}
	mesh::regidx_t reg_vtx()
	{
		return models.conn_regvtx.template decode<uint16_t>(coder);
	}

	// Attributes
// 	int attrc = 0;
	void attr_data(mixing::View e, mesh::listidx_t l)
	{
// 		std::cout << "THESTORE: " << l << std::endl;
// 		++attrc;
// 		std::cout << attrc << std::endl;
// 		std::cout << "ast?" << std::endl;
		models.attr_data[l]->dec(coder, e);
// 		std::cout << "ast2?" << std::endl;
	}
};
