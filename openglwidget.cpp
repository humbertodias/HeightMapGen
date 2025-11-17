#include "openglwidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>
#include <algorithm>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    rotationX = -60.0f;
    rotationY = 45.0f;
    zoom = 4.0f;
    cameraX = 0.0f;
    cameraY = 0.0f;
    cameraZ = 0.0f;
    moveSpeed = 5.0f;

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // IMPORTANTE: Inicializar colorMap vacío
    // Se llenará cuando se llame a setHeightMapData()

    qDebug() << "OpenGLWidget constructor called";
}

OpenGLWidget::~OpenGLWidget()
{
    makeCurrent();

    if (terrainShader) {
        delete terrainShader;
        terrainShader = nullptr;
    }
    if (waterShader) {
        delete waterShader;
        waterShader = nullptr;
    }

    if (terrainVAO) {
        terrainVAO->destroy();
        delete terrainVAO;
    }
    if (terrainVBO) {
        terrainVBO->destroy();
        delete terrainVBO;
    }
    if (terrainEBO) {
        terrainEBO->destroy();
        delete terrainEBO;
    }

    if (waterVAO) {
        waterVAO->destroy();
        delete waterVAO;
    }
    if (waterVBO) {
        waterVBO->destroy();
        delete waterVBO;
    }
    if (waterEBO) {
        waterEBO->destroy();
        delete waterEBO;
    }

    if (terrainTexture) {
        delete terrainTexture;
        terrainTexture = nullptr;
    }
    if (waterTexture) {
        delete waterTexture;
        waterTexture = nullptr;
    }

    // NUEVO: Limpiar texturas de terrain
    for (auto* tex : terrainTextures) {
        if (tex) {
            delete tex;
        }
    }
    terrainTextures.clear();

    doneCurrent();
}

void OpenGLWidget::setupShaders()
{
    // Shader para terreno
    terrainShader = new QOpenGLShaderProgram(this);
    if (!terrainShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/terrain.vert")) {
        qDebug() << "ERROR: Failed to compile terrain vertex shader:" << terrainShader->log();
        return;
    }
    if (!terrainShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/terrain.frag")) {
        qDebug() << "ERROR: Failed to compile terrain fragment shader:" << terrainShader->log();
        return;
    }
    if (!terrainShader->link()) {
        qDebug() << "ERROR: Failed to link terrain shader:" << terrainShader->log();
        return;
    }

    // Shader para agua (mantener igual)
    waterShader = new QOpenGLShaderProgram(this);
    if (!waterShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/water.vert")) {
        qDebug() << "ERROR: Failed to compile water vertex shader:" << waterShader->log();
        return;
    }
    if (!waterShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/water.frag")) {
        qDebug() << "ERROR: Failed to compile water fragment shader:" << waterShader->log();
        return;
    }
    if (!waterShader->link()) {
        qDebug() << "ERROR: Failed to link water shader:" << waterShader->log();
        return;
    }

    qDebug() << "Shaders compiled and linked successfully";
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // MODIFICADO: Solo habilitar culling si NO estamos en modo pintado
    if (!texturePaintMode) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    qDebug() << "=== INITIALIZING SHADERS ===";

    if (terrainShader || waterShader) {
        qDebug() << "ERROR: Shaders already exist before setup!";
    }

    setupShaders();

    if (!terrainShader || !waterShader) {
        qDebug() << "CRITICAL ERROR: Shaders failed to initialize!";
        qDebug() << "terrainShader:" << (terrainShader != nullptr);
        qDebug() << "waterShader:" << (waterShader != nullptr);
        qDebug() << "FALLING BACK TO FIXED PIPELINE - WILL CRASH WITH LARGE MAPS";
    } else {
        qDebug() << "SUCCESS: Shaders initialized correctly";
        qDebug() << "terrainShader valid:" << terrainShader->isLinked();
        qDebug() << "waterShader valid:" << waterShader->isLinked();
    }

    if (terrainShader && waterShader) {
        terrainVAO = new QOpenGLVertexArrayObject(this);
        terrainVAO->create();

        terrainVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        terrainVBO->create();

        terrainEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        terrainEBO->create();

        waterVAO = new QOpenGLVertexArrayObject(this);
        waterVAO->create();

        waterVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        waterVBO->create();

        waterEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        waterEBO->create();

        qDebug() << "VAOs and VBOs created successfully";
    }

    qDebug() << "OpenGL initialized successfully";

    if (!heightMapData.empty() && mapWidth > 0 && mapHeight > 0) {
        qDebug() << "Generating deferred meshes...";
        generateMesh();
        generateWaterMesh();

        qDebug() << "Splatmap initialized:" << mapWidth << "x" << mapHeight;
    }
}

