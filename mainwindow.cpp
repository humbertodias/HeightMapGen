#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openglwidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <cmath>
#include <algorithm>
#include <QEvent>
#include <QDebug>
#include <QLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QWidget>
#include <QMenuBar>
#include <QScrollArea>
#include <QLabel>
#include <QSize>
#include <random>
#include <numeric>
#include <chrono>
#include <sstream>
#include <QCheckBox>  // AGREGAR ESTA LÍNEA
#include <QSlider>    // AGREGAR ESTA LÍNEA TAMBIÉN
#include <queue>
#include <QListWidget>
#include <QColorDialog>
#include <QColorSpace>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{  // <-- LLAVE DE APERTURA (debe estar aquí)
    ui->setupUi(this);
    originalWindowWidth = this->width();
    originalWindowHeight = this->height();
    this->setWindowTitle("HeightMapGen - Editor de Mapas de Altura");
    if (ui->scrollAreaDisplay) {
        ui->scrollAreaDisplay->installEventFilter(this);
    }

    isPainting = false;
    mapWidth = 0;
    mapHeight = 0;
    brushHeight = 128;
    dynamicImageLabel = nullptr;

    ui->lineEditWidth->setText("512");
    ui->lineEditHeight->setText("512");

    if (ui->sliderBrushSize) {
        ui->sliderBrushSize->setRange(1, 100);
        ui->sliderBrushSize->setValue(10);
    }

    if (ui->sliderBrushIntensity) {
        ui->sliderBrushIntensity->setRange(1, 100);
        ui->sliderBrushIntensity->setValue(50);
    }

    if (ui->spinBoxOctaves) {
        ui->spinBoxOctaves->setRange(1, 10);
        ui->spinBoxOctaves->setValue(6);
    }
    if (ui->doubleSpinBoxPersistence) {
        ui->doubleSpinBoxPersistence->setRange(0.1, 0.9);
        ui->doubleSpinBoxPersistence->setSingleStep(0.05);
        ui->doubleSpinBoxPersistence->setValue(0.55);
    }
    if (ui->doubleSpinBoxFrequencyScale) {
        ui->doubleSpinBoxFrequencyScale->setRange(1.0, 50.0);
        ui->doubleSpinBoxFrequencyScale->setSingleStep(0.5);
        ui->doubleSpinBoxFrequencyScale->setValue(8.0);
        frequencyScale = 8.0;
    }
    if (ui->lineEditOffset) {
        ui->lineEditOffset->setText("Aleatorio");
    }

    // Inicializar slider de color de relleno
    if (ui->sliderFillColor) {
        ui->sliderFillColor->setRange(0, 255);
        ui->sliderFillColor->setValue(128);
        brushColor = 128;

        connect(ui->sliderFillColor, &QSlider::valueChanged, this, [this](int value) {
            brushColor = value;
            if (ui->labelFillColorPreview) {
                QString styleSheet = QString("background-color: rgb(%1, %1, %1); border: 1px solid black;").arg(value);
                ui->labelFillColorPreview->setStyleSheet(styleSheet);
            }
        });
    }  // <-- ASEGÚRATE DE QUE ESTE IF CIERRA CORRECTAMENTE

    // ============================================
    // CREAR MENÚ PRINCIPAL (QMenuBar)
    // ============================================

    // Menú Archivo
    QMenu *menuArchivo = menuBar()->addMenu("Archivo");

    QAction *actionNuevo = menuArchivo->addAction("Nuevo Mapa");
    actionNuevo->setShortcut(QKeySequence::New); // Ctrl+N
    connect(actionNuevo, &QAction::triggered, this, &MainWindow::on_pushButtonCreate_clicked);

    QAction *actionAbrir = menuArchivo->addAction("Cargar Mapa");
    actionAbrir->setShortcut(QKeySequence::Open); // Ctrl+O
    connect(actionAbrir, &QAction::triggered, this, &MainWindow::on_pushButtonLoad_clicked);

    QAction *actionGuardar = menuArchivo->addAction("Guardar Mapa");
    actionGuardar->setShortcut(QKeySequence::Save); // Ctrl+S
    connect(actionGuardar, &QAction::triggered, this, &MainWindow::on_pushButtonSave_clicked);

    menuArchivo->addSeparator();

    QAction *actionImportar = menuArchivo->addAction("Importar 3D...");
    connect(actionImportar, &QAction::triggered, this, &MainWindow::on_pushButtonImport3D_clicked);

    QAction *actionExportar = menuArchivo->addAction("Exportar 3D...");
    connect(actionExportar, &QAction::triggered, this, &MainWindow::on_pushButtonExport3D_clicked);

    menuArchivo->addSeparator();

    QAction *actionSalir = menuArchivo->addAction("Salir");
    actionSalir->setShortcut(QKeySequence::Quit); // Ctrl+Q
    connect(actionSalir, &QAction::triggered, this, &QMainWindow::close);

    // Menú Edición
    QMenu *menuEdicion = menuBar()->addMenu("Edición");

    QAction *actionDeshacer = menuEdicion->addAction("Deshacer");
    actionDeshacer->setShortcut(QKeySequence::Undo); // Ctrl+Z
    connect(actionDeshacer, &QAction::triggered, this, &MainWindow::undo);

    QAction *actionRehacer = menuEdicion->addAction("Rehacer");
    actionRehacer->setShortcut(QKeySequence::Redo); // Ctrl+Y
    connect(actionRehacer, &QAction::triggered, this, &MainWindow::redo);

    // Menú Herramientas
    QMenu *menuHerramientas = menuBar()->addMenu("Herramientas");

    QAction *actionGenerar = menuHerramientas->addAction("Generar Terreno...");
    connect(actionGenerar, &QAction::triggered, this, &MainWindow::on_pushButtonGenerate_clicked);

    QAction *actionVista3D = menuHerramientas->addAction("Vista 3D");
    connect(actionVista3D, &QAction::triggered, this, &MainWindow::on_pushButtonView3D_clicked);

    // Menú Ayuda (opcional)
    QMenu *menuAyuda = menuBar()->addMenu("Ayuda");

    QAction *actionAcercaDe = menuAyuda->addAction("Acerca de...");
    connect(actionAcercaDe, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "Acerca de HeightMapGen",
                           "HeightMapGen v1.0\n\nEditor de mapas de altura con generación procedural.");
    });
    // ============================================
    // CREAR TOOLBAR CON ICONOS DEL TEMA
    // ============================================
    QToolBar *mainToolBar = new QToolBar("Herramientas", this);
    mainToolBar->setIconSize(QSize(24, 24));
    mainToolBar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, mainToolBar);

    // ============================================
    // SECCIÓN: ARCHIVO
    // ============================================
    QAction *toolActionNew = mainToolBar->addAction(QIcon::fromTheme("document-new"), "Nuevo");
    toolActionNew->setShortcut(QKeySequence::New);
    toolActionNew->setToolTip("Crear nuevo heightmap (Ctrl+N)");
    connect(toolActionNew, &QAction::triggered, this, &MainWindow::on_pushButtonCreate_clicked);

    QAction *toolActionOpen = mainToolBar->addAction(QIcon::fromTheme("document-open"), "Abrir");
    toolActionOpen->setShortcut(QKeySequence::Open);
    toolActionOpen->setToolTip("Cargar heightmap (Ctrl+O)");
    connect(toolActionOpen, &QAction::triggered, this, &MainWindow::on_pushButtonLoad_clicked);

    QAction *toolActionSave = mainToolBar->addAction(QIcon::fromTheme("document-save"), "Guardar");
    toolActionSave->setShortcut(QKeySequence::Save);
    toolActionSave->setToolTip("Guardar heightmap (Ctrl+S)");
    connect(toolActionSave, &QAction::triggered, this, &MainWindow::on_pushButtonSave_clicked);

    mainToolBar->addSeparator();

    // ============================================
    // SECCIÓN: EDICIÓN
    // ============================================
    QAction *toolActionUndo = mainToolBar->addAction(QIcon::fromTheme("edit-undo"), "Deshacer");
    toolActionUndo->setShortcut(QKeySequence::Undo);
    toolActionUndo->setToolTip("Deshacer (Ctrl+Z)");
    connect(toolActionUndo, &QAction::triggered, this, &MainWindow::undo);

    QAction *toolActionRedo = mainToolBar->addAction(QIcon::fromTheme("edit-redo"), "Rehacer");
    toolActionRedo->setShortcut(QKeySequence::Redo);
    toolActionRedo->setToolTip("Rehacer (Ctrl+Y)");
    connect(toolActionRedo, &QAction::triggered, this, &MainWindow::redo);

    mainToolBar->addSeparator();

    // ============================================
    // SECCIÓN: HERRAMIENTAS
    // ============================================
    QAction *toolActionGenerate = mainToolBar->addAction(QIcon::fromTheme("view-refresh"), "Generar");
    toolActionGenerate->setToolTip("Generar terreno procedural");
    connect(toolActionGenerate, &QAction::triggered, this, &MainWindow::on_pushButtonGenerate_clicked);

    QAction *toolActionView3D = mainToolBar->addAction(QIcon::fromTheme("visibility"), "Vista 3D");
    toolActionView3D->setToolTip("Abrir visualización 3D");
    connect(toolActionView3D, &QAction::triggered, this, &MainWindow::on_pushButtonView3D_clicked);

    mainToolBar->addSeparator();

    // ============================================
    // SECCIÓN: IMPORTAR/EXPORTAR
    // ============================================
    QAction *toolActionImport = mainToolBar->addAction(QIcon::fromTheme("document-import"), "Importar");
    toolActionImport->setToolTip("Importar modelo 3D (OBJ/STL)");
    connect(toolActionImport, &QAction::triggered, this, &MainWindow::on_pushButtonImport3D_clicked);

    QAction *toolActionExport = mainToolBar->addAction(QIcon::fromTheme("document-export"), "Exportar");
    toolActionExport->setToolTip("Exportar modelo 3D (OBJ/STL)");
    connect(toolActionExport, &QAction::triggered, this, &MainWindow::on_pushButtonExport3D_clicked);

    // ===========================================
    //     TEXTURIZAR MAPA
    // ===========================================
    QAction *toolActionTexturize = mainToolBar->addAction(QIcon::fromTheme("applications-graphics"), "Texturizar");
    toolActionTexturize->setToolTip("Pintar texturas en el mapa 3D");
    connect(toolActionTexturize, &QAction::triggered, this, &MainWindow::on_pushButtonTexturize_clicked);
}  // <-- LLAVE DE CIERRE DEL CONSTRUCTOR (debe estar aquí)

