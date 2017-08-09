#include "glwidget.h"
#include "window.h"

#define PAPERMODE


GLWidget::GLWidget(int _maxverts, QWidget *parent) : QOpenGLWidget(parent), scale(0.5), maxverts(_maxverts), off(0), off_lines(0), off_linesin(0), last_fid(-1), view(0)
{
	QSurfaceFormat fmt = format();
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setVersion(3, 3);
	setFormat(fmt);

	this->repaint_timer = new QTimer(this);
	connect(repaint_timer, SIGNAL(timeout()), this, SLOT(update()));
	repaint_timer->start();

	set_paused(true);
	set_delay(0);

	matsc.setToIdentity();
	matro.setToIdentity();
	mattr.setToIdentity();
}

void GLWidget::cleanup()
{
	makeCurrent();
	glDeleteBuffers(1, &vbo_mesh);
	glDeleteBuffers(1, &vbo_lines);
	doneCurrent();
}

void GLWidget::set_history(int h)
{
	std::lock_guard<std::mutex> lk(mutex);
	hist = h;
}
void GLWidget::initializeGL()
{
	static const char *vertexShaderSource =
		"#version 330\n"
		"layout (location = 0) in vec4 vertex;\n"
		"layout (location = 1) in vec3 normal;\n"
		"layout (location = 2) in vec3 color;\n"

		"out vec3 vert;\n"
		"out vec3 vertNormal;\n"
		"out vec3 vertcol;\n"

		"uniform mat4 projMatrix;\n"
		"uniform mat4 mvMatrix;\n"
		"uniform mat3 normalMatrix;\n"
		//     "varying vec3 pos;\n"
		"void main() {\n"
		"   vert = vertex.xyz;\n"
		"   vertNormal = normalMatrix * normal;\n"
		"   vertcol = color;\n"
		"   gl_Position = projMatrix * mvMatrix * vertex;\n"
		"}\n";
	static const char *vertexShaderSourceGrey =
		"#version 330\n"
		"layout (location = 0) in vec4 vertex;\n"
		"layout (location = 1) in vec3 normal;\n"
		"layout (location = 2) in vec3 color;\n"

		"out vec3 vert;\n"
		"out vec3 vertNormal;\n"
		"out vec3 vertcol;\n"

		"uniform mat4 projMatrix;\n"
		"uniform mat4 mvMatrix;\n"
		"uniform mat3 normalMatrix;\n"
		//     "varying vec3 pos;\n"
		"void main() {\n"
		"   vert = vertex.xyz;\n"
		"   vertNormal = normalMatrix * normal;\n"
		"   vertcol = vec3(0.6, 0.6, 0.6);\n"
		"   gl_Position = projMatrix * mvMatrix * vertex;\n"
		"}\n";
	static const char *vertexShaderSourceNoLight =
		"#version 330\n"
		"layout (location = 0) in vec4 vertex;\n"
		"layout (location = 1) in vec3 color;\n"
		"out vec3 vert;\n"
		"out vec3 vertcol;\n"
		"uniform mat4 projMatrix;\n"
		"uniform mat4 mvMatrix;\n"
		//     "varying vec3 pos;\n"
		"void main() {\n"
		"   vert = vertex.xyz;\n"
		"   vertcol = color;\n"
		"   gl_Position = projMatrix * mvMatrix * vertex;\n"
		"}\n";
	static const char *vertexShaderSourceWf =
		"#version 330\n"
		"layout (location = 0) in vec4 vertex;\n"
		"layout (location = 1) in vec3 color;\n"
		"out vec3 vert;\n"
		"out vec3 vertcol;\n"
		"uniform mat4 projMatrix;\n"
		"uniform mat4 mvMatrix;\n"
		//     "varying vec3 pos;\n"
		"void main() {\n"
		"   vert = vertex.xyz;\n"
		"   vertcol = color;\n"
		"   vec4 mved = mvMatrix * vertex;\n"
		"   gl_Position = projMatrix * (mved + vec4(0, 0, 0.002, 0));\n"
		"}\n";

	static const char *fragmentShaderSource =
		"#version 330\n"
		"in highp vec3 vert;\n"
		"in highp vec3 vertNormal;\n"
		"in vec3 vertcol;\n"
		//     "in int gl_PrimitiveID;\n"
		//     "out vec4 cccol;\n"
		"layout (location = 0) out vec4 outcol;\n"
		    "layout (location = 1) out int faceid;\n"
		"uniform highp vec3 lightPos;\n"
		//     "varying vec3 pos;\n"
		"void main() {\n"
		"   highp vec3 L = normalize(lightPos - vert);\n"
		//     "vec3 vertNormal = normalize(cross(dFdx(pos), dFdy(pos));\n"
		"   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
		"   highp vec3 color = /*vec3(0.39, 0.39, 0.39)*/vertcol;\n"

		"   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
		"   outcol = vec4(col, 1.0);\n"
		    "   faceid = gl_PrimitiveID;\n"
		"}\n";
	static const char *fragmentShaderSourceNoLight =
		"#version 330\n"
		"in highp vec3 vert;\n"
		"in vec3 vertcol;\n"
		"layout (location = 0) out vec4 outcol;\n"
		"void main() {\n"
		"   outcol = vec4(vertcol, 1.0);\n"
		"}\n";
	static const char *fragmentShaderSourceWf =
		"#version 330\n"
		"in highp vec3 vert;\n"
		"in vec3 vertcol;\n"
		"layout (location = 0) out vec4 outcol;\n"
		"void main() {\n"
		"   outcol = vec4(vertcol, 1.0);\n"
		"}\n";

	static const char *ptvtx = 
		"#version 330\n"
		"layout (location = 0) in vec4 vertex;\n"
		"out vec2 uv;\n"
		"void main() {\n"
		"   gl_Position =  vertex;\n"
		"   uv = (vertex.xy+vec2(1,1))/2.0;\n"
		"}\n";
	static const char *ptfrag = 
		"#version 330\n"
		"layout(location = 0) out vec4 color;\n"
		"uniform sampler2D texture;\n"
		"in vec2 uv;\n"
		"void main() {\n"
		"   color = texture2D(texture, uv);\n"
		"}\n";


	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

	glClearColor(0, 0, 0, 1);

     glewExperimental = GL_TRUE; 
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw std::runtime_error("GLEW failed");
	if (!GLEW_VERSION_3_3)
		throw std::runtime_error("OpenGL version 3.3 not supported");

	program.create();
	program.attach(Shader(vertexShaderSource, GL_VERTEX_SHADER));
	program.attach(Shader(fragmentShaderSource, GL_FRAGMENT_SHADER));
	program.link();

	program_grey.create();
	program_grey.attach(Shader(vertexShaderSourceGrey, GL_VERTEX_SHADER));
	program_grey.attach(Shader(fragmentShaderSource, GL_FRAGMENT_SHADER));
	program_grey.link();

	program_wf.create();
	program_wf.attach(Shader(vertexShaderSourceWf, GL_VERTEX_SHADER));
	program_wf.attach(Shader(fragmentShaderSourceWf, GL_FRAGMENT_SHADER));
	program_wf.link();

	program_nolight.create();
	program_nolight.attach(Shader(vertexShaderSourceNoLight, GL_VERTEX_SHADER));
	program_nolight.attach(Shader(fragmentShaderSourceNoLight, GL_FRAGMENT_SHADER));
	program_nolight.link();

	program_pt.create();
	program_pt.attach(Shader(ptvtx, GL_VERTEX_SHADER));
	program_pt.attach(Shader(ptfrag, GL_FRAGMENT_SHADER));
	program_pt.link();

	glUseProgram(program);
	projMatrixLoc = glGetUniformLocation(program, "projMatrix");
	mvMatrixLoc = glGetUniformLocation(program, "mvMatrix");
	normalMatrixLoc = glGetUniformLocation(program, "normalMatrix");
	lightPosLoc = glGetUniformLocation(program, "lightPos");
	glUniform3f(lightPosLoc, 0, 0, 70);

	glUseProgram(program_grey);
	projMatrixLocGrey = glGetUniformLocation(program, "projMatrix");
	mvMatrixLocGrey = glGetUniformLocation(program, "mvMatrix");
	normalMatrixLocGrey = glGetUniformLocation(program, "normalMatrix");
	lightPosLocGrey = glGetUniformLocation(program, "lightPos");
	glUniform3f(lightPosLocGrey, 0, 0, 70);

	glUseProgram(program_nolight);
	projMatrixLocNoLight = glGetUniformLocation(program_nolight, "projMatrix");
	mvMatrixLocNoLight = glGetUniformLocation(program_nolight, "mvMatrix");

	glUseProgram(program_wf);
	projMatrixLocWf = glGetUniformLocation(program_nolight, "projMatrix");
	mvMatrixLocWf = glGetUniformLocation(program_nolight, "mvMatrix");

	glUseProgram(program_pt);
	tex_loc = glGetUniformLocation(program_pt, "texture");
// 	std::cout << tex_loc << std::endl;
//      GLenum bufs[] = { GL_FRONT_LEFT, GL_COLOR_ATTACHMENT0 };
//      glDrawBuffers(2, bufs);

// 	GLuint colorbf, fb;
// 
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenTextures(1, &tex_col); glGenRenderbuffers(1, &rb_depth); glGenRenderbuffers(1, &rb_face);
// 	GLint st1=glCheckFramebufferStatus(GL_FRAMEBUFFER);
// 	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
//     glBindFramebuffer(GL_FRAMEBUFFER,fb);
//     //attaching an image to render to
//     glGenRenderbuffers(1,&colorbf);
//     glBindRenderbuffer(GL_RENDERBUFFER,colorbf);
//     glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA8,600,512);
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,   GL_RENDERBUFFER, colorbf);
//     //creating depht buffer
//     glGenRenderbuffers(1, &depth_rb);
//     glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
//     glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 600, 512);
//     //attaching the depth buffer to the FBO
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
// //     st1=glCheckFramebufferStatus(GL_FRAMEBUFFER);
// 	std::cout << "Swag " << st1 << std::endl;


	{
		glGenVertexArrays(1, &vao_mesh);
		glBindVertexArray(vao_mesh);

		glGenBuffers(1, &vbo_mesh);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_mesh);
		glBufferData(GL_ARRAY_BUFFER, (long unsigned int)(maxverts * 1.25 + 1000ul) * 9ul * (long unsigned int)sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	}

