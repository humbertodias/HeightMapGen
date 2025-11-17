#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QPainter>
#include <vector>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

    void setHeightMapData(const std::vector<std::vector<unsigned char>>& data);
    void loadTexture(const QString &path);
    void loadWaterTexture(const QString &path);
    void setWaterLevel(float level);

    // NUEVO: Métodos para modo de pintura de texturas
    void setTexturePaintMode(bool enabled);
    void setCurrentTexture(int index);
    void setTextureBrushSize(int size);
    void loadTerrainTexture(const QString &path);
    void setCurrentPaintColor(const QColor &color);
    void setColorAtPosition(int x, int y, const QColor &color);
    void generateMesh();
    QImage generateColorMapImage() const;
    bool showWater = true;
    float waterLevel = 50.0f;
    bool colorMapValid = !colorMap.empty() &&
                         colorMap.size() == static_cast<size_t>(mapHeight);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void paintEvent(QPaintEvent *event) override;  // NUEVO
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;  // NUEVO

private:

    void generateWaterMesh();
    void setupShaders();
    void setupTerrainBuffers();
    void setupWaterBuffers();
    void applyTextureBrush(const QPoint &screenPos);  // NUEVO
    QVector3D screenToWorld(const QPoint &screenPos);
    // Datos del heightmap
    std::vector<std::vector<unsigned char>> heightMapData;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int mapWidth = 0;
    int mapHeight = 0;

    // Parámetros de cámara
    float rotationX = -60.0f;
    float rotationY = 45.0f;
    float zoom = 4.0f;
    float cameraX = 0.0f;
    float cameraZ = 0.0f;
    float moveSpeed = 5.0f;
    float cameraY = 0.0f;

    QPoint lastMousePos;
    QPoint currentMousePos;  // NUEVO
    bool showBrushCursor = false;  // NUEVO

    // Sistema de texturas
    QOpenGLTexture *terrainTexture = nullptr;
    bool useTexture = false;
    QOpenGLTexture *waterTexture = nullptr;
    bool useWaterTexture = false;

    // NUEVO: Sistema de pintura de texturas
    bool texturePaintMode = false;
    int currentTextureIndex = 0;
    int textureBrushSize = 20;
    std::vector<std::vector<int>> textureMap;

    // Sistema de agua
    QVector3D waterColor = QVector3D(0.2f, 0.4f, 0.8f);
    float waterAlpha = 0.6f;
    std::vector<float> waterVertices;
    std::vector<unsigned int> waterIndices;

    // Proyección y transformaciones
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;

    // Sistema de shaders
    QOpenGLShaderProgram *terrainShader = nullptr;
    QOpenGLShaderProgram *waterShader = nullptr;

    // Buffers para terreno
    QOpenGLBuffer *terrainVBO = nullptr;
    QOpenGLBuffer *terrainEBO = nullptr;
    QOpenGLVertexArrayObject *terrainVAO = nullptr;

    // Buffers para agua
    QOpenGLBuffer *waterVBO = nullptr;
    QOpenGLBuffer *waterEBO = nullptr;
    QOpenGLVertexArrayObject *waterVAO = nullptr;

    // NUEVO: Mapa de colores para pintura
    std::vector<std::vector<QColor>> colorMap;
    QColor currentPaintColor = Qt::red;  // Color actual del pincel

    // Sistema de texture splatting
    std::vector<QOpenGLTexture*> terrainTextures;
    QOpenGLTexture *splatMapTexture = nullptr;
    QImage splatMapImage;
    bool needsSplatMapUpdate = false;
};

#endif // OPENGLWIDGET_H
