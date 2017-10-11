/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include "models.h"
#include "transform.h"
#include "structs/mixing.h"

namespace hry {
namespace io {

struct writer {
	HryModels &models;
	arith::Encoder<> &coder;

	writer(HryModels &_models, arith::Encoder<> &_coder) : models(_models), coder(_coder)
	{}

	void order(int i)
	{
		models.order(i);
	}

	// Connectivity
	void initial(int ntri)
	{
		iop(cbm::INIT);
		numtri(ntri);
	}
	void tri100(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, cbm::TRI100); }
	void tri010(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, cbm::TRI010); }
	void tri001(int ntri, mesh::vtxidx_t v0) { tri1(ntri, v0, cbm::TRI001); }

	void tri110(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, cbm::TRI110); }
	void tri101(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, cbm::TRI101); }
	void tri011(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1) { tri2(ntri, v0, v1, cbm::TRI011); }

	void tri111(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, mesh::vtxidx_t v2) { tri3(ntri, v0, v1, v2, cbm::TRI111); }

	void end()
	{
		iop(cbm::EOM);
	}
	void border(cbm::OP bop = cbm::BORDER)
	{
		op(bop);
	}
	void newvertex(int ntri)
	{
		op(cbm::NEWVTX);
		numtri(ntri);
	}
	void connectforward(int ntri)
	{
		op(cbm::CONNFWD);
		numtri(ntri);
	}
	void connectbackward(int ntri)
	{
		op(cbm::CONNBWD);
		numtri(ntri);
	}
	void splitcutborder(int ntri, int i)
	{
		op(cbm::SPLIT);
		elem(i);
		numtri(ntri);
	}
	void cutborderunion(int ntri, int i, int p)
	{
		op(cbm::UNION);
		elem(i); part(p);
		numtri(ntri);
	}
	void nm(int ntri, mesh::vtxidx_t idx)
	{
		op(cbm::NM);
		vertid(idx);
		numtri(ntri);
	}

	// Attributes
	void attr_data(mixing::View e, mesh::listidx_t l)
	{
		attr_type(DATA, l);
		models.attr_data[l]->enc(coder, e);
	}
	void attr_type(AttrType type, mesh::listidx_t l)
	{
		models.attr_type[l]->template encode<uint8_t>(coder, type);
	}
	void attr_ghist(uint32_t idx, mesh::listidx_t l)
	{
		attr_type(HIST, l);
		models.attr_ghist[l]->template encode<uint32_t>(coder, idx);
	}
	void attr_lhist(uint16_t idx, mesh::listidx_t l)
	{
		attr_type(LHIST, l);
		models.attr_lhist[l]->template encode<uint16_t>(coder, idx);
	}
	void reg_face(mesh::regidx_t r)
	{
		models.conn_regface.template encode<uint16_t>(coder, r);
	}
	void reg_vtx(mesh::regidx_t r)
	{
		models.conn_regvtx.template encode<uint16_t>(coder, r);
	}

private:
	void tri1(int ntri, mesh::vtxidx_t v0, cbm::INITOP op)
	{
		iop(op);
		vertid(v0);
		numtri(ntri);
	}
	void tri2(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, cbm::INITOP op)
	{
		iop(op);
		vertid(v0);
		vertid(v1);
		numtri(ntri);
	}
	void tri3(int ntri, mesh::vtxidx_t v0, mesh::vtxidx_t v1, mesh::vtxidx_t v2, cbm::INITOP op)
	{
		iop(op);
		vertid(v0);
		vertid(v1);
		vertid(v2);
		numtri(ntri);
	}

	void iop(cbm::INITOP op)
	{
		models.conn_iop.encode(coder, op);
	}
	void op(cbm::OP op)
	{
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

	reader(HryModels &_models, arith::Decoder<> &_coder) : models(_models), coder(_coder)
	{}

	void order(int i)
	{
		models.order(i);
	}

	// Connectivity
	cbm::INITOP iop()
	{
		return models.conn_iop.template decode<cbm::INITOP>(coder);
	}
	cbm::OP op()
	{
		return models.conn_op.template decode<cbm::OP>(coder);
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
		return models.conn_vert.template decode<uint32_t>(coder);
	}
	uint16_t numtri()
	{
		return models.conn_numtri.template decode<uint16_t>(coder);
	}

	// Attributes
	void attr_data(mixing::View e, mesh::listidx_t l)
	{
		models.attr_data[l]->dec(coder, e);
	}
	AttrType attr_type(mesh::listidx_t l)
	{
		return (AttrType)models.attr_type[l]->template decode<uint8_t>(coder);
	}
	uint32_t attr_ghist(mesh::listidx_t l)
	{
		models.attr_ghist[l]->template decode<uint32_t>(coder);
	}
	uint16_t attr_lhist(mesh::listidx_t l)
	{
		models.attr_lhist[l]->template decode<uint16_t>(coder);
	}
	mesh::regidx_t reg_face()
	{
		return models.conn_regface.template decode<uint16_t>(coder);
	}
	mesh::regidx_t reg_vtx()
	{
		return models.conn_regvtx.template decode<uint16_t>(coder);
	}
};

}
}