MainWindow::~MainWindow()
{
    if (dynamicImageLabel) {
        delete dynamicImageLabel;
    }
    delete ui;
    // Ajustar tamaño inicial de la ventana
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    // Usar un tamaño mínimo razonable para mostrar todos los botones
    int initialWidth = std::max(670, 180 + 20 + 20); // 670 para mostrar botones hasta x=651
    int initialHeight = std::max(300, 20 + 50);

    this->setFixedSize(QSize(initialWidth, initialHeight));

}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->scrollAreaDisplay && dynamicImageLabel)
    {
        if (event->type() == QEvent::MouseButtonPress) {
            this->mousePressEvent(static_cast<QMouseEvent*>(event));
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            if (isPainting) {
                this->mouseMoveEvent(static_cast<QMouseEvent*>(event));
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            this->mouseReleaseEvent(static_cast<QMouseEvent*>(event));
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_pushButtonCreate_clicked()
{
    int newMapWidth = ui->lineEditWidth->text().toInt();
    int newMapHeight = ui->lineEditHeight->text().toInt();

    if (newMapWidth < 16 || newMapHeight < 16 || newMapWidth > 4096 || newMapHeight > 4096) {
        newMapWidth = 512;
        newMapHeight = 512;
        QMessageBox::warning(this, "Advertencia de Tamaño", "El tamaño debe estar entre 16 y 4096. Usando 512x512.");
        ui->lineEditWidth->setText("512");
        ui->lineEditHeight->setText("512");
    }

    mapWidth = newMapWidth;
    mapHeight = newMapHeight;

    heightMapData.assign(mapHeight, std::vector<unsigned char>(mapWidth, 128));
    currentImage = QImage(mapWidth, mapHeight, QImage::Format_RGB32);

    if (dynamicImageLabel) {
        delete dynamicImageLabel;
        dynamicImageLabel = nullptr;
    }

    dynamicImageLabel = new QLabel(ui->scrollAreaDisplay);
    dynamicImageLabel->setFixedSize(mapWidth, mapHeight);
    ui->scrollAreaDisplay->setWidget(dynamicImageLabel);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    int scrollAreaWidth = std::min(mapWidth, maxScrollWidth);
    int scrollAreaHeight = std::min(mapHeight, maxScrollHeight);

    ui->scrollAreaDisplay->setGeometry(180, 20, scrollAreaWidth, scrollAreaHeight);

    // CAMBIO: Usar las variables miembro en lugar de constantes hardcodeadas
    int requiredWidth = 180 + scrollAreaWidth + 20;
    int requiredHeight = 20 + scrollAreaHeight + 50;

    // Usar el máximo entre el tamaño calculado y el tamaño original
    requiredWidth = std::max(requiredWidth, originalWindowWidth);
    requiredHeight = std::max(requiredHeight, originalWindowHeight);

    QSize newSize(requiredWidth, requiredHeight);
    this->setFixedSize(newSize);

    // Limpiar historial undo/redo
    undoStack.clear();
    redoStack.clear();

    updateHeightmapDisplay();
}

void MainWindow::updateHeightmapDisplay()
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    for (int y = 0; y < mapHeight; ++y) {
        QRgb *pixel = reinterpret_cast<QRgb*>(currentImage.scanLine(y));

        for (int x = 0; x < mapWidth; ++x) {
            unsigned char value = heightMapData[y][x];
            *pixel = qRgb(value, value, value);
            pixel++;
        }
    }

    dynamicImageLabel->setPixmap(QPixmap::fromImage(currentImage));
}

void MainWindow::on_pushButtonSave_clicked()
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Guardar Heightmap", "", "PNG Files (*.png)");
    if (fileName.isEmpty()) return;

    if (currentImage.save(fileName, "PNG")) {
        QMessageBox::information(this, "Éxito", "Heightmap guardado.");
    } else {
        QMessageBox::critical(this, "Error", "No se pudo guardar el archivo.");
    }
}

void MainWindow::on_pushButtonLoad_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Cargar Heightmap", "", "PNG Files (*.png)");

    if (fileName.isEmpty()) return;

    QImage loadedImage;
    if (!loadedImage.load(fileName)) {
        QMessageBox::critical(this, "Error", "No se pudo cargar el archivo.");
        return;
    }

    // Convertir a escala de grises si no lo está
    loadedImage = loadedImage.convertToFormat(QImage::Format_Grayscale8);

    mapWidth = loadedImage.width();
    mapHeight = loadedImage.height();

    // Validar dimensiones
    if (mapWidth < 16 || mapHeight < 16 || mapWidth > 4096 || mapHeight > 4096) {
        QMessageBox::warning(this, "Error", "Las dimensiones deben estar entre 16 y 4096.");
        return;
    }

    // Copiar datos de la imagen a heightMapData
    heightMapData.assign(mapHeight, std::vector<unsigned char>(mapWidth, 0));
    for (int y = 0; y < mapHeight; ++y) {
        const uchar *line = loadedImage.scanLine(y);
        for (int x = 0; x < mapWidth; ++x) {
            heightMapData[y][x] = line[x];
        }
    }

    currentImage = loadedImage.convertToFormat(QImage::Format_RGB32);

    // Actualizar UI
    if (dynamicImageLabel) {
        delete dynamicImageLabel;
        dynamicImageLabel = nullptr;
    }

    dynamicImageLabel = new QLabel(ui->scrollAreaDisplay);
    dynamicImageLabel->setFixedSize(mapWidth, mapHeight);
    ui->scrollAreaDisplay->setWidget(dynamicImageLabel);

    // Ajustar la ventana
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    int scrollAreaWidth = std::min(mapWidth, maxScrollWidth);
    int scrollAreaHeight = std::min(mapHeight, maxScrollHeight);

    ui->scrollAreaDisplay->setGeometry(180, 20, scrollAreaWidth, scrollAreaHeight);

    // CAMBIO: Usar las variables miembro en lugar de constantes hardcodeadas
    int requiredWidth = 180 + scrollAreaWidth + 20;
    int requiredHeight = 20 + scrollAreaHeight + 50;

    // Usar el máximo entre el tamaño calculado y el tamaño original
    requiredWidth = std::max(requiredWidth, originalWindowWidth);
    requiredHeight = std::max(requiredHeight, originalWindowHeight);

    QSize newSize(requiredWidth, requiredHeight);
    this->setFixedSize(newSize);

    // Limpiar historial
    undoStack.clear();
    redoStack.clear();

    updateHeightmapDisplay();
    QMessageBox::information(this, "Éxito", "Heightmap cargado correctamente.");
}
void MainWindow::on_pushButtonExport3D_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Exportar Modelo 3D",
                                                    "",
                                                    "OBJ Files (*.obj);;STL ASCII (*.stl);;STL Binary (*.stl)",
                                                    &selectedFilter);

    if (fileName.isEmpty()) return;

    bool isOBJ = fileName.endsWith(".obj", Qt::CaseInsensitive);
    bool isSTL = fileName.endsWith(".stl", Qt::CaseInsensitive);
    bool isSTLBinary = isSTL && selectedFilter.contains("Binary", Qt::CaseInsensitive);
    bool isSTLASCII = isSTL && !isSTLBinary;

    // NUEVO: Umbral de altura mínima para exportar (ajustable)
    const unsigned char HEIGHT_THRESHOLD = 5; // Píxeles con altura < 5 se ignoran

    if (isOBJ) {
        // === EXPORTAR OBJ CON FILTRADO ===
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "No se pudo crear el archivo.");
            return;
        }

        QTextStream out(&file);

        // Crear mapeo de coordenadas a índices de vértices
        std::vector<std::vector<int>> vertexIndexMap(mapHeight, std::vector<int>(mapWidth, -1));
        int vertexIndex = 1; // OBJ usa índices 1-based

        // Escribir solo vértices con altura > umbral
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                if (heightMapData[y][x] > HEIGHT_THRESHOLD) {
                    float height = heightMapData[y][x] / 255.0f * 100.0f;
                    out << "v " << x << " " << height << " " << y << "\n";
                    vertexIndexMap[y][x] = vertexIndex++;
                }
            }
        }

        // Escribir coordenadas de textura solo para vértices exportados
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                if (vertexIndexMap[y][x] != -1) {
                    out << "vt " << (float)x/mapWidth << " " << (float)y/mapHeight << "\n";
                }
            }
        }

        // Escribir caras solo si todos los vértices existen
        for (int y = 0; y < mapHeight - 1; ++y) {
            for (int x = 0; x < mapWidth - 1; ++x) {
                int topLeft = vertexIndexMap[y][x];
                int topRight = vertexIndexMap[y][x+1];
                int bottomLeft = vertexIndexMap[y+1][x];
                int bottomRight = vertexIndexMap[y+1][x+1];

                // Solo crear triángulos si todos los vértices existen
                if (topLeft != -1 && topRight != -1 && bottomLeft != -1 && bottomRight != -1) {
                    out << "f " << topLeft << "/" << topLeft << " "
                        << bottomLeft << "/" << bottomLeft << " "
                        << topRight << "/" << topRight << "\n";

                    out << "f " << topRight << "/" << topRight << " "
                        << bottomLeft << "/" << bottomLeft << " "
                        << bottomRight << "/" << bottomRight << "\n";
                }
            }
        }

        file.close();
        QMessageBox::information(this, "Éxito",
                                 QString("Modelo OBJ exportado correctamente.\nVértices exportados: %1")
                                     .arg(vertexIndex - 1));

    } else if (isSTLASCII) {
        // === EXPORTAR STL ASCII CON FILTRADO ===
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "No se pudo crear el archivo.");
            return;
        }

        QTextStream out(&file);
        out << "solid heightmap\n";

        int triangleCount = 0;

        for (int y = 0; y < mapHeight - 1; ++y) {
            for (int x = 0; x < mapWidth - 1; ++x) {
                // Solo exportar triángulos si al menos un vértice tiene altura > umbral
                bool hasSignificantHeight =
                    heightMapData[y][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y][x+1] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x+1] > HEIGHT_THRESHOLD;

                if (!hasSignificantHeight) continue;

                float h1 = heightMapData[y][x] / 255.0f * 100.0f;
                float h2 = heightMapData[y][x+1] / 255.0f * 100.0f;
                float h3 = heightMapData[y+1][x] / 255.0f * 100.0f;
                float h4 = heightMapData[y+1][x+1] / 255.0f * 100.0f;

                // Primer triángulo
                out << "  facet normal 0 1 0\n";
                out << "    outer loop\n";
                out << "      vertex " << x << " " << h1 << " " << y << "\n";
                out << "      vertex " << x << " " << h3 << " " << (y+1) << "\n";
                out << "      vertex " << (x+1) << " " << h2 << " " << y << "\n";
                out << "    endloop\n";
                out << "  endfacet\n";
                triangleCount++;

                // Segundo triángulo
                out << "  facet normal 0 1 0\n";
                out << "    outer loop\n";
                out << "      vertex " << (x+1) << " " << h2 << " " << y << "\n";
                out << "      vertex " << x << " " << h3 << " " << (y+1) << "\n";
                out << "      vertex " << (x+1) << " " << h4 << " " << (y+1) << "\n";
                out << "    endloop\n";
                out << "  endfacet\n";
                triangleCount++;
            }
        }

        out << "endsolid heightmap\n";
        file.close();
        QMessageBox::information(this, "Éxito",
                                 QString("Modelo STL ASCII exportado correctamente.\nTriángulos: %1")
                                     .arg(triangleCount));

    } else if (isSTLBinary) {
        // === EXPORTAR STL BINARIO CON FILTRADO ===
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "No se pudo crear el archivo.");
            return;
        }

        // Primero contar triángulos válidos
        uint32_t numTriangles = 0;
        for (int y = 0; y < mapHeight - 1; ++y) {
            for (int x = 0; x < mapWidth - 1; ++x) {
                bool hasSignificantHeight =
                    heightMapData[y][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y][x+1] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x+1] > HEIGHT_THRESHOLD;

                if (hasSignificantHeight) {
                    numTriangles += 2;
                }
            }
        }

        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);
        out.setFloatingPointPrecision(QDataStream::SinglePrecision);

        // Header (80 bytes)
        QByteArray header(80, 0);
        QString headerText = "HeightMapGen Binary STL Export (Filtered)";
        header.replace(0, headerText.length(), headerText.toUtf8());
        file.write(header);

        // Número de triángulos
        out << numTriangles;

        // Escribir triángulos filtrados
        for (int y = 0; y < mapHeight - 1; ++y) {
            for (int x = 0; x < mapWidth - 1; ++x) {
                bool hasSignificantHeight =
                    heightMapData[y][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y][x+1] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x] > HEIGHT_THRESHOLD ||
                    heightMapData[y+1][x+1] > HEIGHT_THRESHOLD;

                if (!hasSignificantHeight) continue;

                float h1 = heightMapData[y][x] / 255.0f * 100.0f;
                float h2 = heightMapData[y][x+1] / 255.0f * 100.0f;
                float h3 = heightMapData[y+1][x] / 255.0f * 100.0f;
                float h4 = heightMapData[y+1][x+1] / 255.0f * 100.0f;

                // Primer triángulo
                out << 0.0f << 1.0f << 0.0f;
                out << (float)x << h1 << (float)y;
                out << (float)x << h3 << (float)(y+1);
                out << (float)(x+1) << h2 << (float)y;
                out << (uint16_t)0;

                // Segundo triángulo
                out << 0.0f << 1.0f << 0.0f;
                out << (float)(x+1) << h2 << (float)y;
                out << (float)x << h3 << (float)(y+1);
                out << (float)(x+1) << h4 << (float)(y+1);
                out << (uint16_t)0;
            }
        }

        file.close();
        QMessageBox::information(this, "Éxito",
                                 QString("Modelo STL Binario exportado correctamente.\n"
                                         "Triángulos: %1\n"
                                         "Tamaño: %2 KB")
                                     .arg(numTriangles)
                                     .arg(file.size() / 1024));
    }
}