void OpenGLWidget::setupTerrainBuffers()
{
    if (vertices.empty() || indices.empty()) {
        qDebug() << "ERROR: No terrain data to upload";
        return;
    }

    terrainVAO->bind();

    terrainVBO->bind();
    terrainVBO->allocate(vertices.data(), vertices.size() * sizeof(float));

    terrainShader->bind();

    terrainShader->enableAttributeArray(0);
    terrainShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));

    terrainShader->enableAttributeArray(1);
    terrainShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));

    terrainShader->enableAttributeArray(2);
    terrainShader->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));

    terrainEBO->bind();
    terrainEBO->allocate(indices.data(), indices.size() * sizeof(unsigned int));

    terrainVAO->release();
    terrainShader->release();

    qDebug() << "Terrain buffers configured";
}

void OpenGLWidget::setupWaterBuffers()
{
    if (waterVertices.empty() || waterIndices.empty()) {
        qDebug() << "No water data to upload";
        return;
    }

    waterVAO->bind();

    waterVBO->bind();
    waterVBO->allocate(waterVertices.data(), waterVertices.size() * sizeof(float));

    waterShader->bind();

    waterShader->enableAttributeArray(0);
    waterShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));

    waterShader->enableAttributeArray(1);
    waterShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));

    waterShader->enableAttributeArray(2);
    waterShader->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));

    waterEBO->bind();
    waterEBO->allocate(waterIndices.data(), waterIndices.size() * sizeof(unsigned int));

    waterVAO->release();
    waterShader->release();

    qDebug() << "Water buffers configured with texture coordinates";
}

