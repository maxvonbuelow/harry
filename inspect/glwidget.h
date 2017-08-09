#pragma once
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/glut.h>

#include <QOpenGLWidget>
#include <QOpenGLContext>

#include <QMatrix4x4>
#include <QTimer>
#include <thread>
#include <mutex>
#include <iostream>
#include "math.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>

#include <math.h>
#include <iostream>
#include <QtMath>
#include <unordered_set>




struct Shader {
	GLuint shader;
	Shader(const char *src, GLenum type) : shader(glCreateShader(type))
	{
		glShaderSource(shader, 1, &src, 0);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == GL_FALSE) {
			GLint maxlen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxlen);

			std::vector<GLchar> infolog(maxlen);
			glGetShaderInfoLog(shader, maxlen, &maxlen, infolog.data());

			throw std::runtime_error(std::string(infolog.data()));
		}
	}

	operator GLuint() const
	{
		return shader;
	}

	~Shader()
	{
		glDeleteShader(shader);
	}
};
struct ShaderProg {
	GLuint program;
	std::unordered_set<GLuint> attached;

	ShaderProg()
	{}

	void create()
	{
		program = glCreateProgram();
	}

	~ShaderProg()
	{
		detachall();
		glDeleteProgram(program);
	}

	operator GLuint() const
	{
		return program;
	}

	void attach(GLuint shader)
	{
		glAttachShader(program, shader);
		attached.insert(shader);
	}
	void detach(GLuint shader)
	{
		glDetachShader(program, shader);
		attached.erase(shader);
	}
	void detachall()
	{
		for (GLuint shader : attached) {
			glDetachShader(program, shader);
		}
		attached.clear();
	}
	void link()
	{
		glLinkProgram(program);

		GLint linked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE) {
			GLint maxlen = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxlen);

			std::vector<GLchar> infolog(maxlen);
			glGetProgramInfoLog(program, maxlen, &maxlen, infolog.data());

			throw std::runtime_error(infolog.data());
		}

		detachall();
	}
};

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget
{
	Q_OBJECT

public:
	GLWidget(int _maxverts, QWidget *parent = 0);
    ~GLWidget()
	{
		cleanup();
	}

	QSize minimumSizeHint() const Q_DECL_OVERRIDE
	{
		return QSize(200, 100);
	}
	QSize sizeHint() const Q_DECL_OVERRIDE
	{
		return QSize(600, 400);
	}

public slots:
	void cleanup();


public:
	int hist = 0;
	void set_history(int h);
protected:
	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;

	void resizeGL(int w, int h) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

public:

	void faceselected(int32_t f);
	void setupVertexAttribs();
	void add_verts(float *verts, int cnt);
	void add_new_verts();
	void push_vert_nolock(const float *v);
	void compute_normal(const float *v0, const float *v1, const float *v2, float *n) const;
	void push_face(const float *v0, const float *v1, const float *v2, const float *col);
	void push_line(const float *v0, const float *v1);
	void push_linein(const float *v0, const float *v1);
	QVector3D getArcBallVector(int x, int y);

	void set_scale_3(double abs_scale);
	void set_scale(double abs_scale);
	void set_rot(float *r);
	void set_rot_normal(float b3x, float b3y, float b3z);
	void set_rot(QMatrix4x4 rot);
	void set_center(double x, double y, double z);

	void set_delay(int d);
	int get_delay();

	void set_paused(bool p);
	bool get_paused();

	int off, maxverts, off_lines, off_linesin;
	std::vector<GLfloat> data;
	std::vector<GLfloat> datawf;
	std::vector<GLfloat> datawfin;
	std::mutex mutex;
	bool paused;
	int delay;
	QPoint lastPos;
	GLuint vao_mesh, vao_lines, vao_quad, vao_wf, vao_wfin;
	GLuint vbo_mesh, vbo_lines, vbo_quad, vbo_wf, vbo_wfin;
	ShaderProg program, program_nolight, program_pt, program_wf, program_grey;
	GLuint tex_col, rb_depth, rb_face;
	GLuint fbo;
	int last_fid;
	int view;

	GLuint tex_loc;

	int projMatrixLoc, projMatrixLocGrey;
	int mvMatrixLoc, mvMatrixLocGrey;
	int normalMatrixLoc, normalMatrixLocGrey;
	int lightPosLoc, lightPosLocGrey;
	int projMatrixLocNoLight;
	int mvMatrixLocNoLight;
	int projMatrixLocWf;
	int mvMatrixLocWf;
	double scale;
	QMatrix4x4 proj;
	QMatrix4x4 camera;
	QMatrix4x4 world;
	QMatrix4x4 matsc;
	QMatrix4x4 mattr;
	QMatrix4x4 matro;
	QTimer *repaint_timer;
};