#ifdef PAPERMODE
	{
		glGenVertexArrays(1, &vao_wf);
		glBindVertexArray(vao_wf);

		glGenBuffers(1, &vbo_wf);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wf);
		glBufferData(GL_ARRAY_BUFFER, (long unsigned int)(maxverts * 2 * 1.25 + 1000ul) * 3ul * (long unsigned int)sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	}

	{
		glGenVertexArrays(1, &vao_wfin);
		glBindVertexArray(vao_wfin);

		glGenBuffers(1, &vbo_wfin);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wfin);
		glBufferData(GL_ARRAY_BUFFER, (long unsigned int)(20000ul) * 3ul * (long unsigned int)sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	}
#endif

	{
		glGenVertexArrays(1, &vao_lines);
		glBindVertexArray(vao_lines);

		glGenBuffers(1, &vbo_lines);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
		glBufferData(GL_ARRAY_BUFFER, 4 * 2 * 6 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
		float lines[] = {
			0, 0, 0, 1, 0, 0,    100, 0, 0, 1, 0, 0,
			0, 0, 0, 0, 1, 0,    0, 100, 0, 0, 1, 0,
			0, 0, 0, 0, 0, 1,    0, 0, 100, 0, 0, 1
		};
		glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * 2 * 6 * sizeof(GLfloat), lines);
	}

	{
		glGenVertexArrays(1, &vao_quad);
		glBindVertexArray(vao_quad);
		
		static const GLfloat g_quad_vertex_buffer_data[] = { 
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			1.0f,  1.0f, 0.0f,
		};

		glGenBuffers(1, &vbo_quad);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	}

	setupVertexAttribs();

	camera.setToIdentity();
	camera.translate(0, 0, -1);

	setMouseTracking(true);
}
void GLWidget::paintGL()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (view == 2) {
		glUseProgram(program_grey);
		glUniformMatrix4fv(projMatrixLocGrey, 1, GL_FALSE, proj.constData());
		glUniformMatrix4fv(mvMatrixLocGrey, 1, GL_FALSE, (camera * world).constData());
		QMatrix3x3 normalMatrix = world.normalMatrix();
		glUniformMatrix3fv(normalMatrixLocGrey, 1, GL_FALSE, normalMatrix.constData());
	} else {
		glUseProgram(program);
		glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, proj.constData());
		glUniformMatrix4fv(mvMatrixLoc, 1, GL_FALSE, (camera * world).constData());
		QMatrix3x3 normalMatrix = world.normalMatrix();
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMatrix.constData());
	}

	add_new_verts();
	int cnt = std::max(off / 9 - hist, 0);
	{
		glBindVertexArray(vao_mesh);
		glDrawArrays(GL_TRIANGLES, 0, cnt);
	}

