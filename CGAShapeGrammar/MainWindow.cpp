#include "MainWindow.h"
#include <QFileDialog>
#include "OBJWriter.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	ui.setupUi(this);

	QActionGroup* renderingModeGroup = new QActionGroup(this);
	renderingModeGroup->addAction(ui.actionViewBasicRendering);
	renderingModeGroup->addAction(ui.actionViewSSAO);
	renderingModeGroup->addAction(ui.actionViewLineRendering);
	renderingModeGroup->addAction(ui.actionViewHatching);
	renderingModeGroup->addAction(ui.actionViewSketchyRendering);

	ui.actionViewShadow->setChecked(true);
	ui.actionViewBasicRendering->setChecked(true);

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionOpenCGA, SIGNAL(triggered()), this, SLOT(onOpenCGA()));
	connect(ui.actionSaveGeometry, SIGNAL(triggered()), this, SLOT(onSaveGeometry()));
	connect(ui.actionViewShadow, SIGNAL(triggered()), this, SLOT(onViewShadow()));
	connect(ui.actionViewBasicRendering, SIGNAL(triggered()), this, SLOT(onViewRendering()));
	connect(ui.actionViewSSAO, SIGNAL(triggered()), this, SLOT(onViewRendering()));
	connect(ui.actionViewLineRendering, SIGNAL(triggered()), this, SLOT(onViewRendering()));
	connect(ui.actionViewHatching, SIGNAL(triggered()), this, SLOT(onViewRendering()));
	connect(ui.actionViewSketchyRendering, SIGNAL(triggered()), this, SLOT(onViewRendering()));
	connect(ui.actionViewRefresh, SIGNAL(triggered()), this, SLOT(onViewRefresh()));
	connect(ui.actionMCTS, SIGNAL(triggered()), this, SLOT(onMCTS()));
	connect(ui.actionMCMC, SIGNAL(triggered()), this, SLOT(onMCMC()));

	connect(ui.actionGenerateBuildingImages, SIGNAL(triggered()), this, SLOT(onGenerateBuildingImages()));

	glWidget = new GLWidget3D(this);
	setCentralWidget(glWidget);

	fileLoaded = false;
}

MainWindow::~MainWindow() {
}

void MainWindow::onOpenCGA() {
	QString new_filename = QFileDialog::getOpenFileName(this, tr("Open CGA file..."), "", tr("CGA Files (*.xml)"));
	if (new_filename.isEmpty()) return;

	fileLoaded = true;
	filename = new_filename;
	glWidget->loadCGA(filename.toUtf8().data());
	this->setWindowTitle("CGA Shape Grammar - " + new_filename);
}

void MainWindow::onSaveGeometry() {
	QString filename = QFileDialog::getSaveFileName(this, tr("Save OBJ file..."), "", tr("OBJ Files (*.obj)"));
	if (filename.isEmpty()) return;

	OBJWriter::write(glWidget->faces, filename.toUtf8().constData());
}

void MainWindow::onViewShadow() {
	glWidget->renderManager.useShadow = ui.actionViewShadow->isChecked();
	glWidget->updateGL();
}

void MainWindow::onViewRendering() {
	if (ui.actionViewBasicRendering->isChecked()) {
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_BASIC;
	}
	else if (ui.actionViewSSAO->isChecked()) {
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_SSAO;
	}
	else if (ui.actionViewLineRendering->isChecked()) {
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_LINE;
	}
	else if (ui.actionViewHatching->isChecked()) {
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_HATCHING;
	}
	else {
		glWidget->renderManager.renderingMode = RenderManager::RENDERING_MODE_SKETCHY;
	}
	glWidget->updateGL();
}

void MainWindow::onViewRefresh() {
	if (fileLoaded) {
		glWidget->loadCGA(filename.toUtf8().data());
	}
}

void MainWindow::onGenerateBuildingImages() {
	glWidget->generateBuildingImages(256, 256, true);
}

void MainWindow::onMCTS() {
	glWidget->runMCTS();
}

void MainWindow::onMCMC() {
	glWidget->runMCMC();
}