void MainWindow::on_pushButtonImport3D_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Importar Modelo 3D",
                                                    "",
                                                    "3D Files (*.obj *.OBJ *.stl *.STL);;OBJ Files (*.obj *.OBJ);;STL Files (*.stl *.STL)");

    if (fileName.isEmpty()) return;

    bool isOBJ = fileName.endsWith(".obj", Qt::CaseInsensitive);
    bool isSTL = fileName.endsWith(".stl", Qt::CaseInsensitive);

    if (!isOBJ && !isSTL) {
        QMessageBox::warning(this, "Error", "Formato no soportado.");
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo abrir el archivo.");
        return;
    }

    // Estructuras para almacenar vértices
    std::vector<float> vertices_x, vertices_y, vertices_z;

    QTextStream in(&file);

    if (isOBJ) {
        // === PARSEAR OBJ ===
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("v ")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    vertices_x.push_back(parts[1].toFloat());
                    vertices_y.push_back(parts[2].toFloat());
                    vertices_z.push_back(parts[3].toFloat());
                }
            }
        }
    } else if (isSTL) {
        // === PARSEAR STL ===
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("vertex")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    vertices_x.push_back(parts[1].toFloat());
                    vertices_y.push_back(parts[2].toFloat());
                    vertices_z.push_back(parts[3].toFloat());
                }
            }
        }
    }

    file.close();

    if (vertices_x.empty()) {
        QMessageBox::warning(this, "Error", "No se encontraron vértices en el archivo.");
        return;
    }

    // === CONVERTIR A HEIGHTMAP ===

    // Encontrar límites
    float minX = *std::min_element(vertices_x.begin(), vertices_x.end());
    float maxX = *std::max_element(vertices_x.begin(), vertices_x.end());
    float minZ = *std::min_element(vertices_z.begin(), vertices_z.end());
    float maxZ = *std::max_element(vertices_z.begin(), vertices_z.end());
    float minY = *std::min_element(vertices_y.begin(), vertices_y.end());
    float maxY = *std::max_element(vertices_y.begin(), vertices_y.end());

    float rangeX = maxX - minX;
    float rangeZ = maxZ - minZ;

    // Calcular dimensiones basadas en el rango real
    int targetWidth = static_cast<int>(std::ceil(rangeX));
    int targetHeight = static_cast<int>(std::ceil(rangeZ));

    // Validar dimensiones (mantener entre 16 y 4096)
    if (targetWidth < 16) targetWidth = 16;
    if (targetHeight < 16) targetHeight = 16;
    if (targetWidth > 4096) targetWidth = 4096;
    if (targetHeight > 4096) targetHeight = 4096;

    mapWidth = targetWidth;
    mapHeight = targetHeight;

    // Inicializar heightmap con valores mínimos
    heightMapData.assign(mapHeight, std::vector<unsigned char>(mapWidth, 0));

    // Proyectar vértices al heightmap
    for (size_t i = 0; i < vertices_x.size(); ++i) {
        // Normalizar coordenadas X,Z al rango del heightmap
        int x = static_cast<int>((vertices_x[i] - minX) / rangeX * (mapWidth - 1));
        int z = static_cast<int>((vertices_z[i] - minZ) / rangeZ * (mapHeight - 1));

        // Normalizar altura Y al rango 0-255
        float normalizedY = (vertices_y[i] - minY) / (maxY - minY);
        unsigned char heightValue = static_cast<unsigned char>(normalizedY * 255.0f);

        // Tomar el valor máximo si hay múltiples vértices en la misma posición
        if (x >= 0 && x < mapWidth && z >= 0 && z < mapHeight) {
            heightMapData[z][x] = std::max(heightMapData[z][x], heightValue);
        }
    }

    // === ACTUALIZAR UI ===

    if (dynamicImageLabel) {
        delete dynamicImageLabel;
        dynamicImageLabel = nullptr;
    }

    dynamicImageLabel = new QLabel(ui->scrollAreaDisplay);
    dynamicImageLabel->setFixedSize(mapWidth, mapHeight);
    ui->scrollAreaDisplay->setWidget(dynamicImageLabel);

    currentImage = QImage(mapWidth, mapHeight, QImage::Format_RGB32);

    // Ajustar ventana
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    int scrollAreaWidth = std::min(mapWidth, maxScrollWidth);
    int scrollAreaHeight = std::min(mapHeight, maxScrollHeight);

    ui->scrollAreaDisplay->setGeometry(180, 20, scrollAreaWidth, scrollAreaHeight);

    // CAMBIO: Usar las variables miembro en lugar de constantes hardcodeadas
    int requiredWidth = 180 + scrollAreaWidth + 20;
    int requiredHeight = 20 + scrollAreaHeight + 50;

    // Usar el máximo entre el tamaño calculado y el tamaño original
    requiredWidth = std::max(requiredWidth, originalWindowWidth);
    requiredHeight = std::max(requiredHeight, originalWindowHeight);

    this->setFixedSize(QSize(requiredWidth, requiredHeight));

    // Limpiar historial
    undoStack.clear();
    redoStack.clear();

    updateHeightmapDisplay();

    QString format = isOBJ ? "OBJ" : "STL";
    QMessageBox::information(this, "Éxito",
                             QString("Modelo %1 importado correctamente como heightmap %2x%3.")
                                 .arg(format).arg(mapWidth).arg(mapHeight));
}

QPoint MainWindow::mapToDataCoordinates(int screenX, int screenY)
{
    int dataX = screenX;
    int dataY = screenY;

    return QPoint(
        std::min(std::max(dataX, 0), mapWidth - 1),
        std::min(std::max(dataY, 0), mapHeight - 1)
        );
}

void MainWindow::applyBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    double intensityFactor = 0.3;
    if (ui->sliderBrushIntensity) {
        intensityFactor = ui->sliderBrushIntensity->value() / 100.0;
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);

                int currentValue = heightMapData[y][x];
                int targetValue = static_cast<int>(currentValue + (brushHeight - currentValue) * intensity * intensityFactor);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

// =================================================================
// === PERLIN NOISE FUNCTIONS
// =================================================================

void MainWindow::initializePerlin()
{
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(p.begin(), p.end(), std::default_random_engine(seed));

    p.insert(p.end(), p.begin(), p.end());

    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<> distrib(100.0, 5000.0);
    frequencyOffset = distrib(gen);
}

double MainWindow::fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double MainWindow::lerp(double t, double a, double b)
{
    return a + t * (b - a);
}

double MainWindow::grad(int hash, double x, double y, double z)
{
    int h = hash & 15;
    double u = (h < 8) ? x : y;
    double v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double MainWindow::perlin(double x, double y)
{
    if (p.empty()) initializePerlin();

    int X = (int)std::floor(x);
    int Y = (int)std::floor(y);

    x -= std::floor(x);
    y -= std::floor(y);
    double z = 0.0;

    double u = fade(x);
    double v = fade(y);

    int A = p[(X & 255)] + (Y & 255);
    int B = p[(X + 1) & 255] + (Y & 255);

    int AA = p[A & 511] + 0;
    int AB = p[B & 511] + 0;
    int BA = p[A & 511] + 1;
    int BB = p[B & 511] + 1;

    return lerp(v, lerp(u, grad(p[AA], x, y, z),
                        grad(p[BA], x - 1, y, z)),
                lerp(u, grad(p[AB], x, y - 1, z),
                     grad(p[BB], x - 1, y - 1, z)));
}

double MainWindow::fbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += perlin(x * freq, y * freq) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return total / maxVal;
}

// =================================================================
// === SIMPLEX NOISE IMPLEMENTATION
// =================================================================

double MainWindow::simplexNoise(double xin, double yin)
{
    if (p.empty()) initializePerlin();

    const double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
    const double G2 = (3.0 - std::sqrt(3.0)) / 6.0;

    double s = (xin + yin) * F2;
    int i = std::floor(xin + s);
    int j = std::floor(yin + s);

    double t = (i + j) * G2;
    double X0 = i - t;
    double Y0 = j - t;
    double x0 = xin - X0;
    double y0 = yin - Y0;

    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else { i1 = 0; j1 = 1; }

    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2;
    double y2 = y0 - 1.0 + 2.0 * G2;

    int ii = i & 255;
    int jj = j & 255;
    int gi0 = p[ii + p[jj]] % 12;
    int gi1 = p[ii + i1 + p[jj + j1]] % 12;
    int gi2 = p[ii + 1 + p[jj + 1]] % 12;

    double n0 = 0.0, n1 = 0.0, n2 = 0.0;

    double t0 = 0.5 - x0*x0 - y0*y0;
    if (t0 > 0) {
        t0 *= t0;
        n0 = t0 * t0 * (grad3[gi0][0]*x0 + grad3[gi0][1]*y0);
    }

    double t1 = 0.5 - x1*x1 - y1*y1;
    if (t1 > 0) {
        t1 *= t1;
        n1 = t1 * t1 * (grad3[gi1][0]*x1 + grad3[gi1][1]*y1);
    }

    double t2 = 0.5 - x2*x2 - y2*y2;
    if (t2 > 0) {
        t2 *= t2;
        n2 = t2 * t2 * (grad3[gi2][0]*x2 + grad3[gi2][1]*y2);
    }

    return 70.0 * (n0 + n1 + n2);
}

// =================================================================
// === VORONOI NOISE IMPLEMENTATION
// =================================================================

