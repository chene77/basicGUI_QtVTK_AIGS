/*=========================================================================

Program:   basic_qtVTK_AIGS
Module:    $RCSfile: mainWindows.cxx,v $
Creator:   Elvis C. S. Chen <chene@robarts.ca>
Language:  C++
Author:    $Author: Elvis Chen $
Date:      $Date: 2018/05/28 12:01:30 $
Version:   $Revision: 0.99 $

==========================================================================

Copyright (c) Elvis C. S. Chen, elvis.chen@gmail.com

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
form, must retain the above copyright notice, this license,
the following disclaimer, and any notices that refer to this
license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
notice, a copy of this license and the following disclaimer
in the documentation or with other materials provided with the
distribution.

3) Modified copies of the source code must be clearly marked as such,
and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

// local includes
#include "mainWindows.h"

// VTK includes
#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkButtonWidget.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkCoordinate.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkImageData.h>
#include <vtkImageExtractComponents.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkMatrix4x4.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkPNGWriter.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSimplePointsReader.h>
#include <vtkSimplePointsWriter.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkTexturedButtonRepresentation2D.h>
#include <vtkTransform.h>
#include <vtkTubeFilter.h>
#include <vtkWindowToImageFilter.h>
#include <vtkXMLPolyDataReader.h>


// tracker
#include <vtkNDITracker.h>
#include <vtkTrackerTool.h>

// QT includes
#include <QColorDialog>
#include <QDebug>
#include <QErrorMessage>
#include <QFileDialog>
#include <QLCDNumber>
#include <QMessageBox>
#include <QTimer>


template< class PReader > vtkPolyData *readAnPolyData(const char *fname) {
  vtkSmartPointer< PReader > reader =
    vtkSmartPointer< PReader >::New();
  reader->SetFileName(fname);
  reader->Update();
  reader->GetOutput()->Register(reader);
  return(vtkPolyData::SafeDownCast(reader->GetOutput()));
  }

basic_QtVTK::basic_QtVTK()
{
  this->setupUi(this);
  this->trackerWidget->hide();

  createVTKObjects();
  setupVTKObjects();
  setupQTObjects();
    
  ren->ResetCameraClippingRange();
  this->openGLWidget->GetRenderWindow()->Render();
}


void basic_QtVTK::createVTKObjects()
{
  actor = vtkSmartPointer<vtkActor>::New();
  myTracker = vtkSmartPointer< vtkNDITracker >::New();
  ren = vtkSmartPointer<vtkRenderer>::New();
  renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  stylusActor = vtkSmartPointer<vtkActor>::New();
  trackerDrawing = vtkSmartPointer<vtkImageCanvasSource2D>::New();
  trackerLogoRepresentation = vtkSmartPointer<vtkLogoRepresentation>::New();
  trackerLogoWidget = vtkSmartPointer<vtkLogoWidget>::New();
}


void basic_QtVTK::cleanVTKObjects()
{
  // if needed
  if (isTrackerInitialized)
    myTracker->StopTracking();
}


void basic_QtVTK::setupVTKObjects()
{
  // number of screenshot
  screenShotFileNumber = 0;

  // tracker
  this->isTrackerInitialized = isStylusCalibrated = false;
  trackedObjects.push_back(std::make_tuple(4, QString("D://chene//data//NDI_roms//8700248.rom"), enumTrackedObjectTypes::enStylus)); // NDI 3 sphere linear stylus
  trackedObjects.push_back(std::make_tuple(5, QString("D://chene//data//NDI_roms//8700302.rom"), enumTrackedObjectTypes::enOthers)); // NDI 4 sphere planar

  this->openGLWidget->SetRenderWindow(renWin);
  
  // VTK Renderer
  ren->SetBackground(.1, .2, .4);

  // connect VTK with Qt
  this->openGLWidget->GetRenderWindow()->AddRenderer(ren);

}


void basic_QtVTK::setupQTObjects()
{
  connect(action_Background_Color, SIGNAL(triggered()), this, SLOT(editRendererBackgroundColor()));
  connect(action_Quit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(actionLoad_Mesh, SIGNAL(triggered()), this, SLOT(loadMesh()));
  connect(actionLoad_Volume, SIGNAL(triggered()), this, SLOT(loadVolume()));
  connect(actionMesh_Color, SIGNAL(triggered()), this, SLOT(editMeshColor()));
  connect(actionScreen_Shot, SIGNAL(triggered()), this, SLOT(screenShot()));
  connect(actionthis_program, SIGNAL(triggered()), this, SLOT(aboutThisProgram()));
  connect(trackerButton, SIGNAL(toggled(bool)), this, SLOT(startTracker(bool)));
  connect(pivotButton, SIGNAL(toggled(bool)), this, SLOT(stylusCalibration(bool)));
  connect(actionLoad_Fiducial, SIGNAL(triggered()), this, SLOT(loadFiducialPts()));
  connect(collectSinglePtbutton, SIGNAL(clicked()), this, SLOT(collectSinglePointPhantom()));
  connect(resetPhantomPtButton, SIGNAL(clicked()), this, SLOT(resetPhantomCollectedPoints()));
  connect(deleteOnePhantomPtButton, SIGNAL(clicked()), this, SLOT(deleteOnePhantomCollectedPoints()));
  connect(phantomRegistrationButton, SIGNAL(clicked()), this, SLOT(performPhantomRegistration()));
}

void basic_QtVTK::startTracker(bool checked)
{
  if (checked)
    {
    // if tracker is not initialized, do so now
    if (!isTrackerInitialized)
      {
      myTracker->SetBaudRate(115200); /*!< Set the baud rate sufficiently high. */
      int nMax = myTracker->GetNumberOfTools();
      tools.resize(nMax);

      for (int i = 0; i < (int)trackedObjects.size(); i++) 
        {
        int port = std::get<0>(trackedObjects[i]);
        QString romName = std::get<1>(trackedObjects[i]);
        this->myTracker->LoadVirtualSROM(port, romName.toStdString().c_str());
        tools[i] = myTracker->GetTool(port);    
        qDebug() << "Loading" << romName << "into port" << port;
        }

      statusBar()->showMessage(tr("Tracking system NOT initialized."), 5000);
      if (myTracker->Probe()) /*!< Find the tracker. */
        {
        qDebug() << "Tracker Initialized";
        statusBar()->showMessage("Tracker Initialized", 5000);
        isTrackerInitialized = true;

        // enable the logo widget to display the status of each tracked object
        this->createTrackerLogo();
        trackerLogoWidget->On();
        this->openGLWidget->GetRenderWindow()->Render();

        // create a QTimer
        trackerTimer = new QTimer(this);
        connect(trackerTimer, SIGNAL(timeout()), this, SLOT(updateTrackerInfo()));

        }
      else
        {
        QErrorMessage *em = new QErrorMessage(this);
        em->showMessage("Tracker Initialization Failed");

        isTrackerInitialized = false;
        trackerButton->setChecked(false);
        }
      }

    if (isTrackerInitialized)
      {
      qDebug() << "Tracking started";
      statusBar()->showMessage("Tracking started.", 5000);
      myTracker->StartTracking();
      trackerTimer->start(0); // in milli-second. 0 is as fast as we can
      }
    }
  else
    {
    // button is un-toggled
    if (isTrackerInitialized)
      {
      trackerTimer->stop();

      myTracker->StopTracking();
    
      trackerLogoWidget->Off();
      this->openGLWidget->GetRenderWindow()->Render();
      statusBar()->showMessage("Tracking stopped.", 5000);
      }
    }
}


