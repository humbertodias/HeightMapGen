#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <QImage>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <random>
#include <numeric>
#include <chrono>
#include "openglwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using HeightMapData_t = std::vector<std::vector<unsigned char>>;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_pushButtonCreate_clicked();
    void on_pushButtonSave_clicked();
    void on_pushButtonLoad_clicked();
    void on_pushButtonGenerate_clicked();
    void on_pushButtonView3D_clicked();
    void on_pushButtonExport3D_clicked();
    void on_pushButtonImport3D_clicked();
    void applyFillBrush(int mapX, int mapY);
    void on_pushButtonTexturize_clicked();
    QImage generateColorMapImage(const std::vector<std::vector<QColor>>& colorMap);

private:
    Ui::MainWindow *ui;
    QLabel *dynamicImageLabel;
    // Agregar estas líneas:
    int originalWindowWidth;
    int originalWindowHeight;
    HeightMapData_t heightMapData;
    QImage currentImage;
    int mapWidth = 0;
    int mapHeight = 0;
    bool isPainting = false;
    int brushColor = 128;  // Color de relleno (0-255
    int brushHeight = 128;
    double brushIntensity = 0.3;
    bool *isFirstClick = new bool(true);

    // En la sección private de mainwindow.h:
    std::vector<std::vector<int>> textureMap; // Mapa de índices de textura por píxel
    std::vector<QString> loadedTextures;      // Lista de rutas de texturas cargadas
    int currentTextureIndex = 0;              // Textura actualmente seleccionada
    int textureBrushSize = 20;                // Tamaño del pincel d

    // === BRUSH MODES ===
    enum BrushMode {
        RAISE_LOWER,
        SMOOTH,
        FLATTEN,
        NOISE,
        FILL,
        LINE,
        RECTANGLE,
        CIRCLE
    };

    // Añadir variables para formas de dos puntos
    QPoint shapeStartPoint;
    bool isDrawingShape = false;
    QImage previewImage;  // Para mostrar preview durante el dibujo
    BrushMode currentBrushMode = RAISE_LOWER;
    int flattenHeight = 128;

    // === PERLIN NOISE VARIABLES ===
    std::vector<int> p;
    int octaves = 6;
    double persistence = 0.55;
    double frequencyOffset = 0.0;
    double frequencyScale = 8.0;

    // === SIMPLEX NOISE FUNCTIONS ===
    static constexpr int grad3[12][3] = {
        {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
        {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
        {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
    };

    // === UNDO/REDO SYSTEM ===
    std::vector<HeightMapData_t> undoStack;
    std::vector<HeightMapData_t> redoStack;
    int maxUndoSteps = 50;

    // === 3D VIEW ===
    OpenGLWidget *glWidget3D = nullptr;

    // === UTILITY FUNCTIONS ===
    void updateHeightmapDisplay();
    QPoint mapToDataCoordinates(int screenX, int screenY);
    void applyBrush(int mapX, int mapY);
    void applySmoothBrush(int mapX, int mapY);
    void applyFlattenBrush(int mapX, int mapY);
    void applyNoiseBrush(int mapX, int mapY);

    // === PERLIN NOISE FUNCTIONS ===
    void initializePerlin();
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y, double z);
    double perlin(double x, double y);
    double fbm(double x, double y);

    // === SIMPLEX NOISE FUNCTIONS ===
    double simplexNoise(double x, double y);
    double simplexFbm(double x, double y);

    // === VORONOI NOISE FUNCTIONS ===
    double voronoiNoise(double x, double y, int numPoints = 20);
    double voronoiFbm(double x, double y);

    // === RIDGED MULTIFRACTAL ===
    double ridgedMultifractal(double x, double y);

    // === BILLOWY NOISE ===
    double billowyNoise(double x, double y);
    double billowyFbm(double x, double y);

    // === DOMAIN WARPING ===
    double domainWarp(double x, double y, double warpStrength = 0.5);

    // === VORONOI VARIABLES ===
    std::vector<std::pair<double, double>> voronoiPoints;
    int voronoiNumPoints = 20;

    // === UNDO/REDO FUNCTIONS ===
    void saveStateToUndo();
    void undo();
    void redo();
    void clearRedoStack();

    // DIBUJO DE FORMAS
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRectangle(int x1, int y1, int x2, int y2);
    void drawCircle(int centerX, int centerY, int radius);
};

#endif // MAINWINDOW_H