double MainWindow::voronoiNoise(double x, double y, int numPoints)
{
    // Determinar la celda actual en una cuadrícula
    int cellX = static_cast<int>(std::floor(x));
    int cellY = static_cast<int>(std::floor(y));

    double minDist = std::numeric_limits<double>::max();

    // Buscar en la celda actual y las 8 celdas vecinas
    for (int offsetY = -1; offsetY <= 1; ++offsetY) {
        for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            int neighborX = cellX + offsetX;
            int neighborY = cellY + offsetY;

            // Generar punto aleatorio para esta celda usando hash
            unsigned int seed = static_cast<unsigned int>(neighborX * 374761393 + neighborY * 668265263);
            seed = (seed ^ (seed >> 13)) * 1274126177;

            double pointX = neighborX + (seed & 0xFFFF) / 65535.0;
            seed = (seed ^ (seed >> 16)) * 85734257;
            double pointY = neighborY + (seed & 0xFFFF) / 65535.0;

            // Calcular distancia
            double dx = x - pointX;
            double dy = y - pointY;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist < minDist) {
                minDist = dist;
            }
        }
    }

    // Normalizar: la distancia máxima típica en una celda es ~1.414 (diagonal)
    // Mapear [0, 1.5] a [-1, 1]
    return (std::min(minDist / 1.5, 1.0) * 2.0) - 1.0;
}

double MainWindow::voronoiFbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += voronoiNoise(x * freq, y * freq, voronoiNumPoints) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return total / maxVal;
}

// =================================================================
// === RIDGED MULTIFRACTAL IMPLEMENTATION
// =================================================================

double MainWindow::ridgedMultifractal(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        // Obtener ruido base (usando Perlin)
        double noiseValue = perlin(x * freq, y * freq);

        // Aplicar transformación ridged: invertir y tomar valor absoluto
        noiseValue = 1.0 - std::abs(noiseValue);

        // Elevar al cuadrado para acentuar las crestas
        noiseValue = noiseValue * noiseValue;

        total += noiseValue * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return (total / maxVal) * 2.0 - 1.0;
}

// =================================================================
// === BILLOWY NOISE IMPLEMENTATION
// =================================================================

double MainWindow::billowyNoise(double x, double y)
{
    // Billowy usa valor absoluto del ruido para crear formas redondeadas
    double noiseValue = perlin(x, y);
    return std::abs(noiseValue) * 2.0 - 1.0;
}

double MainWindow::billowyFbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        double noiseValue = perlin(x * freq, y * freq);
        noiseValue = std::abs(noiseValue);

        total += noiseValue * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return (total / maxVal) * 2.0 - 1.0;
}

// =================================================================
// === DOMAIN WARPING IMPLEMENTATION
// =================================================================

double MainWindow::domainWarp(double x, double y, double warpStrength)
{
    // Usar dos capas de ruido para distorsionar las coordenadas
    double warpX = perlin(x * 0.5, y * 0.5) * warpStrength;
    double warpY = perlin(x * 0.5 + 100.0, y * 0.5 + 100.0) * warpStrength;

    // Aplicar la distorsión y obtener el ruido final
    double warpedX = x + warpX;
    double warpedY = y + warpY;

    return fbm(warpedX, warpedY);
}


double MainWindow::simplexFbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += simplexNoise(x * freq, y * freq) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return total / maxVal;
}

// =================================================================
// === ADDITIONAL BRUSH MODES
// =================================================================

void MainWindow::applySmoothBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    // Crear copia temporal para evitar modificar mientras calculamos promedios
    std::vector<std::vector<unsigned char>> tempData = heightMapData;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                int sum = 0;
                int count = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < mapWidth && ny >= 0 && ny < mapHeight) {
                            sum += tempData[ny][nx];
                            count++;
                        }
                    }
                }
                int average = sum / count;

                double intensity = 1.0 - (distSq / brushRadiusSq);
                int currentValue = tempData[y][x];
                int newValue = static_cast<int>(currentValue + (average - currentValue) * intensity * 0.3);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(newValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

void MainWindow::applyFlattenBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);
                int currentValue = heightMapData[y][x];

                int targetValue = static_cast<int>(currentValue + (flattenHeight - currentValue) * intensity * 0.1);
                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

void MainWindow::applyNoiseBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    if (p.empty()) initializePerlin();

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);

                // Generar ruido en esta posición
                double noiseValue = perlin(x * 0.1, y * 0.1);
                int noiseHeight = static_cast<int>((noiseValue + 1.0) * 127.5);

                int currentValue = heightMapData[y][x];
                int targetValue = static_cast<int>(currentValue + (noiseHeight - currentValue) * intensity * 0.15);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

// =================================================================
// === UNDO/REDO SYSTEM
// =================================================================

void MainWindow::saveStateToUndo()
{
    undoStack.push_back(heightMapData);

    if (undoStack.size() > maxUndoSteps) {
        undoStack.erase(undoStack.begin());
    }

    clearRedoStack();
}

void MainWindow::undo()
{
    if (undoStack.empty()) {
        QMessageBox::information(this, "Deshacer", "No hay acciones para deshacer.");
        return;
    }

    redoStack.push_back(heightMapData);
    heightMapData = undoStack.back();
    undoStack.pop_back();

    updateHeightmapDisplay();
}

void MainWindow::redo()
{
    if (redoStack.empty()) {
        QMessageBox::information(this, "Rehacer", "No hay acciones para rehacer.");
        return;
    }

    undoStack.push_back(heightMapData);
    heightMapData = redoStack.back();
    redoStack.pop_back();

    updateHeightmapDisplay();
}

void MainWindow::clearRedoStack()
{
    redoStack.clear();
}

// =================================================================
// === TERRAIN GENERATION
// =================================================================

void MainWindow::on_pushButtonGenerate_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero (Botón 'Crear').");
        return;
    }

    // 1. OBTENER PARÁMETROS DE LA GUI
    octaves = ui->spinBoxOctaves->value();
    persistence = ui->doubleSpinBoxPersistence->value();
    frequencyScale = ui->doubleSpinBoxFrequencyScale->value();

    QString offsetText = ui->lineEditOffset->text();
    if (offsetText.toLower() == "aleatorio" || offsetText.isEmpty()) {
        initializePerlin();
    } else {
        bool ok;
        double customOffset = offsetText.toDouble(&ok);
        if (ok) {
            frequencyOffset = customOffset;
        } else {
            initializePerlin();
            QMessageBox::warning(this, "Advertencia", "Desplazamiento no válido. Usando valor aleatorio.");
            ui->lineEditOffset->setText(QString::number(frequencyOffset));
        }
        if (p.empty()) initializePerlin();
    }

    double scale = std::min(mapWidth, mapHeight);
    const double baseFrequency = 1.0 / (scale * frequencyScale);

    // NUEVO: Determinar qué algoritmo usar
    QString noiseType = ui->comboBoxNoiseType->currentText();

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            double sampleX = (double)x * baseFrequency + frequencyOffset;
            double sampleY = (double)y * baseFrequency + frequencyOffset;

            double noiseValue;

            if (noiseType == "Simplex Noise") {
                noiseValue = simplexFbm(sampleX, sampleY);
            } else if (noiseType == "Voronoi Noise") {
                noiseValue = voronoiFbm(sampleX, sampleY);
            } else if (noiseType == "Ridged Multifractal") {
                noiseValue = ridgedMultifractal(sampleX, sampleY);
            } else if (noiseType == "Billowy Noise") {
                noiseValue = billowyFbm(sampleX, sampleY);
            } else if (noiseType == "Domain Warping") {
                noiseValue = domainWarp(sampleX, sampleY, 50.0);
            } else {
                // Por defecto: Perlin Noise
                noiseValue = fbm(sampleX, sampleY);
            }

            unsigned char height = (unsigned char)((noiseValue + 1.0) * 127.5);
            heightMapData[y][x] = height;
        }
    }

    updateHeightmapDisplay();
    QMessageBox::information(this, "Éxito",
                             QString("Terreno generado con %1.\nOctavas: %2, Persistencia: %3, Escala: %4")
                                 .arg(noiseType)
                                 .arg(octaves)
                                 .arg(persistence)
                                 .arg(frequencyScale));
}
// =================================================================
// === MOUSE EVENTS
// =================================================================
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    QPoint globalPos = event->globalPosition().toPoint();
    QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

    if (!dynamicImageLabel->rect().contains(localPos)) return;

    saveStateToUndo();
    isPainting = true;

    QString brushModeText = ui->comboBoxBrushMode->currentText();
    qDebug() << "Modo seleccionado:" << brushModeText;  // DEBUG
    // IMPORTANTE: Los casos de geometría deben ir ANTES del else
    if (brushModeText == "Línea") {
        currentBrushMode = LINE;
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        shapeStartPoint = dataPos;
        isDrawingShape = true;
        previewImage = currentImage.copy();
        return;  // CRÍTICO: salir aquí
    } else if (brushModeText == "Rectángulo") {
        currentBrushMode = RECTANGLE;
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        shapeStartPoint = dataPos;
        isDrawingShape = true;
        previewImage = currentImage.copy();
        return;  // CRÍTICO: salir aquí
    } else if (brushModeText == "Círculo") {
        currentBrushMode = CIRCLE;
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        shapeStartPoint = dataPos;
        isDrawingShape = true;
        previewImage = currentImage.copy();
        return;  // CRÍTICO: salir aquí
    } else if (brushModeText == "Suavizar") {
        currentBrushMode = SMOOTH;
    } else if (brushModeText == "Aplanar") {
        currentBrushMode = FLATTEN;
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        flattenHeight = heightMapData[dataPos.y()][dataPos.x()];
    } else if (brushModeText == "Ruido") {
        currentBrushMode = NOISE;
    } else if (brushModeText == "Rellenar") {
        currentBrushMode = FILL;
    } else {
        currentBrushMode = RAISE_LOWER;
        brushHeight = brushColor;
    }

    QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

    switch (currentBrushMode) {
    case RAISE_LOWER:
        applyBrush(dataPos.x(), dataPos.y());
        break;
    case SMOOTH:
        applySmoothBrush(dataPos.x(), dataPos.y());
        break;
    case FLATTEN:
        applyFlattenBrush(dataPos.x(), dataPos.y());
        break;
    case NOISE:
        applyNoiseBrush(dataPos.x(), dataPos.y());
        break;
    case FILL:
        applyFillBrush(dataPos.x(), dataPos.y());
        break;
    default:
        break;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!dynamicImageLabel) return;

    QPoint globalPos = event->globalPosition().toPoint();
    QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

    if (!dynamicImageLabel->rect().contains(localPos)) return;

    // Manejo especial para formas con preview
    if (isDrawingShape && (currentBrushMode == LINE || currentBrushMode == RECTANGLE || currentBrushMode == CIRCLE)) {
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

        // Restaurar imagen original
        currentImage = previewImage.copy();

        // Dibujar preview de la forma
        QPainter painter(&currentImage);
        QPen pen(QColor(brushColor, brushColor, brushColor));
        pen.setWidth(2);
        painter.setPen(pen);

        if (currentBrushMode == LINE) {
            painter.drawLine(shapeStartPoint.x(), shapeStartPoint.y(),
                             dataPos.x(), dataPos.y());
        } else if (currentBrushMode == RECTANGLE) {
            int x = std::min(shapeStartPoint.x(), dataPos.x());
            int y = std::min(shapeStartPoint.y(), dataPos.y());
            int w = std::abs(dataPos.x() - shapeStartPoint.x());
            int h = std::abs(dataPos.y() - shapeStartPoint.y());
            painter.drawRect(x, y, w, h);
        } else if (currentBrushMode == CIRCLE) {
            int dx = dataPos.x() - shapeStartPoint.x();
            int dy = dataPos.y() - shapeStartPoint.y();
            int radius = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            painter.drawEllipse(shapeStartPoint, radius, radius);
        }

        // Actualizar display
        if (dynamicImageLabel) {
            dynamicImageLabel->setPixmap(QPixmap::fromImage(currentImage));
        }
        return;
    }

    // Comportamiento normal para otros pinceles
    if (isPainting) {
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

        switch (currentBrushMode) {
        case RAISE_LOWER:
            applyBrush(dataPos.x(), dataPos.y());
            break;
        case SMOOTH:
            applySmoothBrush(dataPos.x(), dataPos.y());
            break;
        case FLATTEN:
            applyFlattenBrush(dataPos.x(), dataPos.y());
            break;
        case NOISE:
            applyNoiseBrush(dataPos.x(), dataPos.y());
            break;
        case FILL:
            // No hacer nada - el relleno solo se ejecuta en mousePressEvent
            break;
        case LINE:
            break;
        case RECTANGLE:
            break;
        case CIRCLE:
            break;
        default:
            break;
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (isDrawingShape && (currentBrushMode == LINE || currentBrushMode == RECTANGLE || currentBrushMode == CIRCLE)) {
        QPoint globalPos = event->globalPosition().toPoint();
        QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

        // Aplicar la forma final al heightMapData
        if (currentBrushMode == LINE) {
            drawLine(shapeStartPoint.x(), shapeStartPoint.y(), dataPos.x(), dataPos.y());
        } else if (currentBrushMode == RECTANGLE) {
            drawRectangle(shapeStartPoint.x(), shapeStartPoint.y(), dataPos.x(), dataPos.y());
        } else if (currentBrushMode == CIRCLE) {
            int dx = dataPos.x() - shapeStartPoint.x();
            int dy = dataPos.y() - shapeStartPoint.y();
            int radius = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            drawCircle(shapeStartPoint.x(), shapeStartPoint.y(), radius);
        }

        isDrawingShape = false;
        updateHeightmapDisplay();
    }

    isPainting = false;
}

// =================================================================
// === 3D VIEW WINDOW
// =================================================================
void MainWindow::on_pushButtonView3D_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Vista 3D - HeightMap");
    dialog->resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    QHBoxLayout *waterControls = new QHBoxLayout();

    QCheckBox *checkShowWater = new QCheckBox("Mostrar Agua", dialog);
    checkShowWater->setChecked(true);

    QLabel *labelWaterLevel = new QLabel("Nivel de Agua:", dialog);
    QSlider *sliderWaterLevel = new QSlider(Qt::Horizontal, dialog);
    sliderWaterLevel->setRange(0, 100);
    sliderWaterLevel->setValue(50);
    sliderWaterLevel->setMinimumWidth(150);

    waterControls->addWidget(checkShowWater);
    waterControls->addWidget(labelWaterLevel);
    waterControls->addWidget(sliderWaterLevel);
    waterControls->addStretch();

    QPushButton *btnTexture = new QPushButton("Cargar Textura Terreno", dialog);
    waterControls->addWidget(btnTexture);

    QPushButton *btnWaterTexture = new QPushButton("Cargar Textura Agua", dialog);
    waterControls->addWidget(btnWaterTexture);

    mainLayout->addLayout(waterControls);

    // CAMBIAR AQUÍ: Usar un nombre de variable local diferente
    OpenGLWidget *glWidget = new OpenGLWidget(dialog);
    glWidget->setHeightMapData(heightMapData);
    mainLayout->addWidget(glWidget);

    // Ahora las lambdas funcionarán correctamente
    connect(btnTexture, &QPushButton::clicked, [glWidget]() {
        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            "Seleccionar Textura del Terreno",
            "",
            "Imágenes (*.png *.jpg *.jpeg *.bmp)"
            );

        if (!fileName.isEmpty()) {
            glWidget->loadTexture(fileName);
        }
    });

    connect(btnWaterTexture, &QPushButton::clicked, [glWidget]() {
        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            "Seleccionar Textura del Agua",
            "",
            "Imágenes (*.png *.jpg *.jpeg *.bmp)"
            );

        if (!fileName.isEmpty()) {
            glWidget->loadWaterTexture(fileName);
        }
    });

    connect(sliderWaterLevel, &QSlider::valueChanged, [glWidget](int value) {
        glWidget->setWaterLevel(static_cast<float>(value));
    });

    connect(checkShowWater, &QCheckBox::toggled, [glWidget](bool checked) {
        glWidget->showWater = checked;
        glWidget->update();
    });

    dialog->setLayout(mainLayout);
    dialog->show();
}