void OpenGLWidget::generateMesh()
{
    qDebug() << "generateMesh called";

    vertices.clear();
    indices.clear();

    if (heightMapData.empty() || mapWidth == 0 || mapHeight == 0) {
        qDebug() << "ERROR: Cannot generate mesh - no heightmap data";
        return;
    }

    // Reservar memoria
    size_t totalVertices = static_cast<size_t>(mapWidth) * static_cast<size_t>(mapHeight);
    size_t totalIndices = static_cast<size_t>(mapWidth - 1) * static_cast<size_t>(mapHeight - 1) * 6;

    vertices.reserve(totalVertices * 8);
    indices.reserve(totalIndices);

    qDebug() << "Reserved memory for" << totalVertices << "vertices and" << totalIndices << "indices";

    // Generar vértices
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            float height = heightMapData[y][x] / 255.0f * 100.0f;

            vertices.push_back(static_cast<float>(x));
            vertices.push_back(height);
            vertices.push_back(static_cast<float>(y));

            // Usar colorMap solo si NO está vacío y tiene las dimensiones correctas
            float r, g, b;
            if (!colorMap.empty() &&
                colorMap.size() == static_cast<size_t>(mapHeight) &&
                !colorMap[y].empty() &&
                colorMap[y].size() == static_cast<size_t>(mapWidth) &&
                colorMap[y][x].isValid()) {
                // Usar color pintado
                r = colorMap[y][x].redF();
                g = colorMap[y][x].greenF();
                b = colorMap[y][x].blueF();
            } else {
                // Usar colores por altura (sistema original)
                if (height < 20.0f) {
                    r = 0.2f; g = 0.4f; b = 0.8f;
                } else if (height < 40.0f) {
                    r = 0.76f; g = 0.7f; b = 0.5f;
                } else if (height < 60.0f) {
                    r = 0.2f; g = 0.6f; b = 0.2f;
                } else if (height < 80.0f) {
                    r = 0.5f; g = 0.5f; b = 0.5f;
                } else {
                    r = 1.0f; g = 1.0f; b = 1.0f;
                }
            }

            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);

            vertices.push_back(static_cast<float>(x) / mapWidth);
            vertices.push_back(static_cast<float>(y) / mapHeight);
        }
    }

    // Generar índices (sin cambios)
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            unsigned int topLeft = y * mapWidth + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (y + 1) * mapWidth + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    qDebug() << "Mesh generated:" << vertices.size() / 8 << "vertices,"
             << indices.size() / 3 << "triangles";

    setupTerrainBuffers();
}
void OpenGLWidget::generateWaterMesh()
{
    waterVertices.clear();
    waterIndices.clear();

    if (mapWidth <= 0 || mapHeight <= 0 || heightMapData.empty()) {
        qDebug() << "ERROR: Invalid map dimensions for water mesh";
        return;
    }

    float waterHeight = waterLevel;
    unsigned int vertexIndex = 0;

    // Generar agua solo en zonas bajas del terreno
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            // Obtener las alturas de las 4 esquinas de la celda
            float h1 = heightMapData[y][x] / 255.0f * 100.0f;
            float h2 = heightMapData[y][x + 1] / 255.0f * 100.0f;
            float h3 = heightMapData[y + 1][x + 1] / 255.0f * 100.0f;
            float h4 = heightMapData[y + 1][x] / 255.0f * 100.0f;

            // Solo generar agua si AL MENOS UNA esquina está bajo el nivel del agua
            if (h1 < waterHeight || h2 < waterHeight ||
                h3 < waterHeight || h4 < waterHeight) {

                // Vértice 0: esquina superior izquierda (X, Y, Z, R, G, B, U, V)
                waterVertices.push_back(static_cast<float>(x));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x) / mapWidth);
                waterVertices.push_back(static_cast<float>(y) / mapHeight);

                // Vértice 1: esquina superior derecha
                waterVertices.push_back(static_cast<float>(x + 1));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x + 1) / mapWidth);
                waterVertices.push_back(static_cast<float>(y) / mapHeight);

                // Vértice 2: esquina inferior derecha
                waterVertices.push_back(static_cast<float>(x + 1));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y + 1));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x + 1) / mapWidth);
                waterVertices.push_back(static_cast<float>(y + 1) / mapHeight);

                // Vértice 3: esquina inferior izquierda
                waterVertices.push_back(static_cast<float>(x));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y + 1));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x) / mapWidth);
                waterVertices.push_back(static_cast<float>(y + 1) / mapHeight);

                // Dos triángulos para formar el quad
                waterIndices.push_back(vertexIndex);
                waterIndices.push_back(vertexIndex + 1);
                waterIndices.push_back(vertexIndex + 2);

                waterIndices.push_back(vertexIndex);
                waterIndices.push_back(vertexIndex + 2);
                waterIndices.push_back(vertexIndex + 3);

                vertexIndex += 4;
            }
        }
    }

    qDebug() << "Water mesh generated:" << vertexIndex / 4 << "quads at level:" << waterLevel;

    // Configurar buffers después de generar geometría
    setupWaterBuffers();
}
void OpenGLWidget::setWaterLevel(float level)
{
    waterLevel = level;
    generateWaterMesh();
    update();

    qDebug() << "Water level set to:" << waterLevel;
}

void OpenGLWidget::loadTexture(const QString &path)
{
    qDebug() << "Loading texture from:" << path;

    if (terrainTexture) {
        delete terrainTexture;
        terrainTexture = nullptr;
    }

    QImage image(path);
    if (image.isNull()) {
        qDebug() << "ERROR: Failed to load texture";
        useTexture = false;
        return;
    }

    terrainTexture = new QOpenGLTexture(image.flipped(Qt::Vertical));
    terrainTexture->setMinificationFilter(QOpenGLTexture::Linear);
    terrainTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    terrainTexture->setWrapMode(QOpenGLTexture::Repeat);

    useTexture = true;
    update();

    qDebug() << "Texture loaded successfully";
}

void OpenGLWidget::loadWaterTexture(const QString &path)
{
    qDebug() << "Loading water texture from:" << path;

    if (waterTexture) {
        delete waterTexture;
        waterTexture = nullptr;
    }

    QImage image(path);
    if (image.isNull()) {
        qDebug() << "ERROR: Failed to load water texture";
        useWaterTexture = false;
        return;
    }

    waterTexture = new QOpenGLTexture(image.flipped(Qt::Vertical));
    waterTexture->setMinificationFilter(QOpenGLTexture::Linear);
    waterTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    waterTexture->setWrapMode(QOpenGLTexture::Repeat);

    useWaterTexture = true;
    update();

    qDebug() << "Water texture loaded successfully";
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    projection.setToIdentity();

    // Usar proyección ortográfica en modo pintura
    if (texturePaintMode) {
        float aspect = (float)w / (float)h;
        float orthoSize = zoom * 50.0f;
        projection.ortho(-orthoSize * aspect, orthoSize * aspect,
                         -orthoSize, orthoSize,
                         -5000.0f, 5000.0f);  // CAMBIO: Planos más amplios
    } else {
        // Proyección perspectiva para visualización normal
        projection.perspective(45.0f, (float)w / (float)h, 0.1f, 1000.0f);
    }
}