#ifndef PAPERMODE
	glUseProgram(program_nolight);
	glUniformMatrix4fv(projMatrixLocNoLight, 1, GL_FALSE, proj.constData());
	glUniformMatrix4fv(mvMatrixLocNoLight, 1, GL_FALSE, (camera * world).constData());

	{
		glBindVertexArray(vao_lines);
		glDrawArrays(GL_LINES, 0, 2 * 3 + (off != 0 ? 2 : 0));
	}
#endif

#ifdef PAPERMODE
	glUseProgram(program_wf);
	glUniformMatrix4fv(projMatrixLocWf, 1, GL_FALSE, proj.constData());
	glUniformMatrix4fv(mvMatrixLocWf, 1, GL_FALSE, (camera * world).constData());
	{
		glBindVertexArray(vao_wf);
		glDepthFunc(GL_LEQUAL);
		glVertexAttrib3f(1, 0, 0, 0);
		glDrawArrays(GL_LINES, 0, off_lines);
		glDepthFunc(GL_LESS);
	}
	if (view == 1) {
		glBindVertexArray(vao_wfin);
		glVertexAttrib3f(1, 0.5, 0.5, 0.5);
		glDrawArrays(GL_LINES, 0, off_linesin);
	}
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(program_pt);
#ifdef PAPERMODE
	glClearColor(1, 1, 1, 1.0);
