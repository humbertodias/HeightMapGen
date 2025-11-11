#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
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
#include <random>          // Para std::default_random_engine, std::shuffle
#include <numeric>         // Para std::iota
#include <chrono>          // Para la semilla de tiempo
#include <sstream>         // Para convertir string a double de forma segura

// =================================================================
// === 0. CONSTRUCTOR Y DESTRUCTOR
// =================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // El filtro de eventos se instala en el QScrollArea para capturar clics
    if (ui->scrollAreaDisplay) {
        ui->scrollAreaDisplay->installEventFilter(this);
    }

    // Inicializar variables internas
    isPainting = false;
    mapWidth = 0;
    mapHeight = 0;
    brushHeight = 128;
    dynamicImageLabel = nullptr;

    // Valores por defecto
    ui->lineEditWidth->setText("512");
    ui->lineEditHeight->setText("512");

    // Configuración del slider del pincel
    if (ui->sliderBrushSize) {
        ui->sliderBrushSize->setRange(1, 100);
        ui->sliderBrushSize->setValue(10);
    }

    // Configuración inicial de los nuevos controles de Perlin (FBM)
    if (ui->spinBoxOctaves) {
        ui->spinBoxOctaves->setRange(1, 10);
        ui->spinBoxOctaves->setValue(6);
    }
    if (ui->doubleSpinBoxPersistence) {
        ui->doubleSpinBoxPersistence->setRange(0.1, 0.9);
        ui->doubleSpinBoxPersistence->setSingleStep(0.05);
        ui->doubleSpinBoxPersistence->setValue(0.55);
    }
    if (ui->doubleSpinBoxFrequencyScale) { // NUEVO CONTROL
        ui->doubleSpinBoxFrequencyScale->setRange(1.0, 50.0);
        ui->doubleSpinBoxFrequencyScale->setSingleStep(0.5);
        ui->doubleSpinBoxFrequencyScale->setValue(8.0); // 8.0 es el valor de "zoom" por defecto
        frequencyScale = 8.0;
    }
    if (ui->lineEditOffset) {
        ui->lineEditOffset->setText("Aleatorio"); // Valor por defecto
    }

    // La inicialización de Perlin (initializePerlin()) ahora se llama
    // dentro de on_pushButtonGenerate_clicked() para obtener una semilla diferente cada vez.
}

MainWindow::~MainWindow()
{
    // Asegurarse de eliminar el QLabel dinámico si todavía existe
    if (dynamicImageLabel) {
        delete dynamicImageLabel;
    }
    delete ui;
}

// =================================================================
// === 0. FILTRO DE EVENTOS (Captura de clics en el QScrollArea)
// =================================================================
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Solo actuamos si el evento ocurrió en el QScrollArea y el QLabel dinámico existe
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


// =================================================================
// === 1. CREACIÓN DEL MAPA (CON MANEJO SEGURO DEL QLabel)
// =================================================================
void MainWindow::on_pushButtonCreate_clicked()
{
    int newMapWidth = ui->lineEditWidth->text().toInt();
    int newMapHeight = ui->lineEditHeight->text().toInt();

    // Validación y límites de tamaños
    if (newMapWidth < 16 || newMapHeight < 16 || newMapWidth > 4096 || newMapHeight > 4096) {
        newMapWidth = 512;
        newMapHeight = 512;
        QMessageBox::warning(this, "Advertencia de Tamaño", "El tamaño debe estar entre 16 y 4096. Usando 512x512.");
        ui->lineEditWidth->setText("512");
        ui->lineEditHeight->setText("512");
    }

    mapWidth = newMapWidth;
    mapHeight = newMapHeight;

    // Inicializar mapa de altura a gris medio (128)
    heightMapData.assign(mapHeight, std::vector<unsigned char>(mapWidth, 128));
    currentImage = QImage(mapWidth, mapHeight, QImage::Format_RGB32);

    // 1. GESTIÓN SEGURA DEL QLabel DINÁMICO
    if (dynamicImageLabel) {
        delete dynamicImageLabel;
        dynamicImageLabel = nullptr;
    }

    dynamicImageLabel = new QLabel(ui->scrollAreaDisplay);
    dynamicImageLabel->setFixedSize(mapWidth, mapHeight);
    ui->scrollAreaDisplay->setWidget(dynamicImageLabel);

    // 2. CÁLCULO DE TAMAÑO DE LA VENTANA Y SCROLL AREA
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    int scrollAreaWidth = std::min(mapWidth, maxScrollWidth);
    int scrollAreaHeight = std::min(mapHeight, maxScrollHeight);

    // Reposicionar y redimensionar el QScrollArea
    ui->scrollAreaDisplay->setGeometry(180, 20, scrollAreaWidth, scrollAreaHeight);

    // 3. Aplicar el tamaño a la ventana principal
    int requiredWidth = 180 + scrollAreaWidth + 20;
    int requiredHeight = 20 + scrollAreaHeight + 50;

    if (requiredHeight < 300) requiredHeight = 300;

    QSize newSize(requiredWidth, requiredHeight);
    this->setFixedSize(newSize);

    updateHeightmapDisplay();
}