void basic_QtVTK::updateTrackerInfo()
{
  if (isTrackerInitialized)
    {
    myTracker->Update();

    for (int i = 0; i < (int)trackedObjects.size(); i++) 
      {
      if (tools[i]->IsMissing())
        {
        // not connected, shown in blue
        trackerDrawing->SetDrawColor(0, 0, 255);
        trackerDrawing->FillBox(logoWidgetX*i + i + 1, logoWidgetX*(i + 1) + i, 1, logoWidgetY + 1);
        }
      else if (tools[i]->IsOutOfVolume())
        {
        // connected, visible but not accurate, shown in yellow
        trackerDrawing->SetDrawColor(255, 255, 0);
        trackerDrawing->FillBox(logoWidgetX*i + i + 1, logoWidgetX*(i + 1) + i, 1, logoWidgetY + 1);
        }
      else if (tools[i]->IsOutOfView())
        {
        // connected, visible, but outside of the tracking volume. Shown in red
        trackerDrawing->SetDrawColor(255, 0, 0);
        trackerDrawing->FillBox(logoWidgetX*i + i + 1, logoWidgetX*(i + 1) + i, 1, logoWidgetY + 1);
        }
      else
        {
        // connected and withing good tracking accuracy. shown in green
        trackerDrawing->SetDrawColor(0, 255, 0);
        trackerDrawing->FillBox(logoWidgetX*i + i + 1, logoWidgetX*(i + 1) + i, 1, logoWidgetY + 1);
        }
      }

    trackerDrawing->Update();
    ren->ResetCameraClippingRange();
    this->openGLWidget->GetRenderWindow()->Render();
    }
}