void OpenGLWidget::loadTerrainTexture(const QString &path)
{
    qDebug() << "=== LOADING TERRAIN TEXTURE ===";
    qDebug() << "Loading terrain texture for splatting:" << path;

    QImage image(path);
    if (image.isNull()) {
        qDebug() << "ERROR: Failed to load terrain texture from:" << path;
        return;
    }

    qDebug() << "Image loaded successfully. Size:" << image.width() << "x" << image.height();

    QOpenGLTexture *newTexture = new QOpenGLTexture(image.flipped(Qt::Vertical));
    newTexture->setMinificationFilter(QOpenGLTexture::Linear);
    newTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    newTexture->setWrapMode(QOpenGLTexture::Repeat);

    terrainTextures.push_back(newTexture);

    qDebug() << "Terrain texture loaded. Total textures:" << terrainTextures.size();
    qDebug() << "=== END LOADING TEXTURE ===";
    update();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();

    // NUEVO: Validar que el clic está dentro del widget
    if (!rect().contains(event->pos())) {
        qDebug() << "Click outside widget bounds, ignoring";
        return;
    }

    if (texturePaintMode && event->button() == Qt::LeftButton) {
        applyTextureBrush(event->pos());
    }
}
void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->pos().x() - lastMousePos.x();
    int dy = event->pos().y() - lastMousePos.y();

    // Actualizar posición del cursor SIEMPRE
    currentMousePos = event->pos();
    showBrushCursor = true;

    if (texturePaintMode) {
        // Modo pintura
        if (event->buttons() & Qt::LeftButton) {
            applyTextureBrush(event->pos());
        } else if (event->buttons() & Qt::RightButton) {
            // Rotar cámara con botón derecho
            rotationY += dx * 0.2f;
            rotationX += dy * 0.2f;
        }
    } else {
        // Modo cámara normal
        if (event->buttons() & Qt::LeftButton) {
            rotationY += dx * 0.5f;
            rotationX += dy * 0.5f;
        }
    }

    lastMousePos = event->pos();
    update();  // CRÍTICO: Esto redibuja el widget incluyendo el cursor
}

void OpenGLWidget::wheelEvent(QWheelEvent *event)
{
    zoom -= event->angleDelta().y() / 120.0f;

    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 20.0f) zoom = 20.0f;

    update();
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        cameraZ += moveSpeed;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        cameraZ -= moveSpeed;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        cameraX -= moveSpeed;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        cameraX += moveSpeed;
        break;
    case Qt::Key_E:
        cameraY += moveSpeed;
        break;
    case Qt::Key_Q:
        cameraY -= moveSpeed;
        break;
    case Qt::Key_R:
        cameraX = 0.0f;
        cameraY = 0.0f;
        cameraZ = 0.0f;
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    update();
}
void OpenGLWidget::setHeightMapData(const std::vector<std::vector<unsigned char>>& data)
{
    qDebug() << "setHeightMapData called";

    if (data.empty() || data[0].empty()) {
        qDebug() << "WARNING: Empty heightmap data!";
        return;
    }

    qDebug() << "Copying heightMapData...";
    heightMapData = data;
    mapHeight = data.size();
    mapWidth = data[0].size();

    qDebug() << "Map dimensions:" << mapWidth << "x" << mapHeight;

    // NUEVO: Inicializar colorMap si estamos en modo pintado
    if (texturePaintMode) {
        qDebug() << "Texture paint mode active, initializing colorMap...";
        colorMap.assign(mapHeight, std::vector<QColor>(mapWidth, QColor(Qt::transparent)));
        qDebug() << "colorMap initialized with dimensions:" << mapWidth << "x" << mapHeight;
    }

    qDebug() << "Checking OpenGL context...";
    if (context() && context()->isValid()) {
        qDebug() << "Calling generateMesh()...";
        generateMesh();
        qDebug() << "generateMesh() completed";

        qDebug() << "Calling generateWaterMesh()...";
        generateWaterMesh();
        qDebug() << "generateWaterMesh() completed";

        qDebug() << "Calling update()...";
        update();
        qDebug() << "update() completed";
    } else {
        qDebug() << "OpenGL not ready yet, deferring mesh generation";
    }

    qDebug() << "setHeightMapData finished successfully";
}

