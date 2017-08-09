/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glwidget.h"
#include "window.h"
#include "mainwindow.h"
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>

Window::Window(int maxverts, MainWindow *mw)
    : mainWindow(mw)
{
	glWidget = new GLWidget(maxverts, this);

	xSlider = createSlider();
	ySlider = createSlider(512);
//     zSlider = createSlider();

	connect(xSlider, &QSlider::valueChanged, glWidget, &GLWidget::set_delay);
//     connect(glWidget, &GLWidget::xRotationChanged, xSlider, &QSlider::setValue);
	connect(ySlider, &QSlider::valueChanged, glWidget, &GLWidget::set_history);
//     connect(glWidget, &GLWidget::yRotationChanged, ySlider, &QSlider::setValue);
//     connect(zSlider, &QSlider::valueChanged, glWidget, &GLWidget::setZRotation);
//     connect(glWidget, &GLWidget::zRotationChanged, zSlider, &QSlider::setValue);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	QHBoxLayout *container = new QHBoxLayout;
	container->addWidget(glWidget);
	container->addWidget(xSlider);
	container->addWidget(ySlider);
//     container->addWidget(zSlider);

	QWidget *w = new QWidget;
	w->setLayout(container);
	mainLayout->addWidget(w);
	dockBtn = new QPushButton(glWidget->paused ? tr("Play") : tr("Pause"), this);
	connect(dockBtn, &QPushButton::clicked, this, &Window::dockUndock);
	mainLayout->addWidget(dockBtn);

	changeBtn = new QPushButton(tr("Change view"), this);
	connect(changeBtn, &QPushButton::clicked, this, &Window::changeView);
	mainLayout->addWidget(changeBtn);

	curface = new QLabel(this);
	mainLayout->addWidget(curface);

	setLayout(mainLayout);

	xSlider->setValue(0);
	ySlider->setValue(0);
//     zSlider->setValue(0 * 16);

	setWindowTitle("Harry Compressor");
	curface->setText("0");
}

void Window::faceselected(int32_t f)
{
// 	std::cout << f << std::endl;
	curface->setText("Swag");
}

QSlider *Window::createSlider(int max)
{
	QSlider *slider = new QSlider(Qt::Vertical);
	slider->setRange(0, max);
	slider->setSingleStep(16);
	slider->setPageStep(15 * 16);
	slider->setTickInterval(15 * 16);
	slider->setTickPosition(QSlider::TicksRight);
	return slider;
}

void Window::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape) close();
	else QWidget::keyPressEvent(e);
}

void Window::dockUndock()
{
	glWidget->paused = !glWidget->paused;
	dockBtn->setText(glWidget->paused ? tr("Play") : tr("Pause"));
}
void Window::changeView()
{
	++glWidget->view;
	if (glWidget->view == 3) glWidget->view = 0;
}
