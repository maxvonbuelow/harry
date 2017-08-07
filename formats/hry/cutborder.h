#pragma once

// #include <stack>
#include "../../assert.h"

// TODO:
// zero triangles
// vertices w/o faces

#define DFS

template <typename T>
struct DataTpl : T {
	int idx;

	DataTpl() : idx(-1)
	{}
	template <typename ...Params>
	DataTpl(int i, Params &&...params) : idx(i), T(std::forward<Params>(params)...)
	{}

	bool isUndefined()
	{
		return idx == -1;
	}
};
template <typename T>
std::ostream &operator<<(std::ostream &os, const DataTpl<T> &d)
{
	return os << d.idx << " [" << (T)d << "]";
}

template <typename T>
struct ElementTpl {
	ElementTpl<T> *prev, *next;
	DataTpl<T> data;

	bool isEdgeBegin;

	ElementTpl() : isEdgeBegin(true)
	{}
	ElementTpl(const DataTpl<T> &d) : data(d), isEdgeBegin(true)
	{}

	void set_prev(ElementTpl<T> *e)
	{
		prev = e;
		e->next = this;
	}
	void set_next(ElementTpl<T> *e)
	{
		next = e;
		e->prev = this;
	}
};

template <typename T>
struct PartTpl {
	ElementTpl<T> *rootElement;
	int nrVertices, nrEdges;

	PartTpl() : nrVertices(0), nrEdges(0)
	{}
};

struct CutBorderBase {
	enum INITOP {
		INIT,
		TRI100, TRI010, TRI001,
		TRI110, TRI101, TRI011,
		TRI111,
		EOM, IFIRST = INIT, ILAST = EOM
	};
	enum OP { BORDER, CONNBWD, SPLIT, UNION, NM, ADDVTX, CONNFWD, FIRST = BORDER, LAST = CONNFWD };
	static const char* op2str(OP op)
	{
		//"\xE2\x96\xB3", "\xE2\x96\xB3\xC2\xB9", "\xE2\x96\xB3\xC2\xB2", "\xE2\x96\xB3\xC2\xB3", 
		static const char *lut[] = { "_", "<", "\xE2\x88\x9E", "\xE2\x88\xAA", "~", "*", ">", "?" };
		return lut[op];
	}
};

template <typename T>
struct CutBorder : CutBorderBase {
	typedef ElementTpl<T> Element;
	typedef PartTpl<T> Part;
	typedef DataTpl<T> Data;
	using CutBorderBase::INITOP;
	using CutBorderBase::OP;
	using CutBorderBase::op2str;

	Part *parts, *part;
	Element *elements, *element;
	Element *emptyElements;
	int max_elements, max_parts;

// 	std::stack<int> swapped;
	int swapped;
	bool have_swap;

	// acceleration structure for fast lookup if a vertex is currently on the cutboder
	std::vector<int> vertices;

	CutBorder(int maxParts, int maxElems, int vertcnthint = 0) : max_elements(0), max_parts(1), vertices(vertcnthint, 0), have_swap(false)
	{
		parts = new Part[maxParts]();
		++maxElems;
		elements = new Element[maxElems]();
		element = elements;
		emptyElements = elements;

		// initialize links
		for (int i = 0; i < maxElems; ++i) {
			elements[i].next = i + 1 == maxElems ? NULL : elements + (i + 1);
			elements[i].prev = i - 1 <  0        ? NULL : elements + (i - 1);
		}
	}

	~CutBorder()
	{
		delete[] parts;
		delete[] elements;
	}

	bool atEnd()
	{
		return part == NULL && element == NULL;
	}

	Element *traverseStep(Data &v0, Data &v1)
	{
		v0 = element->data;
		v1 = element->next->data;
// 		return element->data;
		return element;
	}

	Element *traversalorder(Element *bfs, Element *dfs)
	{
#ifdef DFS
		return dfs;
#else
		return bfs;
#endif
	}
	void next(Element *bfs, Element *dfs)
	{
		Element *nxt = traversalorder(bfs, dfs);

		// TODO: return on DFS
		Element *beg = nxt;
		while (!nxt->isEdgeBegin) {
			nxt = nxt->next;
			assert_ne(beg, nxt);
		}
		element = nxt;
	}

