#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <QImage>
#include <QEvent>
#include <QLabel>     // Importante: para que QLabel sea conocido
#include <QMouseEvent>
#include <random>       // Necesario para std::shuffle
#include <numeric>      // Necesario para std::iota
#include <chrono>       // Necesario para la semilla de tiempo en initializePerlin

// Definición de tipos de datos C++
using HeightMapData_t = std::vector<std::vector<unsigned char>>;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    // Sobrescribir los eventos del ratón para capturar clics
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_pushButtonCreate_clicked();
    void on_pushButtonSave_clicked();
    void on_pushButtonGenerate_clicked(); // Slot para la generación de Ruido Perlin

private:
    Ui::MainWindow *ui;

    // Puntero para el QLabel que contendrá la imagen del mapa de altura
    QLabel *dynamicImageLabel = nullptr;

    // Variables de la aplicación
    HeightMapData_t heightMapData;
    QImage currentImage;
    int mapWidth = 0;
    int mapHeight = 0;
    bool isPainting = false;
    int brushHeight = 128; // Altura a pintar (0-255)

    // === PERLIN NOISE VARIABLES ===
    std::vector<int> p; // Array de permutaciones (512 elementos)

    // === FBM (FRACTAL BROWNIAN MOTION) / OCTAVES VARIABLES ===
    int octaves = 6;            // Número de capas de ruido. 6 es un buen valor por defecto.
    double persistence = 0.5;   // La amplitud de cada octava subsiguiente.
    double frequencyOffset = 1000.0; // Desplazamiento base para cambiar el sector muestreado.
    double frequencyScale = 8.0; // Nueva variable para controlar el zoom/detalle.

    // === PERLIN NOISE & FBM FUNCTIONS ===
    void initializePerlin();
    double perlin(double x, double y);
    double fbm(double x, double y); // Nueva función para Octavas Múltiples
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y, double z);

    // Funciones de utilidad
    void updateHeightmapDisplay();
    QPoint mapToDataCoordinates(int screenX, int screenY);
    void applyBrush(int mapX, int mapY);
};

#endif // MAINWINDOW_H