// =================================================================
// === 2. FUNCIÓN DE TRANSFERENCIA DE DATOS (Matriz a QImage)
// =================================================================
void MainWindow::updateHeightmapDisplay()
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    // Convertir datos de altura (0-255) a imagen QImage en escala de grises
    for (int y = 0; y < mapHeight; ++y) {
        QRgb *pixel = reinterpret_cast<QRgb*>(currentImage.scanLine(y));

        for (int x = 0; x < mapWidth; ++x) {
            unsigned char value = heightMapData[y][x];
            // Establecer el color RGB con el mismo valor para escala de grises
            *pixel = qRgb(value, value, value);
            pixel++;
        }
    }

    dynamicImageLabel->setPixmap(QPixmap::fromImage(currentImage));
}


// =================================================================
// === 3. EXPORTACIÓN A PNG
// =================================================================
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

// =================================================================
// === 4. LÓGICA DE PINTADO INTERACTIVO
// =================================================================

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

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) + std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);

                int currentValue = heightMapData[y][x];
                // Suavizado exponencial para un pintado gradual
                int targetValue = static_cast<int>(currentValue + (brushHeight - currentValue) * intensity * 0.05);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

// =================================================================
// === 5. FUNCIONES DE RUIDO PERLIN (Implementación Estándar)
// =================================================================

void MainWindow::initializePerlin()
{
    // Inicializa el array de permutaciones 'p' (0 a 255) y lo duplica (total 512)
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);

    // Semilla basada en el tiempo para resultados aleatorios en cada ejecución
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(p.begin(), p.end(), std::default_random_engine(seed));

    // Duplicar el array para el cálculo de índices (p[i + 256])
    p.insert(p.end(), p.begin(), p.end());

    // Generar un offset aleatorio para el desplazamiento de la frecuencia base
    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<> distrib(100.0, 5000.0);
    frequencyOffset = distrib(gen);
}