	void activate_vertex(int i)
	{
		if (i >= vertices.size()) vertices.resize(i + 1, 0);
		++vertices[i];
	}
	void deactivate_vertex(int i)
	{
		--vertices[i];
	}

	Element *new_element(Data v)
	{
		activate_vertex(v.idx);
		Element *e = new (emptyElements) Element(v);
		assert_eq(e->isEdgeBegin, true);
		emptyElements = emptyElements->next;
		max_elements = std::max(max_elements, ++part->nrVertices);
		return e;
	}

	void del_element(Element *e, int n = 1)
	{
		if (n == 0) return;
		deactivate_vertex(e->data.idx);

		Element *nxt = e->next;
		emptyElements->set_prev(e);
		emptyElements = e;
		--part->nrVertices;
		assert_ge(part->nrVertices, 0);

		del_element(nxt, n - 1);
	}

	Element *get_element(int &edgecnt, int i, int p = 0)
	{
		edgecnt = 0;
		Element *e1;
		if (p != 0) e1 = (part - p)->rootElement;
		else e1 = element;

		if (i > 0) {
// 			e1 = e1->next;
			for (int j = 0; j < i; e1 = e1->next, ++j) {
				edgecnt += j && e1->isEdgeBegin ? 1 : 0;
			}
		} else {
			for (int j = 0; j < -i; e1 = e1->prev, ++j) {
				edgecnt += e1->prev->isEdgeBegin ? 1 : 0;
			}
		}

		return e1;
	}
	void find_element(Data v, int &i, int &p)
	{
		// this fn walks into both directions on the cutborder
		Element *l = element;
		Element *r = element->next;

		i = 0; p = 0;
		while (1) {
			if (r->data.idx == v.idx) {
				++i;
				return;
			} else if (l->data.idx == v.idx) {
				i = -i;
				return;
			}

			if (l == r || l->prev == r) {
				++p;
				assert_ge(part - p, parts);
				i = 0;
				l = (part - p)->rootElement;
				r = l->next;
			} else {
				l = l->prev;
				r = r->next;
				++i;
			}
		}
	}
// 	void find_element(Data v, int &i, int &p)
// 	{
// 		// this fn walks into both directions on the cutborder
// 		Element *l = element;
// 		Element *r = element;
// 
// 		i = 0; p = 0;
// 		while (1) {
// 			if (r->data.idx == v.idx) {
// // 				++i;
// 				return;
// 			}/* else if (l->data.idx == v.idx) {
// 				i = -i;
// 				return;
// 			}*/
// 			r = r->next;
// 			++i;
// 
// 			if (l == r) {
// 				++p;
// 				assert_ge(part - p, parts);
// 				i = 0;
// 				l = (part - p)->rootElement;
// 				r = l;
// 			}
// 		}
// 	}

	void new_part(Element *root)
	{
		++part;
		part->rootElement = root;
		max_parts = std::max(max_parts, (int)(part - parts) + 1);
	}
	void del_part()
	{
		assert_eq(part->nrVertices, 0);
		if (part != parts) {
			--part;
			next(part->rootElement, part->rootElement);
		} else {
			part = NULL;
			element = NULL;
		}
	}