#endif
// 	glViewport(0,0,400,400);


	glBindVertexArray(vao_quad);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_col);
	glUniform1i(tex_loc, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);
// 	glDisableVertexAttribArray(0);
}

void GLWidget::resizeGL(int w, int h)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glBindTexture(GL_TEXTURE_2D, tex_col);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_col, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rb_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_depth);

	glBindRenderbuffer(GL_RENDERBUFFER, rb_face);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_R32I, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, rb_face);

     GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
     glDrawBuffers(2, bufs);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "ERR" << std::endl;

	GLint st1=glCheckFramebufferStatus(GL_FRAMEBUFFER);

	proj.setToIdentity();
	proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
}
void GLWidget::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
}
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	QPoint p = mapFromGlobal(QCursor::pos());
	int x = p.x(), y = height() - p.y();
	GLfloat depth;
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	int32_t faceid;
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &faceid);
	if (depth != 1.0f && faceid != last_fid) faceselected(faceid);/* std::cout << faceid << std::endl;*/
	last_fid = faceid;

	if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
		QVector3D v = getArcBallVector(lastPos.x(),lastPos.y()); // from the mouse
		QVector3D u = -getArcBallVector(event->x(), event->y());

		float angle = std::acos(std::min(1.0f, QVector3D::dotProduct(u,v)));

		QVector3D rotAxis = QVector3D::crossProduct(v,u);

		QMatrix4x4 eye2ObjSpaceMat = matro.transposed();

		QVector3D objSpaceRotAxis = eye2ObjSpaceMat * rotAxis;
		
		double rotdeg = 4 * qRadiansToDegrees(angle);
		matro.rotate(rotdeg, objSpaceRotAxis);
		set_rot(matro);
	}
	lastPos = event->pos();
}
void GLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	makeCurrent();
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	QPoint p = mapFromGlobal(QCursor::pos());
	int x = p.x(), y = height() - p.y();
	GLfloat depth;
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	if (depth == 1.0f) return;
	GLdouble wx, wy, wz;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	double modelMatrix[16];
	double projMatrix[16];
	QMatrix4x4 mv = camera * world;
	for (int i = 0; i < 16; ++i) {
		modelMatrix[i] = mv.constData()[i];
	}
	for (int i = 0; i < 16; ++i) {
		projMatrix[i] = proj.constData()[i];
	}

	gluUnProject( x, y, depth, modelMatrix, projMatrix, viewport, &wx, &wy, &wz);
	set_center(wx, wy, wz);
}
void GLWidget::wheelEvent(QWheelEvent *event)
{
	if (event->orientation() == Qt::Vertical) set_scale_3(scale + (double)(event->delta()) / 5000);
}
void GLWidget::faceselected(int32_t f)
{
// 	((Window*)this->parent())->faceselected(f);
}