// =================================================================
// === SHAPE DRAWING FUNCTIONS
// =================================================================

void MainWindow::drawLine(int x1, int y1, int x2, int y2)
{
    if (!ui->sliderBrushSize || !ui->sliderBrushIntensity) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;

    double intensityFactor = ui->sliderBrushIntensity->value() / 100.0;

    // Algoritmo de Bresenham para la línea central
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int lineX = x1;
    int lineY = y1;

    while (true) {
        // Para cada punto de la línea, dibujar un círculo con el radio del pincel
        for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
            for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                int px = lineX + dx;
                int py = lineY + dy;

                // Verificar que está dentro del radio circular
                double distSq = dx * dx + dy * dy;
                double brushRadiusSq = brushRadius * brushRadius;

                if (distSq <= brushRadiusSq && px >= 0 && px < mapWidth && py >= 0 && py < mapHeight) {
                    // Calcular intensidad con falloff radial
                    double intensity = 1.0 - (distSq / brushRadiusSq);
                    intensity *= intensityFactor;

                    // Mezclar con el valor existente
                    int currentValue = heightMapData[py][px];
                    int targetValue = static_cast<int>(currentValue + (brushColor - currentValue) * intensity);
                    heightMapData[py][px] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
                }
            }
        }

        if (lineX == x2 && lineY == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            lineX += sx;
        }
        if (e2 < dx) {
            err += dx;
            lineY += sy;
        }
    }
}

void MainWindow::drawRectangle(int x1, int y1, int x2, int y2)
{
    // Asegurar que x1,y1 sea la esquina superior izquierda
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);

    // Dibujar los cuatro lados del rectángulo
    drawLine(minX, minY, maxX, minY);  // Lado superior
    drawLine(maxX, minY, maxX, maxY);  // Lado derecho
    drawLine(maxX, maxY, minX, maxY);  // Lado inferior
    drawLine(minX, maxY, minX, minY);  // Lado izquierdo
}

void MainWindow::drawCircle(int centerX, int centerY, int radius)
{
    if (!ui->sliderBrushSize || !ui->sliderBrushIntensity) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;

    double intensityFactor = ui->sliderBrushIntensity->value() / 100.0;

    // Algoritmo del punto medio para el círculo
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    auto drawCirclePointsWithBrush = [&](int cx, int cy, int x, int y) {
        // Para cada punto del círculo, dibujar con el tamaño del pincel
        auto drawPointWithBrush = [&](int px, int py) {
            // Dibujar un círculo pequeño alrededor de este punto
            for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
                for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                    int finalX = px + dx;
                    int finalY = py + dy;

                    double distSq = dx * dx + dy * dy;
                    double brushRadiusSq = brushRadius * brushRadius;

                    if (distSq <= brushRadiusSq && finalX >= 0 && finalX < mapWidth && finalY >= 0 && finalY < mapHeight) {
                        double intensity = 1.0 - (distSq / brushRadiusSq);
                        intensity *= intensityFactor;

                        int currentValue = heightMapData[finalY][finalX];
                        int targetValue = static_cast<int>(currentValue + (brushColor - currentValue) * intensity);
                        heightMapData[finalY][finalX] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
                    }
                }
            }
        };

        // Dibujar los 8 puntos simétricos
        drawPointWithBrush(cx + x, cy + y);
        drawPointWithBrush(cx - x, cy + y);
        drawPointWithBrush(cx + x, cy - y);
        drawPointWithBrush(cx - x, cy - y);
        drawPointWithBrush(cx + y, cy + x);
        drawPointWithBrush(cx - y, cy + x);
        drawPointWithBrush(cx + y, cy - x);
        drawPointWithBrush(cx - y, cy - x);
    };

    drawCirclePointsWithBrush(centerX, centerY, x, y);

    while (x < y) {
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
        drawCirclePointsWithBrush(centerX, centerY, x, y);
    }
}

void MainWindow::applyFillBrush(int mapX, int mapY)
{
    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    // Color original del píxel donde se hizo clic
    unsigned char targetColor = heightMapData[mapY][mapX];

    // Si el color de relleno es igual al color objetivo, no hacer nada
    if (targetColor == brushColor) return;

    // Usar una cola para flood fill iterativo (evita stack overflow)
    std::queue<QPoint> queue;
    queue.push(QPoint(mapX, mapY));

    // Marcar píxeles visitados para evitar procesarlos múltiples veces
    std::vector<std::vector<bool>> visited(mapHeight, std::vector<bool>(mapWidth, false));

    while (!queue.empty()) {
        QPoint current = queue.front();
        queue.pop();

        int x = current.x();
        int y = current.y();

        // Verificar límites
        if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

        // Si ya visitamos este píxel, saltar
        if (visited[y][x]) continue;

        // Si el color no coincide con el color objetivo, saltar
        if (heightMapData[y][x] != targetColor) continue;

        // Marcar como visitado y rellenar
        visited[y][x] = true;
        heightMapData[y][x] = static_cast<unsigned char>(brushColor);

        // Añadir píxeles vecinos a la cola (4-conectividad: arriba, abajo, izquierda, derecha)
        queue.push(QPoint(x, y - 1)); // Arriba
        queue.push(QPoint(x, y + 1)); // Abajo
        queue.push(QPoint(x - 1, y)); // Izquierda
        queue.push(QPoint(x + 1, y)); // Derecha
    }

    updateHeightmapDisplay();

}

// ===========================================
//   FUNCIONES TEXTURIZADO MAPA
// ===========================================