void basic_QtVTK::loadFiducialPts()
{
  // fiducial is stored as lines of 3 floats
  QString fname = QFileDialog::getOpenFileName(this,
    tr("Open fiducial file"),
    QDir::currentPath(),
    "PolyData File (*.xyz)");
  vtkNew<vtkSimplePointsReader> reader;
  reader->SetFileName(fname.toStdString().c_str());
  reader->Update();

  fiducialPts = reader->GetOutput()->GetPoints();
  fiducialPts->Modified();

  qDebug() << "# of fiducial:" << fiducialPts->GetNumberOfPoints();
  statusBar()->showMessage(tr("Loaded fiducial file"));
}


void basic_QtVTK::loadVolume()
{
  std::cerr << "hello";
}

void basic_QtVTK::loadMesh()
  {
  QString fname = QFileDialog::getOpenFileName(this,
    tr("Open phantom mesh"),
    QDir::currentPath(),
    "PolyData File (*.vtk *.stl *.ply *.obj *.vtp)");

  // std::cerr << fname.toStdString().c_str() << std::endl;

  vtkPolyData *data;

  QFileInfo info(fname);
  bool knownFileType = true;

  // parse the file extension and use the appropriate reader
  if (info.suffix() == QString(tr("vtk")))
    {
    data = readAnPolyData<vtkPolyDataReader >(fname.toStdString().c_str());
    }
  else if (info.suffix() == QString(tr("stl")) || info.suffix() == QString(tr("stlb")))
    {
    data = readAnPolyData<vtkSTLReader>(fname.toStdString().c_str());
    }
  else if (info.suffix() == QString(tr("ply")))
    {
    data = readAnPolyData<vtkPLYReader>(fname.toStdString().c_str());
    }
  else if (info.suffix() == QString(tr("obj")))
    {
    data = readAnPolyData<vtkOBJReader>(fname.toStdString().c_str());
    }
  else if (info.suffix() == QString(tr("vtp")))
    {
    data = readAnPolyData<vtkXMLPolyDataReader>(fname.toStdString().c_str());
    }
  else
    {
    knownFileType = false;
    }

  if (knownFileType)
    {
    // do something only if we know the file type
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);

    actor->SetMapper(mapper);
    ren->AddActor(actor);

    // reset the camera according to visible actors
    ren->ResetCamera();
    ren->ResetCameraClippingRange();
    this->openGLWidget->GetRenderWindow()->Render();
    }
  else
    {
    QErrorMessage *em = new QErrorMessage(this);
    em->showMessage("Input file format not supported");
    }
}


