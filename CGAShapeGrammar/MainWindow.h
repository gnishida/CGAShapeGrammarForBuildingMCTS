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
	void onGenerateBuildingImages();
	void onMCTS();
	void onMCMC();
};

#endif // MAINWINDOW_H