void GLWidget::setupVertexAttribs()
{
	{
		glBindVertexArray(vao_mesh);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_mesh);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
	}

	{
		glBindVertexArray(vao_wf);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}

	{
		glBindVertexArray(vao_wfin);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wfin);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}

	{
		glBindVertexArray(vao_lines);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	}

	{
		glBindVertexArray(vao_quad);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
}
void GLWidget::add_verts(float *verts, int cnt)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo_mesh);
	glBufferSubData(GL_ARRAY_BUFFER, off * sizeof(GLfloat), cnt * sizeof(GLfloat), verts);
	off += cnt;
}

void GLWidget::add_new_verts()
{
	std::lock_guard<std::mutex> lk(mutex);
	if (!data.empty()) {
		add_verts(data.data(), data.size());

		// pointer
		GLfloat *lastface = data.data() + data.size() - 9 * 3;

		GLfloat ptrdata[12] = { 0, 0, 0, 1, 0, 0,    0, 0, 0, 1, 0, 0 };
		GLfloat min[3] = { INFINITY, INFINITY, INFINITY };
		GLfloat max[3] = { -INFINITY, -INFINITY, -INFINITY };
		for (int i = 0; i < 3; ++i) { // it over vtx
			for (int j = 0; j < 3; ++j) { // it over coordinate
				ptrdata[j] += lastface[i * 9 + j];
				min[j] = std::min(min[j], lastface[i * 9 + j]);
				max[j] = std::max(max[j], lastface[i * 9 + j]);
			}
		}
		GLfloat d[3];
		for (int i = 0; i < 3; ++i) d[i] = max[i] - min[i];
		GLfloat metric = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * 2;
		for (int j = 0; j < 3; ++j) {
			ptrdata[j] /= 3;
			ptrdata[j + 6] = ptrdata[j] + lastface[3 + j] * metric; // add normal
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
		glBufferSubData(GL_ARRAY_BUFFER, 3 * 12 * sizeof(GLfloat), 12 * sizeof(GLfloat), ptrdata);

		data.clear();
	}
#ifdef PAPERMODE
	if (!datawf.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wf);
		glBufferSubData(GL_ARRAY_BUFFER, off_lines * sizeof(GLfloat), datawf.size() * sizeof(GLfloat), datawf.data());

		off_lines += datawf.size();
		datawf.clear();
	}
	if (!datawfin.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_wfin);
		glBufferSubData(GL_ARRAY_BUFFER, off_linesin * sizeof(GLfloat), datawfin.size() * sizeof(GLfloat), datawfin.data());

		off_linesin += datawfin.size();
		datawfin.clear();
	}