// NUEVOS MÉTODOS PARA PINTURA DE TEXTURAS
void OpenGLWidget::setTexturePaintMode(bool enabled)
{
    qDebug() << "setTexturePaintMode called with:" << enabled;
    texturePaintMode = enabled;

    if (!enabled) {
        qDebug() << "Texture paint mode disabled";
        return;
    }

    // CRÍTICO: Verificar si colorMap ya está correctamente inicializado
    bool colorMapValid = !colorMap.empty() &&
                         colorMap.size() == static_cast<size_t>(mapHeight);

    if (colorMapValid && !colorMap[0].empty() &&
        colorMap[0].size() == static_cast<size_t>(mapWidth)) {
        qDebug() << "colorMap already initialized with correct dimensions, skipping";
        return;  // SALIDA TEMPRANA - No hacer nada más
    }

    // Si llegamos aquí, necesitamos inicializar o reinicializar
    if (mapWidth <= 0 || mapHeight <= 0) {
        qDebug() << "ERROR: Invalid map dimensions:" << mapWidth << "x" << mapHeight;
        return;
    }

    qDebug() << "Texture paint mode active, initializing colorMap...";

    // Limpiar colorMap existente si tiene dimensiones incorrectas
    if (!colorMap.empty()) {
        qDebug() << "Clearing existing colorMap with incorrect dimensions";
        colorMap.clear();
    }

    // Inicializar fila por fila para evitar fragmentación
    try {
        colorMap.reserve(mapHeight);

        for (int y = 0; y < mapHeight; ++y) {
            std::vector<QColor> row;
            row.reserve(mapWidth);

            for (int x = 0; x < mapWidth; ++x) {
                row.push_back(QColor(Qt::transparent));
            }

            colorMap.push_back(std::move(row));

            if (y % 100 == 0) {
                qDebug() << "Initialized row" << y << "of" << mapHeight;
            }
        }

        qDebug() << "colorMap initialization completed successfully";
    } catch (const std::exception& e) {
        qDebug() << "ERROR: Failed to initialize colorMap:" << e.what();
        colorMap.clear();
    }

    qDebug() << "setTexturePaintMode completed successfully";
}

void OpenGLWidget::setCurrentTexture(int index)
{
    currentTextureIndex = index;
    qDebug() << "Current texture index set to:" << index;
}

void OpenGLWidget::setTextureBrushSize(int size)
{
    textureBrushSize = size;
    qDebug() << "Texture brush size set to:" << size;
}