	Element *initial(Data v0, Data v1, Data v2)
	{
		part = parts;
		Element *e0 = new_element(v0), *e1 = new_element(v1), *e2 = new_element(v2);
		e0->set_next(e1); e1->set_next(e2); e2->set_next(e0);

		part->nrEdges = 3;

		next(e0, e2);

		part->rootElement = element;

		return e0;
	}
	template <typename ...Params>
	void newVertex(Data v, Params &&...params)
	{
		Element *v0 = element;
		Element *v1 = new_element(v);
		v1->data.init(std::forward<Params>(params)...);
		Element *v2 = element->next;

		++part->nrEdges; // -1 + 2

		v0->set_next(v1);
		v2->set_prev(v1);

		next(v2, v1);
	}
	Data connectForward()
	{
// 		std::cout << "CONFWD ?? " << istri() << " " << part->nrEdges << " " << part->nrVertices << std::endl;
		Data d = !element->next->isEdgeBegin ? Data(-1) : element->next->next->data;
		if (istri()) {
			// destroy
			del_element(element, 3);
			part->nrEdges = 0;

			del_part();
		} else {
			element->isEdgeBegin = element->next->isEdgeBegin;
			Element *e0 = element;
			Element *e1 = element->next->next;
			--part->nrEdges; // -2 + 1
			del_element(element->next);
			e0->set_next(e1);

			next(e1, e0);
		}
		return d;
	}
	Data connectBackward()
	{
// 		std::cout << "CONBWD" << std::endl;
		Data d = !element->prev->isEdgeBegin ? Data(-1) : element->prev->data;
		if (istri()) {
			// destroy
			del_element(element, 3);
			part->nrEdges = 0;

			del_part();
		} else {
			std::swap(element->data, element->prev->data);
			element->isEdgeBegin = element->prev->isEdgeBegin;
			Element *e0 = element->prev->prev;
			Element *e1 = element;
			--part->nrEdges; // -2 + 1
			del_element(element->prev);
			e0->set_next(e1);

// 			next(e1->next, e0);
			next(e1->next, e1);
		}
		return d;
	}

	bool istri()
	{
		return part->nrEdges == 3 && part->nrVertices == 3;
	}

	OP border()
	{
		--part->nrEdges;
		if (part->nrEdges == 0) {
			element->isEdgeBegin = false;
			del_element(element, part->nrVertices);
			del_part();
		} else {
			if (part->nrVertices >= 1 && (part->nrVertices < 2 || element->prev->isEdgeBegin != element->next->isEdgeBegin)) {
				++part->nrEdges;
				if (!element->prev->isEdgeBegin) {
					connectBackward();
					return CONNBWD;
				} else if (!element->next->isEdgeBegin) {
					connectForward();
					return CONNFWD;
				}
			} else if (part->nrVertices >= 2 && !element->prev->isEdgeBegin && !element->next->isEdgeBegin) {
				element->isEdgeBegin = false;
				Element *n = element->next->next;
				element->prev->set_next(n);
				del_element(element, 2);
				element = n;
			} else
				element->isEdgeBegin = false;

			next(element->next, element->next);
		}

		return BORDER;
	}
	void preserveOrder()
	{
		return; // TODO
// 		if (!swapped.empty()) {
		if (have_swap) {
			Part *swapwith;
// 			do {
				swapwith = parts + swapped/*.top()*/;
				if (swapwith < part) {
// 					std::cout << "SWAP: " << (part - parts) << " " << swapped << std::endl;
					part->rootElement = element;
					std::swap(*part, *swapwith);
// 				swapped.pop();
					next(part->rootElement, part->rootElement);
				}
				have_swap = false;
// 			} while (swapwith >= part && !swapped.empty());
		}
	}