void basic_QtVTK::stylusCalibration(bool checked)
{
  // assumes that there is only 1 stylus among all the tracked objects

  // make sure the tracker is initialized/found first.
  if (isTrackerInitialized)
    {
    // find out which port is the stylus
    int stylusPort = -1;
    int toolIdx = -1;
    for (int i = 0; i < (int)trackedObjects.size(); i++)
      {
      int port = std::get<0>(trackedObjects[i]);
      enumTrackedObjectTypes myType = std::get<2>(trackedObjects[i]);

      if (myType == enumTrackedObjectTypes::enStylus)
        {
        stylusPort = port;
        toolIdx = i;
        }
      }

    if (checked)
      {
      qDebug() << "Starting pivot calibration";
      double m[16] = { 1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1 };
      vtkNew<vtkMatrix4x4> matrix;
      matrix->DeepCopy(m);

      qDebug() << "stylus port:" << stylusPort << "index:" << toolIdx;
      tools[toolIdx]->SetCalibrationMatrix(matrix);
      tools[toolIdx]->InitializeToolTipCalibration();
      tools[toolIdx]->SetCollectToolTipCalibrationData(1);
      }
    else
      {
      tools[toolIdx]->SetCollectToolTipCalibrationData(0);
      this->stylusTipRMS->display(tools[toolIdx]->DoToolTipCalibration());
      isStylusCalibrated = true;
      qDebug() << "Pivot calibration finished";
      // tools[toolIdx]->Print(std::cerr);
      createLinearZStylusActor();
      }
    }
  else
    {
    // if tracker is not initialized, do nothing and uncheck the button
    pivotButton->setChecked(false);
    }
}


void basic_QtVTK::createTrackerLogo()
{
  logoWidgetX = 16;
  logoWidgetY = 10;

  int nObjects = (int)trackedObjects.size();

  trackerDrawing->SetScalarTypeToUnsignedChar();
  trackerDrawing->SetNumberOfScalarComponents(3);
  trackerDrawing->SetExtent(0, logoWidgetX*nObjects + nObjects, 0, logoWidgetY + 2, 0, 0);
  // Clear the image  
  trackerDrawing->SetDrawColor(255, 255, 255);
  trackerDrawing->FillBox(0, logoWidgetX*nObjects + nObjects, 0, logoWidgetY + 2);
  trackerDrawing->Update();

  trackerLogoRepresentation->SetImage(trackerDrawing->GetOutput());
  trackerLogoRepresentation->SetPosition(.45, 0);
  trackerLogoRepresentation->SetPosition2(.1, .1);
  trackerLogoRepresentation->GetImageProperty()->SetOpacity(.5);

  trackerLogoWidget->SetRepresentation(trackerLogoRepresentation);
  trackerLogoWidget->SetInteractor(this->openGLWidget->GetInteractor());
}


void basic_QtVTK::screenShot()
{
  // output the screen to PNG files.
  //
  // the file names are 0.png, 1.png, ..., etc.
  //
  QString fname = QString::number(screenShotFileNumber) + QString(tr(".png"));
  screenShotFileNumber++;

  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(this->openGLWidget->GetRenderWindow());
  w2i->ReadFrontBufferOff();
  w2i->SetInputBufferTypeToRGBA();

  vtkNew<vtkImageExtractComponents> iec;
  iec->SetInputConnection(w2i->GetOutputPort());
  iec->SetComponents(0, 1, 2);

  vtkNew<vtkPNGWriter> writer;
  writer->SetFileName(fname.toStdString().c_str());
  writer->SetInputConnection(iec->GetOutputPort());
  writer->Write();
}


void basic_QtVTK::editRendererBackgroundColor()
{
  QColor color = QColorDialog::getColor(Qt::gray, this);

  if (color.isValid())
    {
    int r, g, b;
    color.getRgb(&r, &g, &b);
    ren->SetBackground((double)r / 255.0, (double)g / 255.0, (double)b / 255.0);
    this->openGLWidget->GetRenderWindow()->Render();
    }
}