void OpenGLWidget::applyTextureBrush(const QPoint &screenPos)
{
    // Validar que el clic está dentro del widget
    if (screenPos.x() < 0 || screenPos.x() >= width() ||
        screenPos.y() < 0 || screenPos.y() >= height()) {
        qDebug() << "Click outside widget bounds:" << screenPos;
        return;
    }

    if (colorMap.empty() || heightMapData.empty()) {
        qDebug() << "Cannot paint: colorMap or heightMapData empty";
        return;
    }

    // Convertir coordenadas de pantalla a coordenadas del mapa
    // Usar la matriz MVP inversa para obtener coordenadas del mundo
    QMatrix4x4 mvp = projection * view * model;
    QMatrix4x4 mvpInverse = mvp.inverted();

    // Normalizar coordenadas de pantalla a [-1, 1]
    float normalizedX = (2.0f * screenPos.x()) / width() - 1.0f;
    float normalizedY = 1.0f - (2.0f * screenPos.y()) / height();

    // Proyectar al espacio del mundo (asumiendo Y=0 para el plano del terreno)
    QVector4D nearPoint(normalizedX, normalizedY, -1.0f, 1.0f);
    QVector4D farPoint(normalizedX, normalizedY, 1.0f, 1.0f);

    QVector4D nearWorld = mvpInverse * nearPoint;
    QVector4D farWorld = mvpInverse * farPoint;

    nearWorld /= nearWorld.w();
    farWorld /= farWorld.w();

    // Calcular intersección con el plano Y=0 (aproximación)
    float t = -nearWorld.y() / (farWorld.y() - nearWorld.y());
    float worldX = nearWorld.x() + t * (farWorld.x() - nearWorld.x());
    float worldZ = nearWorld.z() + t * (farWorld.z() - nearWorld.z());

    // Convertir de coordenadas del mundo a coordenadas del mapa
    // Recordar que el modelo está centrado con translate(-mapWidth/2, 0, -mapHeight/2)
    int mapX = static_cast<int>(worldX + mapWidth / 2.0f);
    int mapZ = static_cast<int>(worldZ + mapHeight / 2.0f);

    qDebug() << "Screen:" << screenPos << "-> World:" << worldX << worldZ
             << "-> Map:" << mapX << mapZ;

    // VALIDACIÓN CRÍTICA: Verificar límites ANTES de pintar
    if (mapX < 0 || mapX >= mapWidth || mapZ < 0 || mapZ >= mapHeight) {
        qDebug() << "Map coordinates out of bounds:" << mapX << mapZ
                 << "(map size:" << mapWidth << "x" << mapHeight << ")";
        return;
    }

    // Aplicar pincel con el color actual
    int brushRadius = textureBrushSize;
    int pixelsModified = 0;

    for (int dy = -brushRadius; dy <= brushRadius; ++dy) {
        for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
            int x = mapX + dx;
            int z = mapZ + dy;

            // Verificar límites para cada píxel
            if (x >= 0 && x < mapWidth && z >= 0 && z < mapHeight) {
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= brushRadius) {
                    colorMap[z][x] = currentPaintColor;
                    pixelsModified++;
                }
            }
        }
    }

    qDebug() << "Modified" << pixelsModified << "pixels at map coords:" << mapX << "," << mapZ;

    // Regenerar malla para ver los cambios
    generateMesh();
    update();
}
void OpenGLWidget::paintEvent(QPaintEvent *event)
{
    // Primero renderizar OpenGL
    QOpenGLWidget::paintEvent(event);

    // Luego dibujar el cursor encima con QPainter
    if (showBrushCursor && texturePaintMode) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Calcular radio del cursor en pantalla
        int screenRadius = textureBrushSize;

        // Dibujar círculo del pincel
        QPen pen(Qt::white);
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(currentMousePos, screenRadius, screenRadius);

        // Dibujar cruz central para precisión
        painter.setPen(QPen(Qt::white, 1));
        painter.drawLine(currentMousePos.x() - 5, currentMousePos.y(),
                         currentMousePos.x() + 5, currentMousePos.y());
        painter.drawLine(currentMousePos.x(), currentMousePos.y() - 5,
                         currentMousePos.x(), currentMousePos.y() + 5);
    }
}

void OpenGLWidget::leaveEvent(QEvent *event)
{
    showBrushCursor = false;
    update();
    QOpenGLWidget::leaveEvent(event);
}