double MainWindow::fade(double t)
{
    // Función de suavizado de Perlin (6t^5 - 15t^4 + 10t^3)
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double MainWindow::lerp(double t, double a, double b)
{
    // Interpolación lineal
    return a + t * (b - a);
}

double MainWindow::grad(int hash, double x, double y, double z)
{
    // Proyección del vector gradiente según el valor del hash
    int h = hash & 15;
    double u = (h < 8) ? x : y;
    double v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double MainWindow::perlin(double x, double y)
{
    if (p.empty()) initializePerlin();

    // Coordenadas de la rejilla (índice base)
    int X = (int)std::floor(x);
    int Y = (int)std::floor(y);

    // Coordenadas relativas
    x -= std::floor(x);
    y -= std::floor(y);
    double z = 0.0; // Fijo para 2D

    // Suavizado (Fade)
    double u = fade(x);
    double v = fade(y);

    // Mapeo a la tabla de permutaciones (usando & 255)
    int A = p[(X & 255)] + (Y & 255);
    int B = p[(X + 1) & 255] + (Y & 255);

    // Hash de los 4 puntos de la rejilla. Se usa & 511 para manejar la tabla duplicada.
    int AA = p[A & 511] + 0;
    int AB = p[B & 511] + 0;
    int BA = p[A & 511] + 1;
    int BB = p[B & 511] + 1;

    // Interpolación de gradientes. El resultado está entre -1.0 y 1.0
    return lerp(v, lerp(u, grad(p[AA], x, y, z),
                        grad(p[BA], x - 1, y, z)),
                lerp(u, grad(p[AB], x, y - 1, z),
                     grad(p[BB], x - 1, y - 1, z)));
}

// =================================================================
// === 5.5 FRACTAL BROWNIAN MOTION (FBM) / OCTAVAS MÚLTIPLES
// =================================================================
double MainWindow::fbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0; // Para normalización

    for (int i = 0; i < octaves; ++i) {
        // La frecuencia se dobla en cada octava (lacunarity implícita = 2.0)
        // La amplitud disminuye en cada octava (persistence)

        // El ruido Perlin da valores entre -1.0 y 1.0
        total += perlin(x * freq, y * freq) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    // Normalizar el resultado a [-1.0, 1.0]
    return total / maxVal;
}


// =================================================================
// === 6. GENERACIÓN DEL MAPA DE ALTURA (Botón 'Generar')
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
    frequencyScale = ui->doubleSpinBoxFrequencyScale->value(); // LEER NUEVO CONTROL

    QString offsetText = ui->lineEditOffset->text();
    if (offsetText.toLower() == "aleatorio" || offsetText.isEmpty()) {
        initializePerlin(); // Inicializa Perlin y genera un nuevo frequencyOffset aleatorio
    } else {
        // Intenta convertir el texto a un número (Semilla/Offset fijo)
        bool ok;
        double customOffset = offsetText.toDouble(&ok);
        if (ok) {
            frequencyOffset = customOffset;
        } else {
            // Si la conversión falla, usar aleatorio por seguridad
            initializePerlin();
            QMessageBox::warning(this, "Advertencia", "Desplazamiento no válido. Usando valor aleatorio.");
            ui->lineEditOffset->setText(QString::number(frequencyOffset));
        }
        // Llamar a initializePerlin() para inicializar la tabla 'p' si no se hizo.
        if (p.empty()) initializePerlin();
    }

    // Usamos la dimensión más pequeña para calcular la escala.
    double scale = std::min(mapWidth, mapHeight);

    // Frecuencia base: Ahora usa 'frequencyScale' (ej: 8.0, 4.0, 2.0)
    // Para mapas más detallados, el divisor (frequencyScale) debe ser más pequeño.
    const double baseFrequency = 1.0 / (scale * frequencyScale);

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {

            // Coordenadas de muestreo con desplazamiento (frequencyOffset)
            double sampleX = (double)x * baseFrequency + frequencyOffset;
            double sampleY = (double)y * baseFrequency + frequencyOffset;

            // Utilizamos FBM (Octavas Múltiples)
            double noiseValue = fbm(sampleX, sampleY);

            // Escalar el valor de ruido de [-1.0, 1.0] a [0, 255]
            unsigned char height = (unsigned char)((noiseValue + 1.0) * 127.5);

            heightMapData[y][x] = height;
        }
    }

    updateHeightmapDisplay();
    QMessageBox::information(this, "Éxito",
                             QString("Terreno generado con %1 Octavas, %2 de Persistencia y Escala Freq %3.")
                                 .arg(octaves).arg(persistence).arg(frequencyScale));
}

// =================================================================
// === EVENTOS DEL RATÓN (Usando el puntero dinámico)
// =================================================================

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // Solo procesar si hay un mapa y el evento es dentro del QLabel
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    QPoint globalPos = event->globalPosition().toPoint();

    // Convertir la posición global a coordenadas locales dentro del dynamicImageLabel
    QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

    if (!dynamicImageLabel->rect().contains(localPos)) return;

    isPainting = true;

    // Determinar la acción del pincel (pintar arriba o abajo)
    if (event->button() == Qt::LeftButton) {
        brushHeight = 180; // Levantar
    } else if (event->button() == Qt::RightButton) {
        brushHeight = 80; // Bajar
    }

    QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
    applyBrush(dataPos.x(), dataPos.y());
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isPainting && dynamicImageLabel) {
        QPoint globalPos = event->globalPosition().toPoint();
        QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

        if (!dynamicImageLabel->rect().contains(localPos)) return;

        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        applyBrush(dataPos.x(), dataPos.y());
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    isPainting = false;
}