class PaintableLabel : public QLabel {
public:
    PaintableLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setMouseTracking(true);
    }

    std::function<void(QMouseEvent*)> paintCallback;
    std::function<void()> releaseCallback;  // NUEVO: Callback para cuando se suelta el mouse
    bool isPainting = false;

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            isPainting = true;
            if (paintCallback) {
                paintCallback(event);
            }
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (isPainting && paintCallback) {
            paintCallback(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            isPainting = false;
            if (releaseCallback) {  // NUEVO: Llamar al callback de release
                releaseCallback();
            }
        }
    }
};
void MainWindow::on_pushButtonTexturize_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Texturizar Mapa 3D");
    dialog->resize(1200, 800);

    QHBoxLayout *mainLayout = new QHBoxLayout(dialog);

    // ===== PANEL IZQUIERDO: CONTROLES =====
    QVBoxLayout *leftPanel = new QVBoxLayout();

    QLabel *labelColors = new QLabel("Colores Disponibles:", dialog);
    leftPanel->addWidget(labelColors);

    QListWidget *colorList = new QListWidget(dialog);
    colorList->setViewMode(QListWidget::IconMode);
    colorList->setIconSize(QSize(64, 64));
    colorList->setSpacing(10);
    leftPanel->addWidget(colorList);

    // Colores predefinidos
    QList<QColor> predefinedColors = {
        QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255),
        QColor(255, 255, 0), QColor(255, 0, 255), QColor(0, 255, 255),
        QColor(128, 128, 128), QColor(255, 255, 255)
    };

    for (const QColor &color : predefinedColors) {
        QPixmap pixmap(64, 64);
        pixmap.fill(color);
        QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), "");
        item->setData(Qt::UserRole, color);
        colorList->addItem(item);
    }

    QPushButton *btnCustomColor = new QPushButton("Color Personalizado", dialog);
    leftPanel->addWidget(btnCustomColor);

    QPushButton *btnLoadTexture = new QPushButton("Cargar Textura", dialog);
    leftPanel->addWidget(btnLoadTexture);

    QPushButton *btnLoadDirectory = new QPushButton("Cargar Directorio", dialog);
    leftPanel->addWidget(btnLoadDirectory);

    QLabel *labelBrushSize = new QLabel("Tamaño del Pincel:", dialog);
    leftPanel->addWidget(labelBrushSize);

    QSlider *sliderTextureBrushSize = new QSlider(Qt::Horizontal, dialog);
    sliderTextureBrushSize->setRange(5, 100);
    sliderTextureBrushSize->setValue(20);
    leftPanel->addWidget(sliderTextureBrushSize);


    QLabel *labelBrushSizeValue = new QLabel("20", dialog);
    leftPanel->addWidget(labelBrushSizeValue);

    QLabel *labelOpacity = new QLabel("Opacidad del Pincel:", dialog);
    leftPanel->addWidget(labelOpacity);

    QSlider *sliderBrushOpacity = new QSlider(Qt::Horizontal, dialog);  // ← SOLO AQUÍ
    sliderBrushOpacity->setRange(0, 100);
    sliderBrushOpacity->setValue(100);
    leftPanel->addWidget(sliderBrushOpacity);

    QLabel *labelOpacityValue = new QLabel("100%", dialog);
    leftPanel->addWidget(labelOpacityValue);

    // Modo de pintado
    QLabel *labelPaintMode = new QLabel("Modo de Pintado:", dialog);
    leftPanel->addWidget(labelPaintMode);

    QComboBox *comboPaintMode = new QComboBox(dialog);
    comboPaintMode->addItem("Pincel");
    comboPaintMode->addItem("Relleno");
    comboPaintMode->addItem("Difuminar");
    comboPaintMode->addItem("Clonar");
    comboPaintMode->addItem("Borrador");
    leftPanel->addWidget(comboPaintMode);

    // Opacidad del pincel

    QSlider *sliderOpacity = new QSlider(Qt::Horizontal, dialog);
    sliderOpacity->setRange(0, 100);
    sliderOpacity->setValue(100);
    leftPanel->addWidget(sliderOpacity);


    // Botones de Undo/Redo
    QPushButton *btnUndo = new QPushButton("Deshacer", dialog);
    leftPanel->addWidget(btnUndo);

    QPushButton *btnRedo = new QPushButton("Rehacer", dialog);
    leftPanel->addWidget(btnRedo);

    QPushButton *btnSaveTexture = new QPushButton("Guardar Textura PNG", dialog);
    leftPanel->addWidget(btnSaveTexture);

    QPushButton *btnExportOBJ = new QPushButton("Exportar OBJ con Textura", dialog);
    leftPanel->addWidget(btnExportOBJ);

    QPushButton *btnImportOBJ = new QPushButton("Importar OBJ con Textura", dialog);
    leftPanel->addWidget(btnImportOBJ);

    leftPanel->addStretch();

    // ===== PANEL DERECHO: VISTAS 2D Y 3D =====
    QVBoxLayout *rightPanel = new QVBoxLayout();

    QLabel *label2DTitle = new QLabel("Vista 2D - Pintar aquí:", dialog);
    rightPanel->addWidget(label2DTitle);

    PaintableLabel *label2D = new PaintableLabel(dialog);
    label2D->setMinimumSize(500, 400);
    label2D->setScaledContents(true);

    // Crear imagen 2D inicial
    QImage *paintImage = new QImage(mapWidth, mapHeight, QImage::Format_RGB32);
    paintImage->setColorSpace(QColorSpace::SRgb);

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            unsigned char height = heightMapData[y][x];
            paintImage->setPixel(x, y, qRgb(height, height, height));
        }
    }
    label2D->setPixmap(QPixmap::fromImage(*paintImage));
    rightPanel->addWidget(label2D);

    QLabel *label3DTitle = new QLabel("Vista 3D - Resultado:", dialog);
    rightPanel->addWidget(label3DTitle);

    OpenGLWidget *glWidget = new OpenGLWidget(dialog);
    glWidget->setHeightMapData(heightMapData);
    glWidget->setTexturePaintMode(true);
    glWidget->setMinimumSize(500, 400);
    rightPanel->addWidget(glWidget);

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addLayout(rightPanel, 3);

    // ===== VARIABLES COMPARTIDAS =====
    QColor *currentColor = new QColor(Qt::red);
    int *brushSize = new int(20);
    QList<QImage> *loadedTextures = new QList<QImage>();
    QList<QString> *textureNames = new QList<QString>();
    int *currentTextureMode = new int(0);
    bool *isFirstClick = new bool(true);

    // Sistema de undo/redo
    QList<QImage> *undoStackTexture = new QList<QImage>();
    QList<QImage> *redoStackTexture = new QList<QImage>();
    int *maxUndoStepsTexture = new int(50);

    // Opacidad del pincel
    int *brushOpacity = new int(100);

    // Variable para controlar el primer trazo
    bool *firstPaint = new bool(true);

    // Variables para modo Clonar
    QPoint *cloneSourcePoint = new QPoint(-1, -1);
    bool *cloneSourceSet = new bool(false);

    // ===== LAMBDA PARA EXPORTAR OBJ CON TEXTURA =====
    auto exportOBJWithTexture = [=]() {
        QString objFileName = QFileDialog::getSaveFileName(dialog,
                                                           "Exportar OBJ con Textura",
                                                           "",
                                                           "OBJ Files (*.obj)");
        if (objFileName.isEmpty()) return;

        if (!objFileName.endsWith(".obj", Qt::CaseInsensitive)) {
            objFileName += ".obj";
        }

        QFileInfo fileInfo(objFileName);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();
        QString mtlFileName = baseName + ".mtl";
        QString textureFileName = baseName + "_texture.png";
        QString mtlFilePath = dirPath + "/" + mtlFileName;
        QString textureFilePath = dirPath + "/" + textureFileName;

        // Guardar textura
        paintImage->setColorSpace(QColorSpace::SRgb);
        if (!paintImage->save(textureFilePath, "PNG")) {
            QMessageBox::critical(dialog, "Error", "No se pudo guardar la textura.");
            return;
        }

        // Crear archivo MTL
        QFile mtlFile(mtlFilePath);
        if (!mtlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(dialog, "Error", "No se pudo crear el archivo MTL.");
            return;
        }

        QTextStream mtlStream(&mtlFile);
        mtlStream << "# Material file for " << baseName << ".obj\n";
        mtlStream << "newmtl TexturedTerrain\n";
        mtlStream << "Ka 1.0 1.0 1.0\n";
        mtlStream << "Kd 1.0 1.0 1.0\n";
        mtlStream << "Ks 0.0 0.0 0.0\n";
        mtlStream << "d 1.0\n";
        mtlStream << "illum 1\n";
        mtlStream << "map_Kd " << textureFileName << "\n";
        mtlFile.close();

        // Crear archivo OBJ
        QFile objFile(objFileName);
        if (!objFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(dialog, "Error", "No se pudo crear el archivo OBJ.");
            return;
        }

        QTextStream objStream(&objFile);
        objStream << "# Heightmap exported from HeightMapGenerator\n";
        objStream << "mtllib " << mtlFileName << "\n\n";

        const float HEIGHT_THRESHOLD = 1.0f;
        std::vector<std::vector<int>> vertexIndexMap(mapHeight, std::vector<int>(mapWidth, -1));
        int vertexIndex = 1;

        // Escribir vértices y coordenadas UV
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                if (heightMapData[y][x] > HEIGHT_THRESHOLD) {
                    float height = heightMapData[y][x] / 255.0f * 100.0f;
                    objStream << "v " << x << " " << height << " " << y << "\n";
                    vertexIndexMap[y][x] = vertexIndex++;
                }
            }
        }

        objStream << "\n";

        // Escribir coordenadas UV
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                if (heightMapData[y][x] > HEIGHT_THRESHOLD) {
                    float u = (float)x / (float)mapWidth;
                    float v = 1.0f - ((float)y / (float)mapHeight);
                    objStream << "vt " << u << " " << v << "\n";
                }
            }
        }

        objStream << "\n";
        objStream << "usemtl TexturedTerrain\n\n";

        // Escribir caras
        for (int y = 0; y < mapHeight - 1; ++y) {
            for (int x = 0; x < mapWidth - 1; ++x) {
                int topLeft = vertexIndexMap[y][x];
                int topRight = vertexIndexMap[y][x + 1];
                int bottomLeft = vertexIndexMap[y + 1][x];
                int bottomRight = vertexIndexMap[y + 1][x + 1];

                if (topLeft != -1 && bottomLeft != -1 && topRight != -1) {
                    objStream << "f " << topLeft << "/" << topLeft << " "
                              << bottomLeft << "/" << bottomLeft << " "
                              << topRight << "/" << topRight << "\n";
                }

                if (topRight != -1 && bottomLeft != -1 && bottomRight != -1) {
                    objStream << "f " << topRight << "/" << topRight << " "
                              << bottomLeft << "/" << bottomLeft << " "
                              << bottomRight << "/" << bottomRight << "\n";
                }
            }
        }

        objFile.close();

        QMessageBox::information(dialog, "Éxito",
                                 QString("Exportación completada:\n- %1\n- %2\n- %3")
                                     .arg(objFileName)
                                     .arg(mtlFilePath)
                                     .arg(textureFilePath));
    };

    // Lambda para importar OBJ con textura
    auto importOBJWithTexture = [=]() {
        QString objFileName = QFileDialog::getOpenFileName(dialog,
                                                           "Importar OBJ con Textura",
                                                           "",
                                                           "OBJ Files (*.obj *.OBJ)");
        if (objFileName.isEmpty()) return;

        QFile objFile(objFileName);
        if (!objFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(dialog, "Error", "No se pudo abrir el archivo OBJ.");
            return;
        }

        // === PASO 1: PARSEAR GEOMETRÍA DEL OBJ ===
        std::vector<float> vertices_x, vertices_y, vertices_z;
        QString mtlFileName;

        QTextStream in(&objFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            // Leer vértices
            if (line.startsWith("v ")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    vertices_x.push_back(parts[1].toFloat());
                    vertices_y.push_back(parts[2].toFloat());
                    vertices_z.push_back(parts[3].toFloat());
                }
            }
            // Leer referencia al archivo MTL
            else if (line.startsWith("mtllib ")) {
                mtlFileName = line.mid(7).trimmed();
            }
        }
        objFile.close();

        if (vertices_x.empty()) {
            QMessageBox::warning(dialog, "Error", "No se encontraron vértices en el OBJ.");
            return;
        }

        // === PASO 2: CONVERTIR GEOMETRÍA A HEIGHTMAP ===
        // (Igual que on_pushButtonImport3D_clicked)

        float minX = *std::min_element(vertices_x.begin(), vertices_x.end());
        float maxX = *std::max_element(vertices_x.begin(), vertices_x.end());
        float minZ = *std::min_element(vertices_z.begin(), vertices_z.end());
        float maxZ = *std::max_element(vertices_z.begin(), vertices_z.end());
        float minY = *std::min_element(vertices_y.begin(), vertices_y.end());
        float maxY = *std::max_element(vertices_y.begin(), vertices_y.end());

        float rangeX = maxX - minX;
        float rangeZ = maxZ - minZ;

        int targetWidth = static_cast<int>(std::ceil(rangeX));
        int targetHeight = static_cast<int>(std::ceil(rangeZ));

        if (targetWidth < 16) targetWidth = 16;
        if (targetHeight < 16) targetHeight = 16;
        if (targetWidth > 4096) targetWidth = 4096;
        if (targetHeight > 4096) targetHeight = 4096;

        // Crear nuevo heightmap con las dimensiones calculadas
        std::vector<std::vector<unsigned char>> newHeightMap(targetHeight,
                                                             std::vector<unsigned char>(targetWidth, 0));

        // Proyectar vértices al grid 2D
        for (size_t i = 0; i < vertices_x.size(); ++i) {
            float normX = (vertices_x[i] - minX) / rangeX;
            float normZ = (vertices_z[i] - minZ) / rangeZ;
            float normY = (vertices_y[i] - minY) / (maxY - minY);

            int gridX = static_cast<int>(normX * (targetWidth - 1));
            int gridZ = static_cast<int>(normZ * (targetHeight - 1));

            if (gridX >= 0 && gridX < targetWidth && gridZ >= 0 && gridZ < targetHeight) {
                unsigned char heightValue = static_cast<unsigned char>(normY * 255);
                newHeightMap[gridZ][gridX] = std::max(newHeightMap[gridZ][gridX], heightValue);
            }
        }

        // === PASO 3: ACTUALIZAR HEIGHTMAP PRINCIPAL ===
        // IMPORTANTE: Actualizar las dimensiones y el heightMapData de MainWindow
        mapWidth = targetWidth;
        mapHeight = targetHeight;
        heightMapData = newHeightMap;

        // Actualizar la imagen 2D con el nuevo heightmap
        paintImage->~QImage();
        *paintImage = QImage(mapWidth, mapHeight, QImage::Format_RGB32);
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                unsigned char height = heightMapData[y][x];
                paintImage->setPixel(x, y, qRgb(height, height, height));
            }
        }

        // === PASO 4: CARGAR TEXTURA DEL MTL (SI EXISTE) ===
        if (!mtlFileName.isEmpty()) {
            QFileInfo objFileInfo(objFileName);
            QString mtlFilePath = objFileInfo.absolutePath() + "/" + mtlFileName;

            QFile mtlFile(mtlFilePath);
            if (mtlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream mtlIn(&mtlFile);
                QString textureFileName;

                while (!mtlIn.atEnd()) {
                    QString line = mtlIn.readLine().trimmed();
                    if (line.startsWith("map_Kd ")) {
                        textureFileName = line.mid(7).trimmed();
                        break;
                    }
                }
                mtlFile.close();

                if (!textureFileName.isEmpty()) {
                    QString textureFilePath = objFileInfo.absolutePath() + "/" + textureFileName;
                    QImage textureImage(textureFilePath);

                    if (!textureImage.isNull()) {
                        // Redimensionar textura si no coincide con el heightmap
                        if (textureImage.width() != mapWidth || textureImage.height() != mapHeight) {
                            textureImage = textureImage.scaled(mapWidth, mapHeight,
                                                               Qt::IgnoreAspectRatio,
                                                               Qt::SmoothTransformation);
                        }

                        // Aplicar textura al paintImage y colorMap
                        for (int y = 0; y < mapHeight; ++y) {
                            for (int x = 0; x < mapWidth; ++x) {
                                QColor color = textureImage.pixelColor(x, y);
                                paintImage->setPixel(x, y, color.rgb());
                                glWidget->setColorAtPosition(x, y, color);
                            }
                        }

                        QMessageBox::information(dialog, "Éxito",
                                                 QString("OBJ importado con textura:\n- Dimensiones: %1x%2\n- Textura: %3")
                                                     .arg(mapWidth).arg(mapHeight).arg(textureFileName));
                    } else {
                        QMessageBox::warning(dialog, "Advertencia",
                                             "Geometría importada, pero no se pudo cargar la textura.");
                    }
                }
            }
        } else {
            QMessageBox::information(dialog, "Éxito",
                                     QString("OBJ importado sin textura:\n- Dimensiones: %1x%2")
                                         .arg(mapWidth).arg(mapHeight));
        }

        // === PASO 5: ACTUALIZAR VISTAS ===
        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->setHeightMapData(heightMapData);
        glWidget->generateMesh();
        glWidget->update();
    };


    // ===== LAMBDAS DE FUNCIONALIDAD =====

    // Lambda para guardar estado en undo stack
    auto saveTextureStateToUndo = [=]() {
        undoStackTexture->append(paintImage->copy());

        if (undoStackTexture->size() > *maxUndoStepsTexture) {
            undoStackTexture->removeFirst();
        }

        redoStackTexture->clear();
        qDebug() << "Texture state saved. Undo stack size:" << undoStackTexture->size();
    };

    // Lambda para deshacer
    auto undoTexture = [=]() {
        if (undoStackTexture->isEmpty()) {
            QMessageBox::information(dialog, "Deshacer", "No hay acciones para deshacer.");
            return;
        }

        redoStackTexture->append(paintImage->copy());
        *paintImage = undoStackTexture->last();
        undoStackTexture->removeLast();

        label2D->setPixmap(QPixmap::fromImage(*paintImage));

        // Actualizar colorMap del OpenGLWidget
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                QColor color = paintImage->pixelColor(x, y);
                glWidget->setColorAtPosition(x, y, color);
            }
        }

        glWidget->generateMesh();
        glWidget->update();

        qDebug() << "Undo executed. Stack size:" << undoStackTexture->size();
    };

    // Lambda para rehacer
    auto redoTexture = [=]() {
        if (redoStackTexture->isEmpty()) {
            QMessageBox::information(dialog, "Rehacer", "No hay acciones para rehacer.");
            return;
        }

        undoStackTexture->append(paintImage->copy());
        *paintImage = redoStackTexture->last();
        redoStackTexture->removeLast();

        label2D->setPixmap(QPixmap::fromImage(*paintImage));

        // Actualizar colorMap del OpenGLWidget
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                QColor color = paintImage->pixelColor(x, y);
                glWidget->setColorAtPosition(x, y, color);
            }
        }

        glWidget->generateMesh();
        glWidget->update();

        qDebug() << "Redo executed. Stack size:" << redoStackTexture->size();
    };

    // Lambda de relleno con texturas (flood fill)
    auto fillTexture = [=](int startX, int startY, bool isTexture, int textureIndex) {
        if (startX < 0 || startX >= mapWidth || startY < 0 || startY >= mapHeight) return;

        QColor targetColor = paintImage->pixelColor(startX, startY);

        std::queue<QPoint> queue;
        queue.push(QPoint(startX, startY));

        std::vector<std::vector<bool>> visited(mapHeight, std::vector<bool>(mapWidth, false));

        while (!queue.empty()) {
            QPoint current = queue.front();
            queue.pop();

            int x = current.x();
            int y = current.y();

            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;
            if (visited[y][x]) continue;
            if (paintImage->pixelColor(x, y) != targetColor) continue;

            visited[y][x] = true;

            // Calcular color para este píxel específico
            QColor fillColor;
            if (isTexture) {
                const QImage &texture = loadedTextures->at(textureIndex);
                int texX = x % texture.width();
                int texY = y % texture.height();
                fillColor = texture.pixelColor(texX, texY);
            } else {
                fillColor = *currentColor;
            }

            paintImage->setPixel(x, y, fillColor.rgb());
            glWidget->setColorAtPosition(x, y, fillColor);

            queue.push(QPoint(x, y - 1));
            queue.push(QPoint(x, y + 1));
            queue.push(QPoint(x - 1, y));
            queue.push(QPoint(x + 1, y));
        }

        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->generateMesh();
        glWidget->update();
    };
    // ===== LAMBDAS DE MODOS DE PINCEL ADICIONALES =====

    // Lambda para aplicar pincel de difuminado
    auto applyBlurBrush = [=](int mapX, int mapY) {
        int radius = *brushSize / 2;

        // Crear copia temporal para evitar modificar mientras calculamos promedios
        QImage tempImage = paintImage->copy();

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int x = mapX + dx;
                int y = mapY + dy;

                if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= radius) {
                        // Calcular promedio de vecinos (kernel 3x3)
                        int sumR = 0, sumG = 0, sumB = 0;
                        int count = 0;

                        for (int ny = -1; ny <= 1; ++ny) {
                            for (int nx = -1; nx <= 1; ++nx) {
                                int sampleX = x + nx;
                                int sampleY = y + ny;

                                if (sampleX >= 0 && sampleX < mapWidth && sampleY >= 0 && sampleY < mapHeight) {
                                    QColor neighborColor = tempImage.pixelColor(sampleX, sampleY);
                                    sumR += neighborColor.red();
                                    sumG += neighborColor.green();
                                    sumB += neighborColor.blue();
                                    count++;
                                }
                            }
                        }

                        QColor avgColor(sumR / count, sumG / count, sumB / count);

                        // Aplicar intensidad basada en distancia y opacidad
                        float intensity = (1.0f - (dist / radius)) * (*brushOpacity / 100.0f);
                        QColor existingColor = tempImage.pixelColor(x, y);

                        int r = static_cast<int>(avgColor.red() * intensity + existingColor.red() * (1.0f - intensity));
                        int g = static_cast<int>(avgColor.green() * intensity + existingColor.green() * (1.0f - intensity));
                        int b = static_cast<int>(avgColor.blue() * intensity + existingColor.blue() * (1.0f - intensity));

                        QColor blendedColor(r, g, b);
                        paintImage->setPixel(x, y, blendedColor.rgb());
                        glWidget->setColorAtPosition(x, y, blendedColor);
                    }
                }
            }
        }

        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->generateMesh();
        glWidget->update();
    };

    // Lambda para aplicar pincel de clonado
    auto applyCloneBrush = [=](int mapX, int mapY) {
        if (!*cloneSourceSet) return;

        int radius = *brushSize / 2;
        int offsetX = mapX - cloneSourcePoint->x();
        int offsetY = mapY - cloneSourcePoint->y();

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int destX = mapX + dx;
                int destY = mapY + dy;

                if (destX >= 0 && destX < mapWidth && destY >= 0 && destY < mapHeight) {
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= radius) {
                        // Calcular posición de origen
                        int srcX = cloneSourcePoint->x() + dx;
                        int srcY = cloneSourcePoint->y() + dy;

                        if (srcX >= 0 && srcX < mapWidth && srcY >= 0 && srcY < mapHeight) {
                            QColor srcColor = paintImage->pixelColor(srcX, srcY);

                            // Aplicar opacidad si es menor a 100%
                            if (*brushOpacity < 100) {
                                QColor existingColor = paintImage->pixelColor(destX, destY);
                                float alpha = *brushOpacity / 100.0f;

                                int r = static_cast<int>(srcColor.red() * alpha + existingColor.red() * (1.0f - alpha));
                                int g = static_cast<int>(srcColor.green() * alpha + existingColor.green() * (1.0f - alpha));
                                int b = static_cast<int>(srcColor.blue() * alpha + existingColor.blue() * (1.0f - alpha));

                                srcColor = QColor(r, g, b);
                            }

                            paintImage->setPixel(destX, destY, srcColor.rgb());
                            glWidget->setColorAtPosition(destX, destY, srcColor);
                        }
                    }
                }
            }
        }

        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->generateMesh();
        glWidget->update();
    };

    // Lambda para aplicar pincel borrador (restaura heightmap original)
    auto applyEraserBrush = [=](int mapX, int mapY) {
        int radius = *brushSize / 2;

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int x = mapX + dx;
                int y = mapY + dy;

                if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= radius) {
                        // Restaurar color original del heightmap (escala de grises)
                        unsigned char height = heightMapData[y][x];
                        QColor originalColor(height, height, height);

                        // Aplicar opacidad si es menor a 100%
                        if (*brushOpacity < 100) {
                            QColor existingColor = paintImage->pixelColor(x, y);
                            float alpha = *brushOpacity / 100.0f;

                            int r = static_cast<int>(originalColor.red() * alpha + existingColor.red() * (1.0f - alpha));
                            int g = static_cast<int>(originalColor.green() * alpha + existingColor.green() * (1.0f - alpha));
                            int b = static_cast<int>(originalColor.blue() * alpha + existingColor.blue() * (1.0f - alpha));

                            originalColor = QColor(r, g, b);
                        }

                        paintImage->setPixel(x, y, originalColor.rgb());
                        glWidget->setColorAtPosition(x, y, originalColor);
                    }
                }
            }
        }

        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->generateMesh();
        glWidget->update();
    };
    // ===== LAMBDA PRINCIPAL DE PINTADO =====

    auto paintOnLabel = [=](QMouseEvent *mouseEvent) mutable {
        // Guardar estado solo en el primer clic
        if (*firstPaint) {
            saveTextureStateToUndo();
            *firstPaint = false;
        }

        QPoint pos = mouseEvent->pos();
        int mapX = (pos.x() * mapWidth) / label2D->width();
        int mapY = (pos.y() * mapHeight) / label2D->height();

        if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

        QString paintMode = comboPaintMode->currentText();

        // MODO RELLENO
        if (paintMode == "Relleno") {
            if (*isFirstClick) {
                *isFirstClick = false;

                QListWidgetItem *currentItem = colorList->currentItem();
                if (!currentItem) return;

                bool isTexture = (currentItem->data(Qt::UserRole).toInt() == -1);

                if (isTexture) {
                    int textureIndex = currentItem->data(Qt::UserRole + 1).toInt();
                    fillTexture(mapX, mapY, true, textureIndex);
                } else {
                    fillTexture(mapX, mapY, false, 0);
                }
            }
            return;
        }

        // MODO DIFUMINAR
        if (paintMode == "Difuminar") {
            applyBlurBrush(mapX, mapY);
            return;
        }

        // MODO CLONAR
        if (paintMode == "Clonar") {
            // Ctrl+Click establece el punto de origen
            if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                *cloneSourcePoint = QPoint(mapX, mapY);
                *cloneSourceSet = true;
                QMessageBox::information(dialog, "Clonar",
                                         QString("Origen establecido en (%1, %2)").arg(mapX).arg(mapY));
                return;
            }

            // Click normal clona desde el origen
            if (*cloneSourceSet) {
                applyCloneBrush(mapX, mapY);
            }
            return;
        }

        // MODO BORRADOR
        if (paintMode == "Borrador") {
            applyEraserBrush(mapX, mapY);
            return;
        }

        // MODO PINCEL (por defecto) con opacidad
        int radius = *brushSize / 2;

        QListWidgetItem *currentItem = colorList->currentItem();
        if (!currentItem) return;

        bool isTexture = (currentItem->data(Qt::UserRole).toInt() == -1);

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int x = mapX + dx;
                int y = mapY + dy;

                if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= radius) {
                        QColor pixelColor;

                        if (isTexture) {
                            int textureIndex = currentItem->data(Qt::UserRole + 1).toInt();
                            const QImage &texture = loadedTextures->at(textureIndex);
                            int texX = x % texture.width();
                            int texY = y % texture.height();
                            pixelColor = texture.pixelColor(texX, texY);
                        } else {
                            pixelColor = *currentColor;
                        }

                        // Aplicar opacidad si es menor a 100%
                        if (*brushOpacity < 100) {
                            QColor existingColor = paintImage->pixelColor(x, y);
                            float alpha = *brushOpacity / 100.0f;

                            int r = static_cast<int>(pixelColor.red() * alpha + existingColor.red() * (1.0f - alpha));
                            int g = static_cast<int>(pixelColor.green() * alpha + existingColor.green() * (1.0f - alpha));
                            int b = static_cast<int>(pixelColor.blue() * alpha + existingColor.blue() * (1.0f - alpha));

                            pixelColor = QColor(r, g, b);
                        }

                        paintImage->setPixel(x, y, pixelColor.rgb());
                        glWidget->setColorAtPosition(x, y, pixelColor);
                    }
                }
            }
        }

        label2D->setPixmap(QPixmap::fromImage(*paintImage));
        glWidget->generateMesh();
        glWidget->update();
    };

    // ASIGNAR CALLBACKS AL PAINTABLELABEL
    label2D->paintCallback = paintOnLabel;
    label2D->releaseCallback = [isFirstClick, firstPaint]() {
        *isFirstClick = true;
        *firstPaint = true;
    };
    // ===== CONECTAR EVENTOS (ANTES DE dialog->exec()) =====

    // Botones de Undo/Redo
    connect(btnUndo, &QPushButton::clicked, undoTexture);
    connect(btnRedo, &QPushButton::clicked, redoTexture);

    // Botón de exportación OBJ con textura
    connect(btnExportOBJ, &QPushButton::clicked, exportOBJWithTexture);

    // Selector de color personalizado
    connect(btnCustomColor, &QPushButton::clicked, [colorList, dialog]() {
        QColor color = QColorDialog::getColor(Qt::white, dialog, "Seleccionar Color");
        if (color.isValid()) {
            QPixmap pixmap(64, 64);
            pixmap.fill(color);
            QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), "");
            item->setData(Qt::UserRole, color);
            colorList->addItem(item);
            colorList->setCurrentItem(item);
        }
    });

    // Cargar textura individual
    connect(btnLoadTexture, &QPushButton::clicked, [colorList, loadedTextures, textureNames, dialog]() {
        QString fileName = QFileDialog::getOpenFileName(dialog,
                                                        "Cargar Textura",
                                                        "",
                                                        "Imágenes (*.png *.jpg *.jpeg *.bmp)");
        if (fileName.isEmpty()) return;

        QImage texture(fileName);
        if (texture.isNull()) {
            QMessageBox::warning(dialog, "Error", "No se pudo cargar la textura.");
            return;
        }

        loadedTextures->append(texture);
        textureNames->append(QFileInfo(fileName).fileName());

        QImage thumbnail = texture.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap pixmap = QPixmap::fromImage(thumbnail);

        QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), QFileInfo(fileName).fileName());
        item->setData(Qt::UserRole, QVariant::fromValue(-1));
        item->setData(Qt::UserRole + 1, loadedTextures->size() - 1);
        colorList->addItem(item);
    });

    // Cargar directorio de texturas
    connect(btnLoadDirectory, &QPushButton::clicked, [colorList, loadedTextures, textureNames, dialog]() {
        QString dirPath = QFileDialog::getExistingDirectory(dialog,
                                                            "Seleccionar Directorio de Texturas",
                                                            "",
                                                            QFileDialog::ShowDirsOnly);
        if (dirPath.isEmpty()) return;

        QDir dir(dirPath);
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

        for (const QFileInfo &fileInfo : files) {
            QImage texture(fileInfo.absoluteFilePath());
            if (texture.isNull()) continue;

            loadedTextures->append(texture);
            textureNames->append(fileInfo.fileName());

            QImage thumbnail = texture.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap pixmap = QPixmap::fromImage(thumbnail);

            QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), fileInfo.fileName());
            item->setData(Qt::UserRole, QVariant::fromValue(-1));
            item->setData(Qt::UserRole + 1, loadedTextures->size() - 1);
            colorList->addItem(item);
        }

        QMessageBox::information(dialog, "Éxito",
                                 QString("Se cargaron %1 texturas.").arg(files.size()));
    });

    // Cambio de color/textura seleccionada
    connect(colorList, &QListWidget::currentRowChanged, [colorList, currentColor, glWidget, currentTextureMode](int row) {
        if (row >= 0) {
            QListWidgetItem *item = colorList->item(row);
            if (item->data(Qt::UserRole).toInt() == -1) {
                *currentTextureMode = 1;
            } else {
                *currentTextureMode = 0;
                QColor color = item->data(Qt::UserRole).value<QColor>();
                *currentColor = color;
                glWidget->setCurrentPaintColor(color);
            }
        }
    });

    // Cambio de tamaño de pincel
    connect(sliderTextureBrushSize, &QSlider::valueChanged, [brushSize, labelBrushSizeValue](int value) {
        *brushSize = value;
        labelBrushSizeValue->setText(QString::number(value));
    });

    // Cambio de opacidad del pincel
    connect(sliderBrushOpacity, &QSlider::valueChanged, [brushOpacity, labelOpacityValue](int value) {
        *brushOpacity = value;
        labelOpacityValue->setText(QString::number(value) + "%");
    });

    // Guardar textura PNG con sRGB
    connect(btnSaveTexture, &QPushButton::clicked, [paintImage, dialog]() {
        QString fileName = QFileDialog::getSaveFileName(dialog, "Guardar Textura", "", "PNG Files (*.png)");
        if (fileName.isEmpty()) return;

        paintImage->setColorSpace(QColorSpace::SRgb);

        if (paintImage->save(fileName, "PNG")) {
            QMessageBox::information(dialog, "Éxito", "Textura guardada correctamente en sRGB.");
        } else {
            QMessageBox::critical(dialog, "Error", "No se pudo guardar la textura.");
        }
    });

    // Botón de importación OBJ con textura
    connect(btnImportOBJ, &QPushButton::clicked, importOBJWithTexture);

    // Limpieza de memoria al cerrar el diálogo
    connect(dialog, &QDialog::destroyed, [paintImage, currentColor, brushSize, loadedTextures, textureNames, currentTextureMode, undoStackTexture, redoStackTexture, maxUndoStepsTexture, brushOpacity, firstPaint, cloneSourcePoint, cloneSourceSet]() {
        delete paintImage;
        delete currentColor;
        delete brushSize;
        delete loadedTextures;
        delete textureNames;
        delete currentTextureMode;
        delete undoStackTexture;
        delete redoStackTexture;
        delete maxUndoStepsTexture;
        delete brushOpacity;
        delete firstPaint;
        delete cloneSourcePoint;
        delete cloneSourceSet;
    });

    // Seleccionar primer color por defecto
    if (colorList->count() > 0) {
        colorList->setCurrentRow(0);
    }

    dialog->setLayout(mainLayout);
    dialog->exec();
}

QImage MainWindow::generateColorMapImage(const std::vector<std::vector<QColor>>& colorMap)
{
    if (colorMap.empty() || colorMap[0].empty()) {
        return QImage();
    }

    int height = colorMap.size();
    int width = colorMap[0].size();

    QImage image(width, height, QImage::Format_RGB32);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color = colorMap[y][x];
            if (color.isValid()) {
                image.setPixel(x, y, color.rgb());
            } else {
                // Color por defecto si es transparente
                unsigned char h = heightMapData[y][x];
                image.setPixel(x, y, qRgb(h, h, h));
            }
        }
    }

    return image;
}