#endif
}
void GLWidget::push_vert_nolock(const float *v) {
	for (int i = 0; i < 3; ++i) data.push_back(v[i]);
}
void GLWidget::compute_normal(const float *v0, const float *v1, const float *v2, float *n) const
{
	float u[3], v[3];
	for (int i = 0; i < 3; ++i) {
		u[i] = v0[i] - v2[i];
		v[i] = v0[i] - v1[i];
	}
	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
	float nl = -std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
	for (int i = 0; i < 3; ++i) n[i] /= nl;
}
void GLWidget::push_face(const float *v0, const float *v1, const float *v2, const float *col)
{
	std::lock_guard<std::mutex> lk(mutex);

	float n[3];
	compute_normal(v0, v1, v2, n);

	const float *v[] = { v0, v1, v2 };
	for (int i = 0; i < 3; ++i) {
		push_vert_nolock(v[i]);
		push_vert_nolock(n);
		push_vert_nolock(col);
	}
}
void GLWidget::push_line(const float *v0, const float *v1)
{
	std::lock_guard<std::mutex> lk(mutex);

	for (int i = 0; i < 3; ++i) datawf.push_back(v0[i]);
	for (int i = 0; i < 3; ++i) datawf.push_back(v1[i]);
}
void GLWidget::push_linein(const float *v0, const float *v1)
{
	std::lock_guard<std::mutex> lk(mutex);

	for (int i = 0; i < 3; ++i) datawfin.push_back(v0[i]);
	for (int i = 0; i < 3; ++i) datawfin.push_back(v1[i]);
}
QVector3D GLWidget::getArcBallVector(int x, int y)
{
	QVector3D pt = QVector3D(2.0 * x / GLWidget::width() - 1.0, 2.0 * y / GLWidget::height() - 1.0 , 0);
	pt.setY(pt.y() * -1);

	float xySquared = pt.x() * pt.x() + pt.y() * pt.y();

	if (xySquared <= 1.0)
		pt.setZ(std::sqrt(1.0 - xySquared));
	else
		pt.normalize();

	return pt;
}

void GLWidget::set_scale_3(double abs_scale)
{
	std::lock_guard<std::mutex> lk(mutex);
	matsc.setToIdentity();
	matsc.scale(abs_scale * abs_scale * abs_scale);
	world = matsc * matro * mattr;
	scale = abs_scale;
	update();
}
void GLWidget::set_scale(double abs_scale)
{
	set_scale_3(pow(abs_scale, 1.0 / 3.0));
}
void GLWidget::set_rot(float *r)
{
	QMatrix4x4 rot;
	for (int i = 0; i < 16; ++i) {
		rot.data()[i] = r[i];
	}
	set_rot(rot);
}
void GLWidget::set_rot_normal(float b3x, float b3y, float b3z)
{
	b3x = -b3x;
	b3y = -b3y;
	b3z = -b3z;

	float tx = 1, ty = 0, tz = 0;
	float b1x = b3y * tz - b3z * ty;
	float b1y = b3z * tx - b3x * tz;
	float b1z = b3x * ty - b3y * tx;
	if (b1x * b1x + b1y * b1y + b1z * b1z < 0.001) {
		tx = 0; ty = 1; tz = 0;
		b1x = b3y * tz - b3z * ty;
		b1y = b3z * tx - b3x * tz;
		b1z = b3x * ty - b3y * tx;
	}
	float b1l = sqrt(b1x * b1x + b1y * b1y + b1z * b1z);
	b1x /= b1l; b1y /= b1l; b1z /= b1l;

	float b2x = b3y * b1z - b3z * b1y;
	float b2y = b3z * b1x - b3x * b1z;
	float b2z = b3x * b1y - b3y * b1x;
	float b2l = sqrt(b2x * b2x + b2y * b2y + b2z * b2z);
	b2x /= b2l; b2y /= b2l; b2z /= b2l;

	float rot[] = { b1x, b2x, b3x, 0,
					b1y, b2y, b3y, 0,
					b1z, b2z, b3z, 0,
					0, 0, 0, 1 };

	set_rot(rot);
}
void GLWidget::set_rot(QMatrix4x4 rot)
{
	std::lock_guard<std::mutex> lk(mutex);
	matro = rot;
	world = matsc * matro * mattr;
	update();
}
void GLWidget::set_center(double x, double y, double z)
{
	std::lock_guard<std::mutex> lk(mutex);
	mattr.setToIdentity();
	mattr.translate(-x, -y, -z);
	world = matsc * matro * mattr;
	update();
}

void GLWidget::set_delay(int d)
{
	std::lock_guard<std::mutex> lk(mutex);
	delay = d;
}
int GLWidget::get_delay() {
	std::lock_guard<std::mutex> lk(mutex);
	return delay;
}

void GLWidget::set_paused(bool p) {
	std::lock_guard<std::mutex> lk(mutex);
	paused = p;
}
bool GLWidget::get_paused() {
	std::lock_guard<std::mutex> lk(mutex);
	return paused;
}