void basic_QtVTK::editMeshColor()
{
  QColor color = QColorDialog::getColor(Qt::gray, this);

  if (color.isValid())
    {
    int r, g, b;
    color.getRgb(&r, &g, &b);
    actor->GetProperty()->SetColor((double)r / 255.0, (double)g / 255.0, (double)b / 255.0);
    this->openGLWidget->GetRenderWindow()->Render();
    }  
}


void basic_QtVTK::slotExit()
{
  cleanVTKObjects(); // if needed
  qApp->exit();
}


void basic_QtVTK::aboutThisProgram()
{
  QMessageBox::about(this, tr("About basic_QtVTK"),
    tr("This is a demostration for Qt/VTK/AIGS integration\n\n"
      "By: \n\n"
      "Elvis C.S. Chen\t\t"
      "chene@robarts.ca"));
}


void basic_QtVTK::createLinearZStylusActor()
{
  double radius = 1.5; // mm
  int nSides = 36;
                         // find out which port is the stylus
  int stylusPort = -1;
  int toolIdx = -1;
  for (int i = 0; i < (int)trackedObjects.size(); i++)
    {
    int port = std::get<0>(trackedObjects[i]);
    enumTrackedObjectTypes myType = std::get<2>(trackedObjects[i]);

    if (myType == enumTrackedObjectTypes::enStylus)
      {
      stylusPort = port;
      toolIdx = i;
      }
    }

  vtkMatrix4x4 *calibMatrix = tools[toolIdx]->GetCalibrationMatrix();

  double *pos, *outpt;
  pos = new double[4];
  outpt = new double[4];

  pos[0] = pos[1] = 0.0;
  pos[2] = pos[3] = 1.0;
  calibMatrix->MultiplyPoint(pos, outpt);
  double l = sqrt(outpt[0] * outpt[0] + outpt[1] * outpt[1] + outpt[2] * outpt[2]);
  pos[0] = outpt[0] / l;
  pos[1] = outpt[1] / l;
  pos[2] = outpt[2] / l; // pos now is the normalized direction of the stylus

  vtkNew<vtkAppendPolyData> append;
  double coneHeight = 25.0;

  vtkNew<vtkLineSource> line;
  line->SetPoint1(coneHeight*pos[0], coneHeight*pos[1], coneHeight*pos[2]);
  line->SetPoint2(outpt[0], outpt[1], outpt[2]);

  vtkNew<vtkTubeFilter> tube;
  tube->SetInputConnection(line->GetOutputPort());
  tube->SetRadius(radius);
  tube->SetNumberOfSides(nSides);


  vtkNew<vtkConeSource> cone;
  cone->SetHeight(coneHeight);
  cone->SetRadius(radius);
  cone->SetDirection(-pos[0], -pos[1], -pos[2]);
  cone->SetResolution(nSides);
  cone->SetCenter(.5*coneHeight*pos[0], .5*coneHeight*pos[1], .5*coneHeight*pos[2]);
  //cone->SetCenter(outpt[0] - coneHeight*pos[0], outpt[1] - coneHeight*pos[1], outpt[2] - coneHeight*pos[2]);

  append->AddInputConnection(tube->GetOutputPort());
  append->AddInputConnection(cone->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(append->GetOutputPort());
  stylusActor->SetMapper(mapper);
  stylusActor->SetUserTransform(tools[toolIdx]->GetTransform());

  vtkNew<vtkNamedColors> color;
  stylusActor->GetProperty()->SetColor(color->GetColor3d("zinc_white").GetRed(),
    color->GetColor3d("zinc_white").GetGreen(),
    color->GetColor3d("zinc_white").GetBlue());

  ren->AddActor(stylusActor);

  delete[] pos;
  delete[] outpt;

}


void basic_QtVTK::collectSinglePointPhantom()
{
  qDebug() << "collect";
}


void basic_QtVTK::resetPhantomCollectedPoints()
{

  qDebug() << "reset";
}


void basic_QtVTK::deleteOnePhantomCollectedPoints()
{

  qDebug() << "delete";
}


void basic_QtVTK::performPhantomRegistration()
{
  qDebug() << "register";

}