void OpenGLWidget::paintGL()
{
    // NUEVO: Controlar culling según el modo
    if (texturePaintMode) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // Configurar matrices de transformación
    view.setToIdentity();
    view.translate(0.0f, -50.0f + cameraY, -zoom);
    view.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    view.rotate(rotationY, 0.0f, 1.0f, 0.0f);
    view.translate(-cameraX, 0.0f, -cameraZ);

    model.setToIdentity();
    model.translate(-mapWidth / 2.0f, 0.0f, -mapHeight / 2.0f);

    QMatrix4x4 mvp = projection * view * model;

    // RENDERIZAR TERRENO CON SHADER
    terrainShader->bind();
    terrainShader->setUniformValue("mvpMatrix", mvp);
    terrainShader->setUniformValue("useTexture", useTexture);

    if (useTexture && terrainTexture) {
        terrainTexture->bind(0);
        terrainShader->setUniformValue("textureSampler", 0);
    }

    terrainVAO->bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    terrainVAO->release();

    if (useTexture && terrainTexture) {
        terrainTexture->release();
    }
    terrainShader->release();

    // RENDERIZAR AGUA CON SHADER
    if (showWater && !waterVertices.empty() && !waterIndices.empty()) {
        glDisable(GL_CULL_FACE);

        waterShader->bind();
        waterShader->setUniformValue("mvpMatrix", mvp);
        waterShader->setUniformValue("waterAlpha", waterAlpha);

        // Configurar textura del agua si existe
        if (useWaterTexture && waterTexture) {
            waterShader->setUniformValue("useWaterTexture", true);
            waterShader->setUniformValue("waterTextureSampler", 0);
            glActiveTexture(GL_TEXTURE0);
            waterTexture->bind();
        } else {
            waterShader->setUniformValue("useWaterTexture", false);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        waterVAO->bind();
        glDrawElements(GL_TRIANGLES, waterIndices.size(), GL_UNSIGNED_INT, 0);
        waterVAO->release();

        glDepthMask(GL_TRUE);

        // Liberar textura si se usó
        if (useWaterTexture && waterTexture) {
            waterTexture->release();
        }

        waterShader->release();
        glEnable(GL_CULL_FACE);
    }
}

QVector3D OpenGLWidget::screenToWorld(const QPoint &screenPos)
{
    // Normalizar coordenadas de pantalla a NDC (-1 a 1)
    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();

    // Crear matriz MVP inversa
    QMatrix4x4 mvp = projection * view * model;
    QMatrix4x4 invMVP = mvp.inverted();

    // Punto cercano y lejano en espacio de clip
    QVector4D nearPoint(x, y, -1.0f, 1.0f);
    QVector4D farPoint(x, y, 1.0f, 1.0f);

    // Transformar a espacio del mundo
    QVector4D nearWorld = invMVP * nearPoint;
    QVector4D farWorld = invMVP * farPoint;

    // Dividir por w para obtener coordenadas homogéneas
    nearWorld /= nearWorld.w();
    farWorld /= farWorld.w();

    // Calcular dirección del rayo
    QVector3D rayDir = (farWorld.toVector3D() - nearWorld.toVector3D()).normalized();
    QVector3D rayOrigin = nearWorld.toVector3D();

    // Intersección con plano Y=0 (aproximación simple)
    // Si el rayo no intersecta, devolver origen
    if (qAbs(rayDir.y()) < 0.001f) {
        return rayOrigin;
    }

    float t = -rayOrigin.y() / rayDir.y();
    QVector3D intersection = rayOrigin + rayDir * t;

    return intersection;
}
void OpenGLWidget::setColorAtPosition(int x, int y, const QColor &color)
{
    // Validar coordenadas
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        qDebug() << "setColorAtPosition: coordenadas fuera de rango:" << x << y;
        return;
    }

    // Inicializar colorMap SOLO si está vacío (lazy initialization)
    if (colorMap.empty()) {
        qDebug() << "Lazy initialization of colorMap:" << mapWidth << "x" << mapHeight;

        try {
            // Reservar espacio para el vector externo
            colorMap.reserve(mapHeight);

            // Crear filas una por una
            for (int row = 0; row < mapHeight; ++row) {
                std::vector<QColor> rowVector;
                rowVector.reserve(mapWidth);

                // Llenar la fila con colores transparentes
                for (int col = 0; col < mapWidth; ++col) {
                    rowVector.push_back(QColor(Qt::transparent));
                }

                colorMap.push_back(std::move(rowVector));

                // Log cada 100 filas para monitorear progreso
                if (row % 100 == 0) {
                    qDebug() << "Initialized row" << row << "of" << mapHeight;
                }
            }

            qDebug() << "colorMap lazy initialization completed successfully";
        } catch (const std::bad_alloc& e) {
            qDebug() << "ERROR: Failed to allocate colorMap:" << e.what();
            colorMap.clear();
            return;
        }
    }

    // Establecer el color en la posición especificada
    colorMap[y][x] = color;
}

QImage OpenGLWidget::generateColorMapImage() const
{
    if (mapWidth == 0 || mapHeight == 0) {
        return QImage();
    }

    QImage image(mapWidth, mapHeight, QImage::Format_RGB32);

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            if (!colorMap.empty() && colorMap[y][x].isValid()) {
                // Usar color pintado
                image.setPixel(x, y, colorMap[y][x].rgb());
            } else {
                // Usar color basado en altura (escala de grises)
                unsigned char height = heightMapData[y][x];
                image.setPixel(x, y, qRgb(height, height, height));
            }
        }
    }

    return image;
}

void OpenGLWidget::setCurrentPaintColor(const QColor &color)
{
    currentPaintColor = color;
    qDebug() << "Paint color set to:" << color.name();
}