	template <typename ...Params>
	Data splitCutBorder(int i, Params &&...params)
	{
		int edgecnt;
		Element *e0 = element, *e1 = get_element(edgecnt, i);
		Element *newroot, *newtail;

		// setup connections
		newroot = e0->next;
		newtail = e1->prev;
		e0->set_next(e1);

		Element *split = new_element(e1->data);
		split->data.init(std::forward<Params>(params)...);
		newtail->set_next(split);
		split->set_next(newroot);

		// add part
		if (i > 0) {
// 			std::cout << "POS SPLIT " << edgecnt << " <-- edges " << i << std::endl;
			--i;
			part->rootElement = traversalorder(e1, e0);
			part->nrVertices -= i + 1;
			part->nrEdges -= edgecnt;
			new_part(newroot); // root unimportant -> will be overwritten; TODO: remove
// 			std::cout << "NRV: " << part->nrVertices << std::endl;
			part->nrVertices += i + 1;
			part->nrEdges += edgecnt + 1;

			next(newroot, split);
		} else {
// 			std::cout << "NEG SPLIT" << std::endl;
			i = -i;

			part->rootElement = traversalorder(newroot, split);
			part->nrVertices -= i + 1;
			part->nrEdges -= edgecnt;
			new_part(traversalorder(e1, e0)); // root unimportant -> will be overwritten; TODO: remove
			part->nrVertices += i + 1;
			part->nrEdges += edgecnt + 1;

			std::swap(*part, *(part - 1));
// 			swapped.push((part - 1) - parts);
			swapped = (part - 1) - parts;
			have_swap = true;

// 			next(e1, e0);
			next(newroot, split);
		}

		return e1->data;
	}
	template <typename ...Params>
	Data cutBorderUnion(int i, int p, Params &&...params)
	{
		int edgecnt;
		Element *e0 = element, *e1 = get_element(edgecnt, i, p);

		Element *newroot = element->next;
		Element *newtail = e1->prev;

		e0->set_next(e1);

		Element *un = new_element(e1->data);
		un->data.init(std::forward<Params>(params)...);
		newtail->set_next(un);
		un->set_next(newroot);

		(part - p)->nrVertices += part->nrVertices; part->nrVertices = 0;
		(part - p)->nrEdges += part->nrEdges + 1; part->nrEdges = 0;
		(part - p)->rootElement = traversalorder(newroot, un); // sometimes it's more efficient to remove this line
// 		std::cout << "--------------- PART OFFSET: " << p << std::endl;
		std::swap(*(part - p), *(part - 1)); // process the parts in correct traversal order
		del_part();

// 		next(newroot, newtail);

		return e1->data;
	}

	bool on_cut_border(int i)
	{
		return !!vertices[i];
	}

	template <typename ...Params>
	void init(Element *e, Params &&...params)
	{
		e->data.init(std::forward<Params>(params)...);
	}

	template <typename ...Params>
	bool findAndUpdate(Data v, int &i, int &p, OP &op, Params &&...params)
	{
		if (!on_cut_border(v.idx)) return false;
		find_element(v, i, p);
		bigassert(int tr; assert_eq(get_element(tr, i, p)->data.idx, v.idx);)

		if (p > 0) {
			op = UNION;
			Data res = cutBorderUnion(i, p, std::forward<Params>(params)...);
			assert_eq(res.idx, v.idx);
		} else {
			if (element->next->isEdgeBegin && element->next->next->data.idx == v.idx) {
				op = CONNFWD;
				connectForward();
			} else if (element->prev->isEdgeBegin && element->prev->data.idx == v.idx) {
				op = CONNBWD;
				connectBackward();
			} else if (i == 0) {
				// this can not happen
				assert_fail;
				return false;
			} else {
				op = SPLIT;
				Data res = splitCutBorder(i, std::forward<Params>(params)...);
				assert_eq(res.idx, v.idx);
			}
		}
		return true;
	}

	void draw(std::ostream &os, bool force = false)
	{
		bool write = false;
		Part *curpart = part;
		while (curpart) {
			if (write || force) os << "###" << (curpart - parts) << " (" << curpart->nrVertices << ", " << curpart->nrEdges << ")  ";
			Element *cur = curpart == part ? element : curpart->rootElement;
			Element *beg = cur;
			int edges = 0, vertices = 0;
			int inactives = 0;
			do {
				assert_eq(cur, cur->prev->next);
				assert_eq(cur, cur->next->prev);
				++vertices;
				if (!cur->isEdgeBegin) {
					++inactives;
					if (write || force) os << "{" << cur->data.idx << "-" << cur->next->data.idx << "} -> ";
				} else {
					inactives = 0;
					if (write || force) os << "[" << cur->data.idx << "-" << cur->next->data.idx << "] -> ";
					++edges;
				}
				assert_le(inactives, 1);
				cur = cur->next;
			} while (cur != beg);
			assert_eq(edges, curpart->nrEdges);
			assert_eq(vertices, curpart->nrVertices);
			if (curpart == parts) curpart = NULL;
			else --curpart;
		}
		if (write || force) os << std::endl;
	}
};
