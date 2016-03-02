#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "GLWidget3D.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
		
private:
	Ui::MainWindowClass ui;
	GLWidget3D* glWidget;
	bool fileLoaded;
	QString filename;

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:
	void onOpenCGA();
	void onSaveGeometry();
	void onViewShadow();
	void onViewRendering();
	void onViewRefresh();
	void onRotationStart();
	void onRotationEnd();
	void onGenerateBuildingImages();
	void camera_update();
};

#endif // MAINWINDOW_H
