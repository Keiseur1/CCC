#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QSlider>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

// ==================== CẤU TRÚC DỮ LIỆU ĐỒ THỊ ====================
struct Node {
  int id;
  QString name;
  QPointF pos;
  QString category; // "poi", "bridge", "station", "park", "airport", "junction", "lake", "beach"
  QString icon;
};

struct Edge {
  int to;
  double distance;
  double trafficFactor;
  QString streetName;
  bool isBridge;
  bool isMajor;
  double baseTraffic = 1.0;
};

struct RouteStep {
  QString from, to;
  double meters;
  int seconds;
  double traffic;
  QColor color;
  QString instruction;
  QString street;
  QString icon;
};

struct Graph {
  QVector<Node> nodes;
  QVector<QVector<Edge>> adj;

  void addNode(int id, const QString &name, QPointF pos,
               const QString &category = "junction", const QString &icon = "📍") {
    nodes.append({id, name, pos, category, icon});
    adj.append(QVector<Edge>());
  }

  void addEdge(int u, int v, double dist, double traffic = 1.0,
               const QString &streetName = "", bool isBridge = false, bool isMajor = true) {
    adj[u].append({v, dist, traffic, streetName, isBridge, isMajor, traffic});
    adj[v].append({u, dist, traffic, streetName, isBridge, isMajor, traffic});
  }

  static double travelTimeOf(double distPx, double traffic, double v0 = 40.0) {
    double distKm = distPx * 0.005; // 1 pixel = 5 mét trong tỉ lệ bản đồ chuẩn
    double speed = std::max(5.0, v0 / std::max(0.1, traffic));
    return distKm / speed; // giờ
  }

  QVector<int> dijkstra(int start, int end, const QSet<int> &bannedNodes,
                        const QSet<QPair<int, int>> &bannedEdges,
                        double peakMul, double v0) const {
    const int n = nodes.size();
    QVector<double> time(n, std::numeric_limits<double>::max());
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    if (bannedNodes.contains(start) || bannedNodes.contains(end))
      return {};

    time[start] = 0.0;

    for (int i = 0; i < n; ++i) {
      int u = -1;
      double minT = std::numeric_limits<double>::max();
      for (int j = 0; j < n; ++j) {
        if (!visited[j] && time[j] < minT) {
          minT = time[j];
          u = j;
        }
      }

      if (u == -1 || u == end)
        break;
      visited[u] = true;

      for (const Edge &e : adj[u]) {
        if (bannedNodes.contains(e.to))
          continue;
        QPair<int, int> key1(u, e.to), key2(e.to, u);
        if (bannedEdges.contains(key1) || bannedEdges.contains(key2))
          continue;

        double effectiveTraffic = e.trafficFactor * peakMul;
        double t = travelTimeOf(e.distance, effectiveTraffic, v0);
        if (time[u] + t < time[e.to]) {
          time[e.to] = time[u] + t;
          prev[e.to] = u;
        }
      }
    }

    QVector<int> path;
    if (start == end) {
      path.append(start);
      return path;
    }
    if (time[end] >= std::numeric_limits<double>::max() / 2)
      return path;

    for (int at = end; at != -1; at = prev[at])
      path.prepend(at);
    return path;
  }

  QVector<int> dijkstraByDistance(int start, int end, const QSet<int> &bannedNodes,
                                  const QSet<QPair<int, int>> &bannedEdges) const {
    const int n = nodes.size();
    QVector<double> dist(n, std::numeric_limits<double>::max());
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    if (bannedNodes.contains(start) || bannedNodes.contains(end))
      return {};

    dist[start] = 0.0;

    for (int i = 0; i < n; ++i) {
      int u = -1;
      double minD = std::numeric_limits<double>::max();
      for (int j = 0; j < n; ++j) {
        if (!visited[j] && dist[j] < minD) {
          minD = dist[j];
          u = j;
        }
      }

      if (u == -1 || u == end)
        break;
      visited[u] = true;

      for (const Edge &e : adj[u]) {
        if (bannedNodes.contains(e.to))
          continue;
        QPair<int, int> key1(u, e.to), key2(e.to, u);
        if (bannedEdges.contains(key1) || bannedEdges.contains(key2))
          continue;

        if (dist[u] + e.distance < dist[e.to]) {
          dist[e.to] = dist[u] + e.distance;
          prev[e.to] = u;
        }
      }
    }

    QVector<int> path;
    if (start == end) {
      path.append(start);
      return path;
    }
    if (dist[end] >= std::numeric_limits<double>::max() / 2)
      return path;

    for (int at = end; at != -1; at = prev[at])
      path.prepend(at);
    return path;
  }
};

// ==================== CÁC WIDGET GIAO DIỆN GIAO THÔNG GOOGLE MAPS ====================
static inline const QStringList kDayNames = {"Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"};
static inline const QStringList kDayShortNames = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};

class TrafficSwitch : public QAbstractButton {
  Q_OBJECT
public:
  explicit TrafficSwitch(QWidget *parent = nullptr) : QAbstractButton(parent) {
    setCheckable(true);
    setChecked(true);
    setFixedSize(38, 20);
    setCursor(Qt::PointingHandCursor);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    bool on = isChecked();
    // Google Maps teal toggle: #00796B
    QColor trackColor = on ? QColor(0, 121, 107) : QColor(209, 213, 219);
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(rect(), 10, 10);

    // Knob: white circle with subtle shadow
    int knobD = 16;
    int knobY = 2;
    int knobX = on ? (width() - knobD - 2) : 2;

    p.setPen(QColor(0, 0, 0, 30));
    p.setBrush(Qt::white);
    p.drawEllipse(knobX, knobY, knobD, knobD);
  }
};

class TrafficTimeScaleWidget : public QWidget {
public:
  explicit TrafficTimeScaleWidget(QWidget *parent = nullptr) : QWidget(parent) {
    setFixedHeight(26);
  }
  void setDark(bool dark) { m_dark = dark; update(); }
protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f("Segoe UI", 8);
    p.setFont(f);
    p.setPen(m_dark ? QColor(160, 165, 175) : QColor(95, 99, 104));

    int margin = 7;
    int w = width() - 2 * margin;

    struct Mark { int minutes; QString label; };
    QVector<Mark> marks = {
      {8 * 60, "08:00"},
      {12 * 60, "12:00"},
      {16 * 60, "16:00"},
      {20 * 60, "20:00"}
    };

    for (const auto &m : marks) {
      double frac = static_cast<double>(m.minutes - 6 * 60) / (16.0 * 60.0);
      int x = margin + static_cast<int>(frac * w);
      p.drawLine(x, 2, x, 7);
      QRect textRect(x - 22, 9, 44, 15);
      p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, m.label);
    }
  }
private:
  bool m_dark = false;
};

// ==================== MAP VIEW (CANVAS & RENDER ĐÀ NẴNG THỰC TẾ) ====================
class MapView : public QGraphicsView {
  Q_OBJECT
public:
  enum RouteMode { TimeRoute = 0, DistRoute = 1 };
  enum TransportMode { Car = 0, Motorbike = 1, Walking = 2 };

  MapView(QWidget *parent = nullptr) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(bgColor);

    buildGraph();
    drawMap();
    resetView();

    pathAnimTimer = new QTimer(this);
    connect(pathAnimTimer, &QTimer::timeout, this, &MapView::onPathAnimTick);

    trafficSimTimer = new QTimer(this);
    connect(trafficSimTimer, &QTimer::timeout, this, &MapView::onTrafficSimTick);

    smoothAnimTimer = new QTimer(this);
    connect(smoothAnimTimer, &QTimer::timeout, this, &MapView::onSmoothAnimTick);

    setupFloatingControls();
    setupFloatingSearchBar();
    setupFloatingCategoryChips();
    setupFloatingRouteBanner();
    setupFloatingTrafficWidget();

    applyLiveTraffic();
    if (trafficSimTimer)
      trafficSimTimer->start(2500);
  }

  // Getters
  const QVector<RouteStep> &lastRoute() const { return lastSteps; }
  double lastTotalMeters() const { return totalMeters; }
  int lastTotalSeconds() const { return totalSeconds; }
  QVector<QVector<QPointF>> lastAlternatives() const { return altPaths; }

  const QVector<RouteStep> &timeRouteSteps() const { return timeSteps; }
  double timeRouteTotalMeters() const { return timeTotalMeters; }
  int timeRouteTotalSeconds() const { return timeTotalSeconds; }
  const QVector<RouteStep> &distRouteSteps() const { return distSteps; }
  double distRouteTotalMeters() const { return distTotalMeters; }
  int distRouteTotalSeconds() const { return distTotalSeconds; }
  bool hasDualRoutes() const { return dualRoutesAvailable; }
  bool areDualRoutesSame() const { return dualRoutesSame; }
  RouteMode currentRouteMode() const { return routeMode; }
  TransportMode currentTransportMode() const { return transportMode; }

  int nodeCount() const { return graph.nodes.size(); }
  int edgeCount() const {
    int c = 0;
    for (int u = 0; u < graph.nodes.size(); ++u)
      for (const Edge &e : graph.adj[u])
        if (u < e.to)
          ++c;
    return c;
  }
  QStringList nodeNames() const {
    QStringList l;
    for (const Node &n : graph.nodes)
      l << QString("%1 (%2)").arg(n.name, n.icon);
    return l;
  }
  int nodeIdByIndex(int idx) const {
    if (idx >= 0 && idx < graph.nodes.size())
      return graph.nodes[idx].id;
    return -1;
  }
  QString nodeNameById(int id) const {
    if (id >= 0 && id < graph.nodes.size())
      return graph.nodes[id].name;
    return "";
  }
  QPointF nodePos(int id) const {
    if (id < 0 || id >= graph.nodes.size())
      return {};
    return graph.nodes[id].pos;
  }
  int nodeDegree(int id) const {
    if (id < 0 || id >= graph.nodes.size())
      return 0;
    return graph.adj[id].size();
  }
  int startNodeId() const { return startEdgeU; }
  int endNodeId() const { return endEdgeU; }

  bool isTrafficLayerVisible() const { return showTrafficLayer; }
  bool isLandmarksLayerVisible() const { return showLandmarksLayer; }
  bool isStreetLabelsLayerVisible() const { return showStreetLabelsLayer; }
  bool isFollowVehicle() const { return followVehicle; }

  // Setters
  void setV0(double v) {
    v0 = v;
    recompute();
  }
  void setPeakMul(double m) {
    peakMul = m;
    recompute();
  }
  void setUseMeters(bool b) {
    useMeters = b;
    emit statusMessage(QString("Đơn vị: %1").arg(b ? "mét" : "km"));
    updateRouteBanner();
    viewport()->update();
  }
  void setDarkMode(bool b) {
    darkMode = b;
    applyTheme();
  }
  void setGridVisible(bool b) {
    gridVisible = b;
    if (gridGroup)
      gridGroup->setVisible(b);
  }
  void setBanLongBridge(bool b) {
    banLongBridge = b;
    // Cầu Rồng giữa cầu là Node 55
    if (b)
      bannedNodes.insert(55);
    else
      bannedNodes.remove(55);
    recompute();
  }
  void setAvoidCongested(bool b) {
    avoidCongested = b;
    recompute();
  }
  void setShowAlternatives(bool b) {
    showAlternatives = b;
    recompute();
  }
  void setRouteMode(int mode) {
    if (routeMode == static_cast<RouteMode>(mode))
      return;
    routeMode = static_cast<RouteMode>(mode);
    recompute();
    emit routeModeChanged(mode);
  }
  void setTransportMode(int mode) {
    transportMode = static_cast<TransportMode>(mode);
    if (transportMode == Car)
      v0 = 40.0;
    else if (transportMode == Motorbike)
      v0 = 35.0;
    else if (transportMode == Walking)
      v0 = 5.0;
    recompute();
  }

  void setTrafficLayerVisible(bool b) {
    showTrafficLayer = b;
    if (trafficLinesGroup)
      trafficLinesGroup->setVisible(b);
    if (legendGroup)
      legendGroup->setVisible(false);
    if (floatingTrafficSwitch && floatingTrafficSwitch->isChecked() != b) {
      floatingTrafficSwitch->blockSignals(true);
      floatingTrafficSwitch->setChecked(b);
      floatingTrafficSwitch->blockSignals(false);
    }
    if (btnTraffic && btnTraffic->isChecked() != b) {
      btnTraffic->blockSignals(true);
      btnTraffic->setChecked(b);
      btnTraffic->blockSignals(false);
    }
    if (!startPoint.isNull() && !endPoint.isNull()) {
      recompute();
    }
    viewport()->update();
    emit trafficLayerVisibilityChanged(b);
    emit statusMessage(b ? "🚦 Lớp giao thông Google Maps: BẬT"
                         : "⚪ Lớp giao thông Google Maps: TẮT");
  }

  void toggleTrafficLayer() {
    setTrafficLayerVisible(!showTrafficLayer);
  }

  void setLandmarksLayerVisible(bool b) {
    showLandmarksLayer = b;
    if (poiGroup)
      poiGroup->setVisible(b);
    viewport()->update();
    emit statusMessage(b ? "Đã bật lớp địa danh & POI" : "Đã ẩn lớp địa danh");
  }

  void setStreetLabelsLayerVisible(bool b) {
    showStreetLabelsLayer = b;
    if (streetLabelGroup)
      streetLabelGroup->setVisible(b);
    viewport()->update();
    emit statusMessage(b ? "Đã bật lớp tên đường" : "Đã ẩn lớp tên đường");
  }

  void setFollowVehicle(bool b) {
    followVehicle = b;
    emit statusMessage(b ? "Chế độ camera bám theo xe: BẬT" : "Chế độ camera bám theo xe: TẮT");
  }

  void setEndpoints(int u, int v) {
    if (u < 0 || u >= graph.nodes.size() || v < 0 || v >= graph.nodes.size() || u == v)
      return;
    clearPath();
    clearMarkers();
    stopAnimation();
    stopPathAnim();
    startPoint = graph.nodes[u].pos;
    startEdgeU = u;
    startEdgeV = -1;
    addMarker(startPoint, colorA, "A");
    endPoint = graph.nodes[v].pos;
    endEdgeU = v;
    endEdgeV = -1;
    addMarker(endPoint, colorB, "B");
    recompute();
    emit endpointsChanged(u, v);
    emit routeChanged();
  }

public slots:
  void setTrafficSimulation(bool enabled) {
    trafficSimEnabled = enabled;
    if (trafficSimEnabled) {
      if (trafficSimTimer)
        trafficSimTimer->start(1500);
      onTrafficSimTick();
    } else {
      if (!isTypicalMode) {
        if (trafficSimTimer)
          trafficSimTimer->start(2500);
        applyLiveTraffic();
      } else {
        if (trafficSimTimer)
          trafficSimTimer->stop();
      }
    }
  }

  void clearAll() {
    clearPath();
    clearMarkers();
    stopAnimation();
    stopPathAnim();
    startPoint = endPoint = QPointF();
    startEdgeU = startEdgeV = endEdgeU = endEdgeV = -1;
    lastSteps.clear();
    totalMeters = 0;
    totalSeconds = 0;
    altPaths.clear();
    if (floatingRouteBanner)
      floatingRouteBanner->hide();
    emit endpointsChanged(-1, -1);
    emit routeChanged();
    emit statusMessage("Đã xoá. Chọn điểm A → B để tìm đường.");
  }

  void swapAB() {
    std::swap(startPoint, endPoint);
    std::swap(startEdgeU, endEdgeU);
    std::swap(startEdgeV, endEdgeV);
    clearMarkers();
    redrawMarkers();
    emit endpointsChanged(startEdgeU, endEdgeU);
    if (!startPoint.isNull() && !endPoint.isNull())
      recompute();
  }

  void zoomIn() { scale(1.2, 1.2); viewport()->update(); }
  void zoomOut() { scale(0.83, 0.83); viewport()->update(); }
  void resetView() {
    resetTransform();
    fitInView(QRectF(-10, -25, 1160, 740), Qt::KeepAspectRatio);
    viewport()->update();
  }
  void fitView() {
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    viewport()->update();
  }

  void centerOnNode(int id) {
    if (id < 0 || id >= graph.nodes.size())
      return;
    centerOn(graph.nodes[id].pos);
    auto *flash = scene->addEllipse(
        graph.nodes[id].pos.x() - 20, graph.nodes[id].pos.y() - 20, 40, 40,
        QPen(QColor(66, 133, 244), 3.5), QBrush(QColor(66, 133, 244, 70)));
    flash->setZValue(45);
    QTimer::singleShot(1100, this, [this, flash]() {
      if (scene && flash->scene()) {
        scene->removeItem(flash);
        delete flash;
      }
    });
  }

  void playNavigationSimulation() {
    if (primaryPoints.size() >= 2) {
      startAnimation(primaryPoints);
    }
  }

  void savePng(const QString &fn) {
    QPixmap pix(size());
    pix.fill(darkMode ? QColor(24, 26, 32) : QColor(242, 239, 233));
    QPainter p(&pix);
    render(&p);
    p.end();
    pix.save(fn);
    emit statusMessage("Đã lưu ảnh: " + fn);
  }

signals:
  void routeChanged();
  void statusMessage(const QString &msg);
  void mousePosChanged(QPointF scenePos);
  void routeModeChanged(int mode);
  void endpointsChanged(int startId, int endId);
  void trafficSimTimeChanged(int hour, const QString &desc);
  void trafficLayerVisibilityChanged(bool visible);

protected:
  void resizeEvent(QResizeEvent *e) override {
    QGraphicsView::resizeEvent(e);
    if (floatingSearchBar) {
      floatingSearchBar->move(18, 16);
    }
    if (floatingCategoryBar) {
      floatingCategoryBar->move(18, 64);
    }
    if (floatingRouteBanner) {
      int bw = floatingRouteBanner->width();
      floatingRouteBanner->move(std::max(390, width() / 2 - bw / 2), 16);
    }
    if (floatingControlFrame) {
      floatingControlFrame->move(width() - floatingControlFrame->width() - 16,
                                 height() - floatingControlFrame->height() - 24);
    }
    if (floatingTrafficFrame) {
      repositionFloatingTraffic();
    }
  }

  void mouseMoveEvent(QMouseEvent *e) override {
    if ((e->pos() - mousePressPos).manhattanLength() > 6) {
      mouseMovedSignificantly = true;
    }

    currentMouseScenePos = mapToScene(e->pos());
    emit mousePosChanged(currentMouseScenePos);

    int oldHoverNode = hoveredNodeId;
    int oldHoverU = hoveredEdgeU;
    int oldHoverV = hoveredEdgeV;

    hoveredNodeId = findNearestNode(currentMouseScenePos, 20.0);
    if (hoveredNodeId < 0) {
      auto [u, v, proj] = findNearestPointOnEdge(currentMouseScenePos, 14.0);
      if (u != -1 && QLineF(currentMouseScenePos, proj).length() <= 14.0) {
        hoveredEdgeU = u;
        hoveredEdgeV = v;
      } else {
        hoveredEdgeU = hoveredEdgeV = -1;
      }
    } else {
      hoveredEdgeU = hoveredEdgeV = -1;
    }

    if (hoveredNodeId >= 0 || hoveredEdgeU >= 0) {
      setCursor(Qt::PointingHandCursor);
    } else {
      setCursor(Qt::OpenHandCursor);
    }

    if (hoveredNodeId != oldHoverNode || hoveredEdgeU != oldHoverU || hoveredEdgeV != oldHoverV) {
      viewport()->update();
    }

    QGraphicsView::mouseMoveEvent(e);
  }

  void leaveEvent(QEvent *e) override {
    if (hoveredNodeId >= 0 || hoveredEdgeU >= 0) {
      hoveredNodeId = -1;
      hoveredEdgeU = hoveredEdgeV = -1;
      viewport()->update();
    }
    QGraphicsView::leaveEvent(e);
  }

  void mousePressEvent(QMouseEvent *event) override {
    mousePressPos = event->pos();
    mouseMovedSignificantly = false;

    // Kiểm tra nhấp vào La Bàn (Compass) góc trên bên phải
    int vpW = viewport()->width();
    int cx = vpW - 40;
    int cy = 35;
    if (QLineF(event->pos(), QPointF(cx, cy)).length() <= 20.0) {
      resetView();
      emit statusMessage("Đã căn lại hướng Bắc & vị trí trung tâm");
      return;
    }

    if (event->button() == Qt::RightButton) {
      handleRightClick(event);
      return;
    }

    // LUÔN chuyển tiếp cho QGraphicsView để ScrollHandDrag (kéo map) hoạt động trơn tru!
    QGraphicsView::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    QGraphicsView::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton && !mouseMovedSignificantly) {
      // Click chuột thực sự (không phải kéo bản đồ)
      QPointF clickPos = mapToScene(event->pos());
      int nearestNode = findNearestNode(clickPos, 18.0);
      if (nearestNode >= 0) {
        selectedNodeId = nearestNode;
        selectedEdgeU = selectedEdgeV = -1;
        setEndpointFromNode(nearestNode);
        viewport()->update();
        return;
      }

      auto [edgeU, edgeV, projected] = findNearestPointOnEdge(clickPos, 12.0);
      if (edgeU != -1) {
        selectedEdgeU = edgeU;
        selectedEdgeV = edgeV;
        selectedNodeId = -1;
        setEndpointFromEdge(edgeU, edgeV, projected);
        viewport()->update();
        return;
      }
    }
  }

  void mouseDoubleClickEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      scale(1.25, 1.25);
      viewport()->update();
    }
  }

  void wheelEvent(QWheelEvent *event) override {
    double f = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    scale(f, f);
    viewport()->update();
  }

  void keyPressEvent(QKeyEvent *e) override {
    switch (e->key()) {
    case Qt::Key_Escape:
      clearAll();
      break;
    case Qt::Key_R:
      resetView();
      break;
    case Qt::Key_F:
      fitView();
      break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
      zoomIn();
      break;
    case Qt::Key_Minus:
      zoomOut();
      break;
    default:
      QGraphicsView::keyPressEvent(e);
    }
  }

  void drawForeground(QPainter *painter, const QRectF &) override {
    painter->save();
    painter->setWorldMatrixEnabled(false);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    int vpW = viewport()->width();
    int vpH = viewport()->height();

    // 1. THANH TỈ LỆ THƯỚC ĐO (GOOGLE MAPS SCALE BAR)
    double scaleFactor = transform().m11();
    double desiredMeters = 500.0;
    if (scaleFactor > 2.2) desiredMeters = 200.0;
    if (scaleFactor > 4.5) desiredMeters = 100.0;
    if (scaleFactor < 0.7) desiredMeters = 1000.0;

    double barPx = (desiredMeters / 5.0) * scaleFactor;
    int x0 = 20;
    int y0 = vpH - 24;

    QColor cardBg = darkMode ? QColor(32, 34, 42, 230) : QColor(255, 255, 255, 235);
    QColor barCol = darkMode ? QColor(210, 215, 225) : QColor(60, 64, 67);

    painter->setPen(Qt::NoPen);
    painter->setBrush(cardBg);
    painter->drawRoundedRect(x0 - 8, y0 - 18, barPx + 16, 26, 6, 6);

    painter->setPen(QPen(barCol, 2));
    painter->drawLine(QLineF(x0, y0, x0 + barPx, y0));
    painter->drawLine(QLineF(x0, y0 - 4, x0, y0 + 2));
    painter->drawLine(QLineF(x0 + barPx, y0 - 4, x0 + barPx, y0 + 2));

    QFont sf("Segoe UI", 9, QFont::DemiBold);
    painter->setFont(sf);
    painter->setPen(barCol);
    QString distText = (desiredMeters >= 1000.0)
                           ? QString("%1 km").arg(desiredMeters / 1000.0, 0, 'f', 1)
                           : QString("%1 m").arg(desiredMeters, 0, 'f', 0);
    painter->drawText(QRectF(x0, y0 - 18, barPx, 16), Qt::AlignCenter, distText);

    QFont wf("Segoe UI", 8);
    painter->setFont(wf);
    painter->setPen(darkMode ? QColor(130, 135, 150) : QColor(128, 134, 139));
    painter->drawText(x0 + barPx + 20, y0 + 2, "Bản đồ TP. Đà Nẵng • Google Maps Edition");

    // 2. LA BÀN TƯƠNG TÁC (INTERACTIVE COMPASS)
    int cx = vpW - 40;
    int cy = 35;
    painter->setPen(QPen(darkMode ? QColor(70, 75, 90) : QColor(218, 220, 224), 1.5));
    painter->setBrush(cardBg);
    painter->drawEllipse(QPointF(cx, cy), 18, 18);

    QPolygonF northArrow;
    northArrow << QPointF(cx, cy - 13) << QPointF(cx - 5, cy) << QPointF(cx + 5, cy);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(234, 67, 53));
    painter->drawPolygon(northArrow);

    QPolygonF southArrow;
    southArrow << QPointF(cx, cy + 13) << QPointF(cx - 5, cy) << QPointF(cx + 5, cy);
    painter->setBrush(darkMode ? QColor(150, 155, 165) : QColor(180, 185, 190));
    painter->drawPolygon(southArrow);

    QFont cf("Segoe UI", 8, QFont::Bold);
    painter->setFont(cf);
    painter->setPen(QColor(234, 67, 53));
    painter->drawText(QRectF(cx - 10, cy - 27, 20, 12), Qt::AlignCenter, "N");

    // 3. HOVER TOOLTIP CARD (GOOGLE MAPS INFO CARD KHI RÊ CHUỘT)
    if (hoveredNodeId >= 0 && hoveredNodeId < graph.nodes.size()) {
      const Node &n = graph.nodes[hoveredNodeId];
      QPoint viewPt = mapFromScene(n.pos);

      // Vẽ vòng sáng hào quang quanh điểm hover trong tọa độ viewport
      painter->setPen(QPen(QColor(66, 133, 244, 210), 2.5));
      painter->setBrush(QColor(66, 133, 244, 45));
      painter->drawEllipse(viewPt, 16, 16);

      // Tính toán vị trí thẻ tooltip
      int cardW = 270;
      int cardH = 88;
      int cardX = viewPt.x() + 20;
      int cardY = viewPt.y() - 44;

      if (cardX + cardW > vpW - 16)
        cardX = viewPt.x() - cardW - 20;
      if (cardY < 60)
        cardY = 60;
      if (cardY + cardH > vpH - 40)
        cardY = vpH - cardH - 40;

      // Nền thẻ Card + đổ bóng
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(0, 0, 0, darkMode ? 80 : 35));
      painter->drawRoundedRect(cardX + 2, cardY + 3, cardW, cardH, 10, 10);

      QColor cardBody = darkMode ? QColor(36, 38, 48, 245) : QColor(255, 255, 255, 248);
      QColor cardBdr  = darkMode ? QColor(70, 75, 95)       : QColor(218, 220, 224);
      painter->setPen(QPen(cardBdr, 1));
      painter->setBrush(cardBody);
      painter->drawRoundedRect(cardX, cardY, cardW, cardH, 10, 10);

      // Icon & Tên địa điểm
      QFont tf("Segoe UI", 10, QFont::Bold);
      painter->setFont(tf);
      painter->setPen(darkMode ? QColor(138, 180, 248) : QColor(26, 115, 232));
      painter->drawText(cardX + 12, cardY + 22, QString("%1  %2").arg(n.icon, n.name));

      // Phân loại địa điểm & số tuyến nối
      QFont subF("Segoe UI", 8);
      painter->setFont(subF);
      painter->setPen(darkMode ? QColor(180, 185, 195) : QColor(95, 99, 104));
      QString catStr = "Nút giao thông";
      if (n.category == "airport") catStr = "✈️ Cảng hàng không quốc tế";
      else if (n.category == "bridge") catStr = "🌉 Cầu vượt sông Hàn";
      else if (n.category == "park") catStr = "🌳 Công viên & Sinh thái";
      else if (n.category == "poi") catStr = "🛍️ Điểm đến nổi tiếng";
      else if (n.category == "station") catStr = "🚆 Ga đường sắt";
      else if (n.category == "lake") catStr = "💧 Hồ cảnh quan";
      else if (n.category == "beach") catStr = "🏖️ Bãi biển du lịch";

      painter->drawText(cardX + 12, cardY + 40, QString("%1 • %2 nhánh kết nối").arg(catStr).arg(graph.adj[n.id].size()));

      // Dòng hướng dẫn thao tác
      painter->setPen(darkMode ? QColor(150, 155, 165) : QColor(128, 134, 139));
      painter->drawText(cardX + 12, cardY + 58, "👉 Click chuột trái để chọn điểm đi / đến");

      // Trạng thái mật độ khu vực
      double avgT = 1.0;
      int edgeCnt = graph.adj[n.id].size();
      if (edgeCnt > 0) {
        double sum = 0;
        for (const auto &ed : graph.adj[n.id]) sum += ed.trafficFactor;
        avgT = sum / edgeCnt;
      }
      QColor chipC = trafficToColor(avgT);
      painter->setPen(Qt::NoPen);
      painter->setBrush(chipC);
      painter->drawEllipse(cardX + 14, cardY + 71, 7, 7);

      painter->setPen(chipC);
      QFont cf2("Segoe UI", 8, QFont::DemiBold);
      painter->setFont(cf2);
      QString tStatus = (avgT * peakMul <= 1.2) ? "Lưu thông thông thoáng" : ((avgT * peakMul <= 1.8) ? "Mật độ trung bình" : "Đang đông / ùn ứ");
      painter->drawText(cardX + 26, cardY + 78, tStatus);

    } else if (hoveredEdgeU >= 0 && hoveredEdgeV >= 0) {
      // Hover trên một đoạn đường
      double traf = trafficOf(graph, hoveredEdgeU, hoveredEdgeV);
      QString street = streetOf(graph, hoveredEdgeU, hoveredEdgeV);
      if (street.isEmpty())
        street = QString("%1 ↔ %2").arg(graph.nodes[hoveredEdgeU].name, graph.nodes[hoveredEdgeV].name);

      QPoint viewPt = mapFromScene(currentMouseScenePos);
      int cardW = 265;
      int cardH = 76;
      int cardX = viewPt.x() + 18;
      int cardY = viewPt.y() - 38;

      if (cardX + cardW > vpW - 16)
        cardX = viewPt.x() - cardW - 18;
      if (cardY < 60)
        cardY = 60;
      if (cardY + cardH > vpH - 40)
        cardY = vpH - cardH - 40;

      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(0, 0, 0, darkMode ? 80 : 35));
      painter->drawRoundedRect(cardX + 2, cardY + 3, cardW, cardH, 8, 8);

      QColor cardBody = darkMode ? QColor(36, 38, 48, 245) : QColor(255, 255, 255, 248);
      QColor cardBdr  = darkMode ? QColor(70, 75, 95)       : QColor(218, 220, 224);
      painter->setPen(QPen(cardBdr, 1));
      painter->setBrush(cardBody);
      painter->drawRoundedRect(cardX, cardY, cardW, cardH, 8, 8);

      QFont tf("Segoe UI", 9, QFont::Bold);
      painter->setFont(tf);
      painter->setPen(darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36));
      painter->drawText(cardX + 10, cardY + 20, "🛣️ " + street);

      double distPx = QLineF(graph.nodes[hoveredEdgeU].pos, graph.nodes[hoveredEdgeV].pos).length();
      double distMeters = distPx * 5.0;
      bool isBr = isBridgeOf(graph, hoveredEdgeU, hoveredEdgeV);
      QString typeStr = isBr ? "🌉 Cầu vượt sông" : "Trục đường chính";

      QFont subF("Segoe UI", 8);
      painter->setFont(subF);
      painter->setPen(darkMode ? QColor(170, 175, 185) : QColor(95, 99, 104));
      painter->drawText(cardX + 10, cardY + 36, QString("%1 • Dài %2 m").arg(typeStr).arg(distMeters, 0, 'f', 0));

      QColor c = trafficToColor(traf);
      painter->setPen(Qt::NoPen);
      painter->setBrush(c);
      painter->drawEllipse(cardX + 12, cardY + 47, 7, 7);

      painter->setPen(c);
      QFont sf2("Segoe UI", 8, QFont::DemiBold);
      painter->setFont(sf2);
      double speed = std::max(5.0, v0 / std::max(0.1, traf * peakMul));
      QString statusWord = (traf <= 1.25) ? "Thông thoáng" : ((traf <= 1.7) ? "Đông vừa" : ((traf <= 2.2) ? "Ùn ứ" : "Tắc đường"));
      QString tMsg = QString("Mật độ %1x (%2) • ~%3 km/h")
                         .arg(traf * peakMul, 0, 'f', 1)
                         .arg(statusWord)
                         .arg(speed, 0, 'f', 0);
      painter->drawText(cardX + 24, cardY + 54, tMsg);

      painter->setPen(darkMode ? QColor(140, 145, 155) : QColor(128, 134, 139));
      QFont hf("Segoe UI", 7);
      painter->setFont(hf);
      painter->drawText(cardX + 10, cardY + 68, "👉 Click chọn điểm • Chuột phải chỉnh mật độ");
    }

    painter->restore();
  }

private:
  QGraphicsScene *scene = nullptr;
  Graph graph;

  QPointF startPoint, endPoint;
  int startEdgeU = -1, startEdgeV = -1;
  int endEdgeU = -1, endEdgeV = -1;

  QVector<QGraphicsItem *> pathItems;
  QVector<QGraphicsItem *> markerItems;
  QVector<QGraphicsItem *> altPathItems;
  QVector<QGraphicsItem *> secondaryPathItems;
  QGraphicsItemGroup *gridGroup = nullptr;
  QGraphicsItemGroup *poiGroup = nullptr;
  QGraphicsItemGroup *streetLabelGroup = nullptr;
  QGraphicsItemGroup *legendGroup = nullptr;
  QToolButton *btnTraffic = nullptr;

  bool showTrafficLayer = true;
  bool showLandmarksLayer = true;
  bool showStreetLabelsLayer = true;
  bool followVehicle = false;

  int hoveredNodeId = -1;
  int hoveredEdgeU = -1, hoveredEdgeV = -1;
  int selectedNodeId = -1;
  int selectedEdgeU = -1, selectedEdgeV = -1;
  QPoint mousePressPos;
  bool mouseMovedSignificantly = false;
  QPointF currentMouseScenePos;

  QVector<RouteStep> lastSteps;
  QVector<QVector<QPointF>> altPaths;
  double totalMeters = 0;
  int totalSeconds = 0;

  RouteMode routeMode = TimeRoute;
  TransportMode transportMode = Car;
  bool dualRoutesAvailable = false;
  bool dualRoutesSame = false;
  QVector<RouteStep> timeSteps, distSteps;
  double timeTotalMeters = 0, distTotalMeters = 0;
  int timeTotalSeconds = 0, distTotalSeconds = 0;

  double v0 = 40.0;
  double peakMul = 1.0;
  bool useMeters = true;
  bool darkMode = false;
  bool gridVisible = false;
  bool banLongBridge = false;
  bool avoidCongested = false;
  bool showAlternatives = false;
  QSet<int> bannedNodes;
  QSet<QPair<int, int>> bannedEdges;

  QColor colorA = QColor(52, 168, 83);     // Marker A (Green)
  QColor colorB = QColor(234, 67, 53);     // Marker B (Red)
  QColor bgColor = QColor(242, 239, 233);  // Google Maps Land
  QColor roadColor = QColor(196, 190, 176); // Road outline casing
  QColor textColor = QColor(60, 64, 67);   // Label text

  QGraphicsItemGroup *mover = nullptr;

  // Smooth vehicle simulation
  QTimer *smoothAnimTimer = nullptr;
  struct AnimSeg {
    QPointF p0, p1;
    double len;
    double cumDist;
    double angleDeg;
  };
  QVector<AnimSeg> animSegments;
  double animTotalDist = 0.0;
  double animCurrentDist = 0.0;
  double animSpeed = 180.0; // px/sec

  QTimer *pathAnimTimer = nullptr;
  QVector<QGraphicsLineItem *> pathAnimLines;
  int pathAnimIndex = 0;
  QVector<QPointF> primaryPoints;

  QTimer *trafficSimTimer = nullptr;
  bool trafficSimEnabled = false;
  int simHour = 8; // giờ giả lập hiện tại (6-22)
  int simMinute = 0; // phút giả lập (0-59)

  // Floating Widgets
  QFrame *floatingControlFrame = nullptr;
  QFrame *floatingSearchBar = nullptr;
  QLineEdit *searchInput = nullptr;
  QToolButton *searchClearBtn = nullptr;
  QCompleter *searchCompleter = nullptr;
  QFrame *floatingCategoryBar = nullptr;
  QVector<QPushButton *> categoryChipButtons;

  QFrame *floatingRouteBanner = nullptr;
  QLabel *bannerModeIcon = nullptr;
  QLabel *bannerEtaLabel = nullptr;
  QLabel *bannerDistLabel = nullptr;
  QLabel *bannerDetailLabel = nullptr;
  QPushButton *bannerSimBtn = nullptr;

  // Floating Google Maps Traffic Widget
  QFrame *floatingTrafficFrame = nullptr;
  QToolButton *trafficModeBtn = nullptr;
  TrafficSwitch *floatingTrafficSwitch = nullptr;
  QWidget *typicalTrafficPanel = nullptr;
  QLabel *typicalTimeLabel = nullptr;
  QSlider *typicalTimeSlider = nullptr;
  TrafficTimeScaleWidget *typicalScaleWidget = nullptr;
  QVector<QPushButton *> dayButtons;
  int selectedDayIndex = 0; // 0 = T2 (Thứ Hai)
  int typicalMinuteOfDay = 9 * 60; // 09:00
  bool isTypicalMode = false;

  struct TrafficOverlayLine {
    QGraphicsLineItem *line = nullptr;
    int u = -1;
    int v = -1;
    bool isMajor = false;
  };
  QGraphicsItemGroup *trafficLinesGroup = nullptr;
  QVector<TrafficOverlayLine> trafficOverlayLines;

  void setupFloatingControls() {
    floatingControlFrame = new QFrame(this);
    floatingControlFrame->setObjectName("floatingControls");

    QVBoxLayout *flay = new QVBoxLayout(floatingControlFrame);
    flay->setContentsMargins(4, 4, 4, 4);
    flay->setSpacing(3);

    auto makeBtn = [this, flay](const QString &text, const QString &tip, auto slot) {
      QToolButton *btn = new QToolButton(floatingControlFrame);
      btn->setText(text);
      btn->setToolTip(tip);
      connect(btn, &QToolButton::clicked, this, slot);
      flay->addWidget(btn);
      return btn;
    };

    makeBtn("+", "Phóng to (Zoom In)", &MapView::zoomIn);
    makeBtn("−", "Thu nhỏ (Zoom Out)", &MapView::zoomOut);

    QFrame *sep = new QFrame(floatingControlFrame);
    sep->setFrameShape(QFrame::HLine);
    flay->addWidget(sep);

    makeBtn("⌖", "Về trung tâm Đà Nẵng", &MapView::resetView);
    makeBtn("⛶", "Vừa toàn cảnh (Fit)", &MapView::fitView);

    QFrame *sep2 = new QFrame(floatingControlFrame);
    sep2->setFrameShape(QFrame::HLine);
    flay->addWidget(sep2);

    QToolButton *btnFollow = new QToolButton(floatingControlFrame);
    btnFollow->setText("🎯");
    btnFollow->setToolTip("Bám theo xe khi dẫn đường (Follow Camera)");
    btnFollow->setCheckable(true);
    btnFollow->setChecked(followVehicle);
    connect(btnFollow, &QToolButton::toggled, this, &MapView::setFollowVehicle);
    flay->addWidget(btnFollow);

    btnTraffic = new QToolButton(floatingControlFrame);
    btnTraffic->setText("🚦");
    btnTraffic->setToolTip("Bật/Tắt màu sắc mật độ giao thông trên tuyến đường (A → B)");
    btnTraffic->setCheckable(true);
    btnTraffic->setChecked(showTrafficLayer);
    connect(btnTraffic, &QToolButton::toggled, this, &MapView::setTrafficLayerVisible);
    flay->addWidget(btnTraffic);

    connect(this, &MapView::trafficLayerVisibilityChanged, btnTraffic, [this](bool v) {
      if (btnTraffic && btnTraffic->isChecked() != v) {
        btnTraffic->blockSignals(true);
        btnTraffic->setChecked(v);
        btnTraffic->blockSignals(false);
      }
      if (floatingTrafficSwitch && floatingTrafficSwitch->isChecked() != v) {
        floatingTrafficSwitch->blockSignals(true);
        floatingTrafficSwitch->setChecked(v);
        floatingTrafficSwitch->blockSignals(false);
      }
    });

    floatingControlFrame->adjustSize();
    updateFloatingControlTheme();
  }

  void updateFloatingControlTheme() {
    if (!floatingControlFrame) return;
    if (darkMode) {
      floatingControlFrame->setStyleSheet(R"(
        QFrame#floatingControls {
          background: #282A36;
          border: 1px solid #44475A;
          border-radius: 8px;
        }
        QToolButton {
          background: transparent;
          border: none;
          border-radius: 4px;
          color: #F8F8F2;
          font-family: 'Segoe UI';
          font-size: 14px;
          font-weight: bold;
          min-width: 32px;
          min-height: 32px;
        }
        QToolButton:hover { background: #44475A; }
        QToolButton:pressed, QToolButton:checked { background: #6272A4; }
      )");
    } else {
      floatingControlFrame->setStyleSheet(R"(
        QFrame#floatingControls {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 8px;
        }
        QToolButton {
          background: transparent;
          border: none;
          border-radius: 4px;
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 14px;
          font-weight: bold;
          min-width: 32px;
          min-height: 32px;
        }
        QToolButton:hover { background: #F1F3F4; }
        QToolButton:pressed, QToolButton:checked { background: #E8F0FE; color: #1A73E8; }
      )");
    }
  }

  void setupFloatingSearchBar() {
    floatingSearchBar = new QFrame(this);
    floatingSearchBar->setObjectName("searchBarFrame");

    QHBoxLayout *lay = new QHBoxLayout(floatingSearchBar);
    lay->setContentsMargins(10, 4, 8, 4);
    lay->setSpacing(6);

    QLabel *icon = new QLabel("🔍", floatingSearchBar);
    icon->setStyleSheet("font-size: 14px; background: transparent;");
    lay->addWidget(icon);

    searchInput = new QLineEdit(floatingSearchBar);
    searchInput->setPlaceholderText("Tìm kiếm địa điểm, cầu, đường phố Đà Nẵng...");
    searchInput->setFrame(false);
    lay->addWidget(searchInput, 1);

    searchClearBtn = new QToolButton(floatingSearchBar);
    searchClearBtn->setText("✕");
    searchClearBtn->setToolTip("Xoá tìm kiếm");
    searchClearBtn->setVisible(false);
    connect(searchClearBtn, &QToolButton::clicked, this, [this]() {
      searchInput->clear();
      searchClearBtn->setVisible(false);
    });
    connect(searchInput, &QLineEdit::textChanged, this, [this](const QString &t) {
      searchClearBtn->setVisible(!t.isEmpty());
    });
    lay->addWidget(searchClearBtn);

    QToolButton *layersBtn = new QToolButton(floatingSearchBar);
    layersBtn->setText("🥞 Lớp");
    layersBtn->setToolTip("Tùy chọn lớp hiển thị bản đồ");
    QMenu *layersMenu = new QMenu(layersBtn);

    QAction *actTraffic = layersMenu->addAction("🚦 Mật độ giao thông (Tuyến đường)");
    actTraffic->setCheckable(true);
    actTraffic->setChecked(showTrafficLayer);
    connect(actTraffic, &QAction::toggled, this, &MapView::setTrafficLayerVisible);
    connect(this, &MapView::trafficLayerVisibilityChanged, actTraffic, [actTraffic](bool v) {
      actTraffic->setChecked(v);
    });

    QAction *actPOI = layersMenu->addAction("📍 Địa danh & POI");
    actPOI->setCheckable(true);
    actPOI->setChecked(showLandmarksLayer);
    connect(actPOI, &QAction::toggled, this, &MapView::setLandmarksLayerVisible);

    QAction *actStreet = layersMenu->addAction("🏷️ Tên đường phố");
    actStreet->setCheckable(true);
    actStreet->setChecked(showStreetLabelsLayer);
    connect(actStreet, &QAction::toggled, this, &MapView::setStreetLabelsLayerVisible);

    layersMenu->addSeparator();
    QAction *actFollow = layersMenu->addAction("🚗 Bám theo xe khi dẫn đường");
    actFollow->setCheckable(true);
    actFollow->setChecked(followVehicle);
    connect(actFollow, &QAction::toggled, this, &MapView::setFollowVehicle);

    layersBtn->setMenu(layersMenu);
    layersBtn->setPopupMode(QToolButton::InstantPopup);
    lay->addWidget(layersBtn);

    QStringList searchPool;
    for (int i = 0; i < graph.nodes.size(); ++i) {
      searchPool << QString("%1 (%2)").arg(graph.nodes[i].name, graph.nodes[i].icon);
    }
    searchCompleter = new QCompleter(searchPool, searchInput);
    searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    searchCompleter->setFilterMode(Qt::MatchContains);
    searchInput->setCompleter(searchCompleter);

    auto onSelectMatch = [this](const QString &text) {
      for (int i = 0; i < graph.nodes.size(); ++i) {
        if (text.contains(graph.nodes[i].name) || graph.nodes[i].name.contains(text.trimmed())) {
          centerOnNode(i);
          emit statusMessage(QString("📍 Đã tìm thấy: %1").arg(graph.nodes[i].name));
          break;
        }
      }
    };
    connect(searchCompleter, QOverload<const QString &>::of(&QCompleter::activated), this, onSelectMatch);
    connect(searchInput, &QLineEdit::returnPressed, this, [this, onSelectMatch]() {
      onSelectMatch(searchInput->text());
    });

    floatingSearchBar->setFixedSize(360, 42);
    updateFloatingSearchBarTheme();
  }

  void updateFloatingSearchBarTheme() {
    if (!floatingSearchBar) return;
    if (darkMode) {
      floatingSearchBar->setStyleSheet(R"(
        QFrame#searchBarFrame {
          background: #2D3038;
          border: 1px solid #44475A;
          border-radius: 21px;
        }
        QLineEdit {
          background: transparent;
          color: #E8EAED;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QToolButton {
          background: transparent;
          border: none;
          color: #8AB4F8;
          font-weight: bold;
          font-size: 12px;
          padding: 4px 8px;
          border-radius: 12px;
        }
        QToolButton:hover { background: #3C4048; }
      )");
    } else {
      floatingSearchBar->setStyleSheet(R"(
        QFrame#searchBarFrame {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 21px;
        }
        QLineEdit {
          background: transparent;
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QToolButton {
          background: transparent;
          border: none;
          color: #1A73E8;
          font-weight: bold;
          font-size: 12px;
          padding: 4px 8px;
          border-radius: 12px;
        }
        QToolButton:hover { background: #F1F3F4; }
      )");
    }
  }

  void setupFloatingCategoryChips() {
    floatingCategoryBar = new QFrame(this);
    floatingCategoryBar->setObjectName("categoryBarFrame");

    QHBoxLayout *lay = new QHBoxLayout(floatingCategoryBar);
    lay->setContentsMargins(6, 2, 6, 2);
    lay->setSpacing(6);

    auto makeChip = [this, lay](const QString &text, const QString &statusText, int targetNodeId) {
      QPushButton *btn = new QPushButton(text, floatingCategoryBar);
      btn->setCursor(Qt::PointingHandCursor);
      btn->setObjectName("catChipBtn");
      connect(btn, &QPushButton::clicked, this, [this, targetNodeId, statusText]() {
        if (targetNodeId >= 0 && targetNodeId < graph.nodes.size()) {
          centerOnNode(targetNodeId);
        }
        emit statusMessage(statusText);
      });
      lay->addWidget(btn);
      categoryChipButtons.append(btn);
      return btn;
    };

    makeChip("🍴 Nhà hàng", "🍴 Nhà hàng: Mộc Quán Seafood, Bếp Cuốn Michelin, Pizza 4P's...", 158);
    makeChip("🏨 Khách sạn", "🏨 Khách sạn: Mường Thanh Luxury, Four Points by Sheraton...", 162);
    makeChip("📷 Tham quan", "📷 Điểm tham quan: Tượng Cá Chép Hóa Rồng, Cầu Tình Yêu, Cầu Rồng...", 156);
    makeChip("🏖️ Bãi biển", "🏖️ Bãi biển du lịch: Bãi Biển Mỹ Khê, Bãi tắm Phạm Văn Đồng...", 75);
    makeChip("☕ Quán Café", "☕ Quán Café: Cửa Ngõ Café, B&B Bakery...", 159);
    makeChip("🛍️ Mua sắm", "🛍️ Trung tâm mua sắm: Vincom Plaza Sơn Trà, Chợ Hàn, Chợ Cồn, Chợ Đêm...", 58);

    floatingCategoryBar->adjustSize();
    floatingCategoryBar->resize(floatingCategoryBar->sizeHint().width() + 14, 32);
    updateFloatingCategoryBarTheme();
  }

  void updateFloatingCategoryBarTheme() {
    if (!floatingCategoryBar) return;
    if (darkMode) {
      floatingCategoryBar->setStyleSheet(R"(
        QFrame#categoryBarFrame {
          background: rgba(36, 38, 48, 230);
          border: 1px solid #44475A;
          border-radius: 16px;
        }
        QPushButton#catChipBtn {
          background: #2D3038;
          border: 1px solid #44475A;
          border-radius: 12px;
          color: #E8EAED;
          font-family: 'Segoe UI';
          font-size: 11px;
          font-weight: 500;
          padding: 3px 10px;
        }
        QPushButton#catChipBtn:hover {
          background: #3C4048;
          border-color: #6272A4;
        }
        QPushButton#catChipBtn:pressed {
          background: #44475A;
          color: #8AB4F8;
        }
      )");
    } else {
      floatingCategoryBar->setStyleSheet(R"(
        QFrame#categoryBarFrame {
          background: rgba(255, 255, 255, 240);
          border: 1px solid #DADCE0;
          border-radius: 16px;
        }
        QPushButton#catChipBtn {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 12px;
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 11px;
          font-weight: 500;
          padding: 3px 10px;
        }
        QPushButton#catChipBtn:hover {
          background: #F1F3F4;
          border-color: #BDC1C6;
        }
        QPushButton#catChipBtn:pressed {
          background: #E8F0FE;
          color: #1A73E8;
          border-color: #1A73E8;
        }
      )");
    }
  }

  void setupFloatingRouteBanner() {
    floatingRouteBanner = new QFrame(this);
    floatingRouteBanner->setObjectName("routeBannerFrame");

    QHBoxLayout *lay = new QHBoxLayout(floatingRouteBanner);
    lay->setContentsMargins(14, 4, 10, 4);
    lay->setSpacing(8);

    bannerModeIcon = new QLabel("🚗", floatingRouteBanner);
    bannerModeIcon->setStyleSheet("font-size: 18px; background: transparent;");
    lay->addWidget(bannerModeIcon);

    bannerEtaLabel = new QLabel("-- phút", floatingRouteBanner);
    bannerEtaLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #188038; background: transparent;");
    lay->addWidget(bannerEtaLabel);

    bannerDistLabel = new QLabel("(-- km)", floatingRouteBanner);
    bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #5F6368; background: transparent;");
    lay->addWidget(bannerDistLabel);

    QFrame *vsep = new QFrame(floatingRouteBanner);
    vsep->setFrameShape(QFrame::VLine);
    vsep->setStyleSheet("border-color: #DADCE0;");
    lay->addWidget(vsep);

    bannerDetailLabel = new QLabel("Tuyến tối ưu nhất", floatingRouteBanner);
    bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1A73E8; background: #E8F0FE; padding: 2px 8px; border-radius: 10px;");
    lay->addWidget(bannerDetailLabel);

    bannerSimBtn = new QPushButton("▶ Mô phỏng", floatingRouteBanner);
    connect(bannerSimBtn, &QPushButton::clicked, this, &MapView::playNavigationSimulation);
    lay->addWidget(bannerSimBtn);

    QToolButton *closeBtn = new QToolButton(floatingRouteBanner);
    closeBtn->setText("✕");
    closeBtn->setStyleSheet("background: transparent; border: none; font-size: 12px; color: #5F6368;");
    connect(closeBtn, &QToolButton::clicked, floatingRouteBanner, &QFrame::hide);
    lay->addWidget(closeBtn);

    floatingRouteBanner->setFixedHeight(42);
    floatingRouteBanner->hide();
    updateFloatingRouteBannerTheme();
  }

  void updateFloatingRouteBannerTheme() {
    if (!floatingRouteBanner) return;
    if (darkMode) {
      floatingRouteBanner->setStyleSheet(R"(
        QFrame#routeBannerFrame {
          background: #2D3038;
          border: 1px solid #44475A;
          border-radius: 21px;
        }
        QPushButton {
          background: #8AB4F8; color: #202124; border: none; border-radius: 12px;
          padding: 4px 10px; font-weight: bold; font-size: 11px;
        }
        QPushButton:hover { background: #AECBFA; }
      )");
      if (bannerDistLabel)
        bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #BDC1C6; background: transparent;");
      if (bannerDetailLabel)
        bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #8AB4F8; background: #384252; padding: 2px 8px; border-radius: 10px;");
    } else {
      floatingRouteBanner->setStyleSheet(R"(
        QFrame#routeBannerFrame {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 21px;
        }
        QPushButton {
          background: #1A73E8; color: white; border: none; border-radius: 12px;
          padding: 4px 10px; font-weight: bold; font-size: 11px;
        }
        QPushButton:hover { background: #1557B0; }
      )");
      if (bannerDistLabel)
        bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #5F6368; background: transparent;");
      if (bannerDetailLabel)
        bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1A73E8; background: #E8F0FE; padding: 2px 8px; border-radius: 10px;");
    }
  }

  void updateRouteBanner() {
    if (!floatingRouteBanner) return;
    if (lastSteps.isEmpty() || totalMeters <= 0) {
      floatingRouteBanner->hide();
      return;
    }
    QString modeIcon = "🚗";
    if (transportMode == Motorbike) modeIcon = "🛵";
    else if (transportMode == Walking) modeIcon = "🚶‍♂️";
    bannerModeIcon->setText(modeIcon);

    int m = totalSeconds / 60;
    int s = totalSeconds % 60;
    QString etaText = (m > 0) ? QString("%1 ph %2 s").arg(m).arg(s) : QString("%1 giây").arg(s);
    bannerEtaLabel->setText(etaText);
    bannerDistLabel->setText(QString("(%1)").arg(fmtDist(totalMeters)));

    QString detail = (routeMode == DistRoute) ? "Quãng đường ngắn nhất" : "Nhanh nhất tránh kẹt";
    bannerDetailLabel->setText(detail);

    floatingRouteBanner->adjustSize();
    int bw = floatingRouteBanner->width();
    floatingRouteBanner->move(std::max(390, width() / 2 - bw / 2), 16);
    floatingRouteBanner->show();
  }

  void setupFloatingTrafficWidget() {
    floatingTrafficFrame = new QFrame(this);
    floatingTrafficFrame->setObjectName("floatingTrafficFrame");

    QVBoxLayout *mainLay = new QVBoxLayout(floatingTrafficFrame);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    // 1. Top Bar Widget
    QWidget *topBar = new QWidget(floatingTrafficFrame);
    QHBoxLayout *topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(12, 6, 12, 6);
    topLay->setSpacing(10);

    // Dropdown Mode Button
    trafficModeBtn = new QToolButton(topBar);
    trafficModeBtn->setText("Giao thông theo thời gian thực ▾");
    trafficModeBtn->setCursor(Qt::PointingHandCursor);
    trafficModeBtn->setObjectName("trafficModeBtn");

    QMenu *modeMenu = new QMenu(trafficModeBtn);
    QAction *actLive = modeMenu->addAction("Giao thông trực tiếp");
    QAction *actTypical = modeMenu->addAction("Tình trạng giao thông điển hình");

    connect(actLive, &QAction::triggered, this, [this]() {
      isTypicalMode = false;
      trafficModeBtn->setText("Giao thông theo thời gian thực ▾");
      typicalTrafficPanel->hide();
      floatingTrafficFrame->adjustSize();
      repositionFloatingTraffic();
      applyLiveTraffic();
      if (trafficSimTimer) trafficSimTimer->start(2500);
      emit statusMessage("Đã chuyển sang: Giao thông theo thời gian thực (Trực tiếp)");
    });

    connect(actTypical, &QAction::triggered, this, [this]() {
      isTypicalMode = true;
      trafficModeBtn->setText("Tình trạng giao thông điển hình ▾");
      typicalTrafficPanel->show();
      floatingTrafficFrame->adjustSize();
      repositionFloatingTraffic();
      if (trafficSimTimer) trafficSimTimer->stop();
      int val = typicalTimeSlider ? typicalTimeSlider->value() : (9 * 60);
      int h = val / 60;
      int m = (val % 60) / 15 * 15;
      applyTypicalTraffic(selectedDayIndex, h, m);
    });

    trafficModeBtn->setMenu(modeMenu);
    trafficModeBtn->setPopupMode(QToolButton::InstantPopup);
    topLay->addWidget(trafficModeBtn);

    // Divider 1
    QFrame *div1 = new QFrame(topBar);
    div1->setFrameShape(QFrame::VLine);
    div1->setObjectName("trafficDiv");
    div1->setFixedHeight(16);
    topLay->addWidget(div1);

    // Legend Widget
    QWidget *legendW = new QWidget(topBar);
    QHBoxLayout *legLay = new QHBoxLayout(legendW);
    legLay->setContentsMargins(0, 0, 0, 0);
    legLay->setSpacing(5);

    QLabel *lblFast = new QLabel("Nhanh", legendW);
    lblFast->setObjectName("trafficLegendText");
    legLay->addWidget(lblFast);

    auto makeBar = [legendW, legLay](const QColor &c) {
      QFrame *b = new QFrame(legendW);
      b->setFixedSize(16, 7);
      b->setStyleSheet(QString("background: %1; border-radius: 1.5px;").arg(c.name()));
      legLay->addWidget(b);
    };

    makeBar(QColor(0, 214, 143));   // #00D68F (Nhanh - Xanh lá)
    makeBar(QColor(255, 209, 59));  // #FFD13B (Vàng)
    makeBar(QColor(255, 77, 77));   // #FF4D4D (Cam đỏ)
    makeBar(QColor(158, 42, 43));   // #9E2A2B (Đỏ sẫm - Chậm)

    QLabel *lblSlow = new QLabel("Chậm", legendW);
    lblSlow->setObjectName("trafficLegendText");
    legLay->addWidget(lblSlow);

    topLay->addWidget(legendW);

    // Divider 2
    QFrame *div2 = new QFrame(topBar);
    div2->setFrameShape(QFrame::VLine);
    div2->setObjectName("trafficDiv");
    div2->setFixedHeight(16);
    topLay->addWidget(div2);

    // Toggle switch
    floatingTrafficSwitch = new TrafficSwitch(topBar);
    floatingTrafficSwitch->setChecked(showTrafficLayer);
    connect(floatingTrafficSwitch, &TrafficSwitch::toggled, this, &MapView::setTrafficLayerVisible);
    topLay->addWidget(floatingTrafficSwitch);

    mainLay->addWidget(topBar);

    // 2. Typical Traffic Bottom Panel
    typicalTrafficPanel = new QWidget(floatingTrafficFrame);
    QVBoxLayout *botLay = new QVBoxLayout(typicalTrafficPanel);
    botLay->setContentsMargins(12, 0, 12, 10);
    botLay->setSpacing(8);

    QFrame *hDiv = new QFrame(typicalTrafficPanel);
    hDiv->setFrameShape(QFrame::HLine);
    hDiv->setObjectName("trafficHDiv");
    botLay->addWidget(hDiv);

    QHBoxLayout *contentLay = new QHBoxLayout;
    contentLay->setContentsMargins(0, 0, 0, 0);
    contentLay->setSpacing(14);

    // Left Column: Day buttons & time label
    QVBoxLayout *leftCol = new QVBoxLayout;
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(5);

    QHBoxLayout *dayLay = new QHBoxLayout;
    dayLay->setContentsMargins(0, 0, 0, 0);
    dayLay->setSpacing(3);

    for (int i = 0; i < 7; ++i) {
      QPushButton *btn = new QPushButton(kDayShortNames[i], typicalTrafficPanel);
      btn->setCursor(Qt::PointingHandCursor);
      btn->setFixedSize(24, 24);
      connect(btn, &QPushButton::clicked, this, [this, i]() {
        selectedDayIndex = i;
        updateDayButtonsUI();
        int val = typicalTimeSlider->value();
        int h = val / 60;
        int m = (val % 60) / 15 * 15;
        typicalTimeLabel->setText(QString("%1,%2:%3").arg(kDayNames[selectedDayIndex]).arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')));
        applyTypicalTraffic(selectedDayIndex, h, m);
      });
      dayLay->addWidget(btn);
      dayButtons.append(btn);
    }
    leftCol->addLayout(dayLay);

    typicalTimeLabel = new QLabel("Thứ Hai,09:00", typicalTrafficPanel);
    typicalTimeLabel->setObjectName("typicalTimeLabel");
    leftCol->addWidget(typicalTimeLabel);

    contentLay->addLayout(leftCol);

    // Right Column: Slider & Scale
    QVBoxLayout *rightCol = new QVBoxLayout;
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(0);

    typicalTimeSlider = new QSlider(Qt::Horizontal, typicalTrafficPanel);
    typicalTimeSlider->setRange(6 * 60, 22 * 60);
    typicalTimeSlider->setSingleStep(15);
    typicalTimeSlider->setPageStep(60);
    typicalTimeSlider->setValue(9 * 60);
    typicalTimeSlider->setCursor(Qt::PointingHandCursor);
    typicalTimeSlider->setFixedWidth(190);

    typicalScaleWidget = new TrafficTimeScaleWidget(typicalTrafficPanel);
    typicalScaleWidget->setFixedWidth(190);

    connect(typicalTimeSlider, &QSlider::valueChanged, this, [this](int val) {
      typicalMinuteOfDay = val;
      int h = val / 60;
      int m = (val % 60) / 15 * 15;
      typicalTimeLabel->setText(QString("%1,%2:%3").arg(kDayNames[selectedDayIndex]).arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')));
      applyTypicalTraffic(selectedDayIndex, h, m);
    });

    rightCol->addWidget(typicalTimeSlider);
    rightCol->addWidget(typicalScaleWidget);

    contentLay->addLayout(rightCol);
    botLay->addLayout(contentLay);

    mainLay->addWidget(typicalTrafficPanel);
    typicalTrafficPanel->hide();

    updateDayButtonsUI();
    updateFloatingTrafficTheme();
    floatingTrafficFrame->adjustSize();
  }

  void updateDayButtonsUI() {
    for (int i = 0; i < dayButtons.size(); ++i) {
      if (i == selectedDayIndex) {
        dayButtons[i]->setStyleSheet(
            "QPushButton { background: #00796B; color: #FFFFFF; font-weight: bold; "
            "font-size: 11px; border: none; border-radius: 12px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; }");
      } else {
        dayButtons[i]->setStyleSheet(
            darkMode ? "QPushButton { background: transparent; color: #BDC1C6; font-size: 11px; "
                       "border: none; border-radius: 12px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; } "
                       "QPushButton:hover { background: #3C4043; }"
                     : "QPushButton { background: transparent; color: #3C4043; font-size: 11px; "
                       "border: none; border-radius: 12px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; } "
                       "QPushButton:hover { background: #F1F3F4; }");
      }
    }
  }

  void repositionFloatingTraffic() {
    if (!floatingTrafficFrame) return;
    int tw = floatingTrafficFrame->width();
    int th = floatingTrafficFrame->height();
    int tx = std::max(20, (width() - tw) / 2);
    int ty = height() - th - 20;
    floatingTrafficFrame->move(tx, ty);
  }

  void updateFloatingTrafficTheme() {
    if (!floatingTrafficFrame) return;

    if (typicalScaleWidget) {
      typicalScaleWidget->setDark(darkMode);
    }

    if (darkMode) {
      floatingTrafficFrame->setStyleSheet(R"(
        QFrame#floatingTrafficFrame {
          background: #202124;
          border: 1px solid #3C4043;
          border-radius: 8px;
        }
        QToolButton#trafficModeBtn {
          background: transparent;
          border: none;
          color: #E8EAED;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 4px 6px;
          border-radius: 4px;
        }
        QToolButton#trafficModeBtn:hover {
          background: #303134;
        }
        QFrame#trafficDiv, QFrame#trafficHDiv {
          border: none;
          background: #3C4043;
        }
        QLabel#trafficLegendText {
          color: #9AA0A6;
          font-family: 'Segoe UI';
          font-size: 11px;
          font-style: italic;
        }
        QLabel#typicalTimeLabel {
          color: #E8EAED;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: bold;
        }
        QSlider::groove:horizontal {
          height: 3px;
          background: #3C4043;
          border-radius: 1px;
        }
        QSlider::sub-page:horizontal {
          background: #00897B;
          border-radius: 1px;
        }
        QSlider::handle:horizontal {
          background: #00897B;
          border: 2px solid #202124;
          width: 14px;
          height: 14px;
          margin: -6px 0;
          border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
          background: #26A69A;
        }
      )");
    } else {
      floatingTrafficFrame->setStyleSheet(R"(
        QFrame#floatingTrafficFrame {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 8px;
        }
        QToolButton#trafficModeBtn {
          background: transparent;
          border: none;
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 4px 6px;
          border-radius: 4px;
        }
        QToolButton#trafficModeBtn:hover {
          background: #F1F3F4;
        }
        QFrame#trafficDiv, QFrame#trafficHDiv {
          border: none;
          background: #E0E0E0;
        }
        QLabel#trafficLegendText {
          color: #5F6368;
          font-family: 'Segoe UI';
          font-size: 11px;
          font-style: italic;
        }
        QLabel#typicalTimeLabel {
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: bold;
        }
        QSlider::groove:horizontal {
          height: 3px;
          background: #DADCE0;
          border-radius: 1px;
        }
        QSlider::sub-page:horizontal {
          background: #00796B;
          border-radius: 1px;
        }
        QSlider::handle:horizontal {
          background: #00796B;
          border: 2px solid #FFFFFF;
          width: 14px;
          height: 14px;
          margin: -6px 0;
          border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
          background: #004D40;
        }
      )");
    }
    updateDayButtonsUI();
  }

  void updateCityTrafficOverlay() {
    for (const auto &ref : trafficOverlayLines) {
      if (ref.line) {
        double traf = trafficOf(graph, ref.u, ref.v);
        QColor c = trafficToColor(traf);
        int lineW = ref.isMajor ? 4 : 3;
        ref.line->setPen(QPen(c, lineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      }
    }
  }

  void applyTypicalTraffic(int dayOfWeek, int hour, int minute) {
    double timeInHours = hour + minute / 60.0;
    bool isWeekend = (dayOfWeek >= 5);

    double rushBridge = 1.0;
    double rushMajor = 1.0;
    double rushSecondary = 1.0;
    double rushBeach = 1.0;
    double rushQuiet = 1.0;
    QString desc;

    if (!isWeekend) {
      // ===== NGÀY TRONG TUẦN (T2 - T6) =====
      if (timeInHours >= 6.5 && timeInHours < 7.25) {
        rushBridge = 1.45; rushMajor = 1.35; rushSecondary = 1.20; rushBeach = 1.05; rushQuiet = 0.95;
        desc = "Bắt đầu giờ cao điểm sáng";
      } else if (timeInHours >= 7.25 && timeInHours <= 8.5) {
        double peak = 1.0 - std::abs(timeInHours - 7.75) / 0.75;
        rushBridge = 2.05 + 0.35 * peak;      // 2.05 - 2.40 (Đỏ sẫm / Đỏ)
        rushMajor = 1.75 + 0.30 * peak;       // 1.75 - 2.05 (Cam / Đỏ)
        rushSecondary = 1.35 + 0.20 * peak;   // 1.35 - 1.55 (Vàng / Cam)
        rushBeach = 1.10;                     // 1.10 (Xanh)
        rushQuiet = 1.00;                     // 1.00 (Xanh)
        desc = "Cao điểm sáng (Ùn tắc khu vực trung tâm & cầu)";
      } else if (timeInHours > 8.5 && timeInHours <= 9.5) {
        rushBridge = 1.50; rushMajor = 1.40; rushSecondary = 1.25; rushBeach = 1.05; rushQuiet = 0.98;
        desc = "Tan dần cao điểm sáng";
      } else if (timeInHours > 9.5 && timeInHours < 11.25) {
        rushBridge = 1.25; rushMajor = 1.20; rushSecondary = 1.10; rushBeach = 1.00; rushQuiet = 0.95;
        desc = "Lưu thông thông thoáng";
      } else if (timeInHours >= 11.25 && timeInHours <= 12.75) {
        double peak = 1.0 - std::abs(timeInHours - 12.0) / 0.75;
        rushBridge = 1.65 + 0.20 * peak;      // 1.65 - 1.85 (Cam)
        rushMajor = 1.55 + 0.25 * peak;       // 1.55 - 1.80 (Vàng đậm / Cam)
        rushSecondary = 1.35 + 0.15 * peak;   // 1.35 - 1.50 (Vàng)
        rushBeach = 1.05;                     // 1.05 (Xanh)
        rushQuiet = 0.95;                     // 0.95 (Xanh)
        desc = "Tan tầm trưa (Đông khu vực văn phòng & trường học)";
      } else if (timeInHours > 12.75 && timeInHours < 16.5) {
        rushBridge = 1.25; rushMajor = 1.20; rushSecondary = 1.10; rushBeach = 1.05; rushQuiet = 0.95;
        desc = "Lưu thông bình thường";
      } else if (timeInHours >= 16.5 && timeInHours <= 18.5) {
        double peak = 1.0 - std::abs(timeInHours - 17.5) / 1.0;
        rushBridge = 2.15 + 0.35 * peak;      // 2.15 - 2.50 (Đỏ sẫm / Đỏ)
        rushMajor = 1.85 + 0.35 * peak;       // 1.85 - 2.20 (Đỏ)
        rushSecondary = 1.45 + 0.25 * peak;   // 1.45 - 1.70 (Vàng đậm / Cam)
        rushBeach = 1.30;                     // 1.30 (Vàng nhẹ)
        rushQuiet = 1.05;                     // 1.05 (Xanh)
        desc = "Cao điểm chiều (Ùn tắc tan tầm trên diện rộng)";
      } else if (timeInHours > 18.5 && timeInHours <= 20.0) {
        rushBridge = 1.55; rushMajor = 1.45; rushSecondary = 1.30; rushBeach = 1.20; rushQuiet = 0.98;
        desc = "Buổi tối (Đông nhẹ khu phố ẩm thực)";
      } else if (timeInHours > 20.0 && timeInHours <= 22.0) {
        rushBridge = 1.20; rushMajor = 1.15; rushSecondary = 1.05; rushBeach = 1.00; rushQuiet = 0.90;
        desc = "Đêm (Lưu thông rất thoáng)";
      } else {
        rushBridge = 0.90; rushMajor = 0.90; rushSecondary = 0.85; rushBeach = 0.85; rushQuiet = 0.80;
        desc = "Đêm muộn (Đường vắng)";
      }
    } else {
      // ===== CUỐI TUẦN (THỨ BẢY & CHỦ NHẬT) =====
      if (timeInHours >= 7.0 && timeInHours < 9.0) {
        rushBridge = 1.15; rushMajor = 1.10; rushSecondary = 1.05; rushBeach = 1.25; rushQuiet = 0.90;
        desc = "Cuối tuần - Sáng sớm tắm biển & cà phê";
      } else if (timeInHours >= 9.0 && timeInHours <= 11.5) {
        rushBridge = 1.35; rushMajor = 1.30; rushSecondary = 1.25; rushBeach = 1.20; rushQuiet = 0.95;
        desc = "Cuối tuần - Mua sắm & dạo phố";
      } else if (timeInHours > 11.5 && timeInHours < 15.5) {
        rushBridge = 1.20; rushMajor = 1.15; rushSecondary = 1.10; rushBeach = 1.10; rushQuiet = 0.90;
        desc = "Cuối tuần - Trưa bình thường";
      } else if (timeInHours >= 15.5 && timeInHours <= 18.5) {
        rushBridge = 1.70; rushMajor = 1.50; rushSecondary = 1.35; rushBeach = 2.10; rushQuiet = 1.00;
        desc = "Cuối tuần - Chiều đông đúc bãi biển Mỹ Khê";
      } else if (timeInHours > 18.5 && timeInHours <= 21.5) {
        rushBridge = 2.30; rushMajor = 1.80; rushSecondary = 1.50; rushBeach = 1.95; rushQuiet = 1.00;
        desc = "Cuối tuần - Tối cao điểm dạo phố & Cầu Rồng phun lửa";
      } else {
        rushBridge = 1.20; rushMajor = 1.10; rushSecondary = 1.00; rushBeach = 1.15; rushQuiet = 0.85;
        desc = "Cuối tuần - Đêm muộn";
      }
    }

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (int i = 0; i < graph.adj[u].size(); ++i) {
        int v = graph.adj[u][i].to;
        if (u < v) {
          Edge &e = graph.adj[u][i];
          const QString &st = e.streetName;

          double categoryFactor = rushSecondary;

          bool isBeachRoad = (st.contains("Võ Nguyên Giáp") || st.contains("Phạm Văn Đồng") ||
                              st.contains("Hoàng Sa") || st.contains("Trường Sa") ||
                              st.contains("Mỹ Khê") || st.contains("Võ Văn Kiệt") ||
                              st.contains("Hà Bổng") || st.contains("Hồ Nghinh") ||
                              st.contains("Nguyễn Văn Thoại"));

          bool isQuietRoad = (st.contains("Nguyễn Tất Thành") || st.contains("Như Nguyệt") ||
                              st.contains("Hà Huy Tập") || st.contains("Thái Thị Bôi") ||
                              st.contains("Lê Duy Đình") || st.contains("Hồ Công Viên") ||
                              st.contains("Đỗ Bá") || st.contains("Khu Đô Thị"));

          bool isHotspot = (st.contains("Cầu Sông Hàn") || st.contains("Cầu Rồng") ||
                            st.contains("Cầu Trần Thị Lý") || st.contains("Cầu Thuận Phước") ||
                            e.isBridge);

          if (isHotspot) {
            categoryFactor = rushBridge;
          } else if (isBeachRoad) {
            categoryFactor = rushBeach;
          } else if (isQuietRoad) {
            categoryFactor = rushQuiet;
          } else if (e.isMajor) {
            categoryFactor = rushMajor;
          }

          // Hash biến thiên ổn định cho từng đoạn đường (u, v) để màu sắc phân hóa tự nhiên
          unsigned int seed = (static_cast<unsigned int>(std::min(u, v)) * 2654435761u) ^
                              (static_cast<unsigned int>(std::max(u, v)) * 2246822519u);
          double variation = 0.85 + ((seed % 1000) / 1000.0) * 0.30;

          double finalFactor = e.baseTraffic * categoryFactor * variation;
          finalFactor = std::max(0.80, std::min(2.70, finalFactor));

          e.trafficFactor = finalFactor;
          for (int j = 0; j < graph.adj[v].size(); ++j) {
            if (graph.adj[v][j].to == u) {
              graph.adj[v][j].trafficFactor = finalFactor;
              break;
            }
          }
        }
      }
    }

    updateCityTrafficOverlay();
    if (!startPoint.isNull() && !endPoint.isNull()) {
      recompute();
    } else {
      viewport()->update();
    }

    QString timeStr = QString("%1:%2")
                          .arg(hour, 2, 10, QChar('0'))
                          .arg(minute, 2, 10, QChar('0'));
    emit trafficSimTimeChanged(hour, QString("%1 – %2").arg(timeStr, desc));
    emit statusMessage(QString("Mô phỏng GT điển hình: %1 %2 – %3")
                           .arg(kDayNames[dayOfWeek], timeStr, desc));
  }

  void applyLiveTraffic() {
    if (isTypicalMode) return;

    QTime now = QTime::currentTime();
    QDate today = QDate::currentDate();
    int dayOfWeek = std::clamp(today.dayOfWeek() - 1, 0, 6); // 0=Thứ Hai .. 6=Chủ Nhật
    int hour = now.hour();
    int minute = now.minute();

    applyTypicalTraffic(dayOfWeek, hour, minute);

    // Mô phỏng dao động giao thông vi mô sống động thời gian thực (Live Traffic Dynamics)
    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (int i = 0; i < graph.adj[u].size(); ++i) {
        int v = graph.adj[u][i].to;
        if (u < v && graph.adj[u][i].isMajor) {
          if ((std::rand() % 100) < 22) {
            double delta = ((std::rand() % 21) - 10) / 100.0; // dao động ±0.10
            double f = std::clamp(graph.adj[u][i].trafficFactor + delta, 0.8, 2.7);
            graph.adj[u][i].trafficFactor = f;
            for (auto &rev : graph.adj[v]) {
              if (rev.to == u) { rev.trafficFactor = f; break; }
            }
          }
        }
      }
    }
    updateCityTrafficOverlay();
    if (!startPoint.isNull() && !endPoint.isNull()) {
      recompute();
    } else {
      viewport()->update();
    }

    QString timeStr = QString("%1:%2")
                          .arg(hour, 2, 10, QChar('0'))
                          .arg(minute, 2, 10, QChar('0'));
    emit trafficSimTimeChanged(hour, QString("%1 – Trực tiếp").arg(timeStr));
    emit statusMessage(QString("Mô phỏng GT: 🟢 %1 – Trực tiếp thời gian thực").arg(timeStr));
  }

  void onSmoothAnimTick() {
    if (animSegments.isEmpty()) {
      smoothAnimTimer->stop();
      return;
    }

    animCurrentDist += animSpeed * 0.035;
    if (animCurrentDist >= animTotalDist) {
      animCurrentDist = 0.0; // Vòng lặp mô phỏng liên tục
    }

    // Tìm đoạn đường đang chạy
    const AnimSeg *curSeg = &animSegments.last();
    double segStartDist = 0.0;
    for (const auto &seg : animSegments) {
      if (animCurrentDist <= seg.cumDist) {
        curSeg = &seg;
        segStartDist = seg.cumDist - seg.len;
        break;
      }
    }

    double localT = 0.0;
    if (curSeg->len > 1e-4) {
      localT = std::clamp((animCurrentDist - segStartDist) / curSeg->len, 0.0, 1.0);
    }
    QPointF curPos = curSeg->p0 + localT * (curSeg->p1 - curSeg->p0);

    if (mover && mover->scene()) {
      mover->setPos(curPos);
      mover->setRotation(curSeg->angleDeg + 90.0);
      if (followVehicle) {
        centerOn(curPos);
      }
    }
  }

  QColor trafficToColor(double factor) const {
    double f = factor * peakMul;
    if (f <= 1.25)
      return QColor(15, 157, 88);    // Xanh lá Google Maps (thông thoáng) #0F9D58
    if (f <= 1.55)
      return QColor(251, 188, 4);    // Vàng Google Maps (đông nhẹ) #FBBC04
    if (f <= 1.9)
      return QColor(255, 112, 67);   // Cam (đông vừa) #FF7043
    if (f <= 2.3)
      return QColor(234, 67, 53);    // Đỏ Google Maps (ùn tắc) #EA4335
    return QColor(183, 28, 28);      // Đỏ sẫm (tắc nghẽn nghiêm trọng) #B71C1C
  }

  // ==================== KHỞI TẠO TOÀN BỘ MẠNG LƯỚI ĐƯỜNG ĐÀ NẴNG THỰC TẾ ====================
  void buildGraph() {
    // 1. KHU VỰC TÂY BẮC & THANH KHÊ
    graph.addNode(0, "Nguyễn Tất Thành - Cửa Ngõ Tây", QPointF(-20, 75), "junction", "🌊");
    graph.addNode(1, "Nguyễn Tất Thành - Trần Cao Vân Bắc", QPointF(260, 36), "junction", "🌊");
    graph.addNode(2, "Trần Cao Vân - Thái Thị Bôi", QPointF(120, 130), "junction", "📍");
    graph.addNode(3, "Điện Biên Phủ - Cửa Ngõ Tây (Ngã Ba Huế)", QPointF(-20, 270), "junction", "📍");
    graph.addNode(4, "Điện Biên Phủ - AEON Mall Thanh Khê", QPointF(180, 270), "poi", "🛍️");
    graph.addNode(5, "Cổng Cảng Hàng Không QT Đà Nẵng", QPointF(210, 440), "airport", "✈️");
    graph.addNode(6, "Đường Băng Sân Bay Đà Nẵng", QPointF(95, 520), "airport", "🛫");
    graph.addNode(7, "Điện Biên Phủ - Nguyễn Tri Phương", QPointF(290, 275), "junction", "📍");
    graph.addNode(8, "Hồ Công Viên 29/3 (Bắc)", QPointF(265, 310), "park", "🌳");
    graph.addNode(9, "Hồ Công Viên 29/3 (Nam)", QPointF(265, 380), "park", "🌳");
    graph.addNode(10, "Lê Duy Đình - Nguyễn Tri Phương", QPointF(255, 345), "junction", "📍");
    graph.addNode(11, "Nguyễn Tri Phương - Nguyễn Văn Linh", QPointF(290, 440), "junction", "📍");
    graph.addNode(12, "Nguyễn Hữu Thọ - Duy Tân", QPointF(340, 610), "junction", "📍");
    graph.addNode(13, "Sân Bay Phía Nam Duy Tân", QPointF(230, 610), "airport", "✈️");

    // 2. KHU VỰC TRUNG TÂM (HẢI CHÂU, THẠC GIÁN, VĨNH TRUNG)
    graph.addNode(14, "Lý Thái Tổ - Ga Đà Nẵng", QPointF(360, 275), "station", "🚆");
    graph.addNode(15, "Hải Phòng - Ông Ích Khiêm", QPointF(450, 175), "junction", "📍");
    graph.addNode(16, "Trần Cao Vân - Ông Ích Khiêm", QPointF(440, 125), "junction", "📍");
    graph.addNode(17, "Nguyễn Tất Thành - Ông Ích Khiêm", QPointF(440, 22), "junction", "🌊");
    graph.addNode(18, "Hồ Thạc Gián (Đ. Phan Thanh)", QPointF(360, 330), "lake", "💧");
    graph.addNode(19, "Hồ Hàm Nghi (Đ. Hàm Nghi)", QPointF(410, 330), "lake", "💧");
    graph.addNode(20, "Phan Thanh - Nguyễn Văn Linh", QPointF(360, 440), "junction", "📍");
    graph.addNode(21, "Hàm Nghi - Nguyễn Văn Linh", QPointF(410, 440), "junction", "📍");
    graph.addNode(22, "Lê Đình Lý - Nguyễn Văn Linh", QPointF(450, 440), "junction", "📍");
    graph.addNode(23, "Lê Đình Lý - Duy Tân", QPointF(460, 610), "junction", "📍");
    graph.addNode(24, "Chợ Cồn (Hùng Vương - Ông Ích Khiêm)", QPointF(450, 275), "poi", "🛍️");
    graph.addNode(25, "Ngã Tư Lê Duẩn - Ông Ích Khiêm", QPointF(450, 220), "junction", "📍");
    graph.addNode(26, "Ngã Tư Lê Duẩn - Hoàng Diệu", QPointF(490, 220), "junction", "📍");
    graph.addNode(27, "Hùng Vương - Ngô Gia Tự", QPointF(490, 275), "junction", "📍");
    graph.addNode(28, "Hoàng Diệu - Hoàng Văn Thụ", QPointF(490, 365), "junction", "📍");
    graph.addNode(29, "Hoàng Diệu - Nguyễn Văn Linh", QPointF(490, 440), "junction", "📍");
    graph.addNode(30, "Hoàng Diệu - Duy Tân", QPointF(510, 610), "junction", "📍");
    graph.addNode(31, "Triệu Nữ Vương - Nguyễn Văn Linh", QPointF(470, 440), "junction", "📍");
    graph.addNode(32, "Lê Lợi - Quang Trung", QPointF(530, 80), "junction", "📍");
    graph.addNode(33, "Lê Lợi - Hải Phòng", QPointF(530, 170), "junction", "📍");
    graph.addNode(34, "Ngã Tư Lê Duẩn - Lê Lợi", QPointF(530, 220), "junction", "📍");
    graph.addNode(35, "Phan Châu Trinh - Hùng Vương", QPointF(530, 275), "junction", "📍");
    graph.addNode(36, "Phan Châu Trinh - Hoàng Văn Thụ", QPointF(530, 365), "junction", "📍");
    graph.addNode(37, "Phan Châu Trinh - Nguyễn Văn Linh", QPointF(530, 440), "junction", "📍");
    graph.addNode(38, "Phan Châu Trinh - Trưng Nữ Vương", QPointF(540, 540), "junction", "📍");
    graph.addNode(39, "Trần Phú - Quang Trung", QPointF(610, 80), "junction", "📍");
    graph.addNode(40, "Trần Phú - Hải Phòng", QPointF(610, 165), "junction", "📍");
    graph.addNode(41, "Trần Phú - Lê Duẩn (Tây Cầu Sông Hàn)", QPointF(610, 220), "junction", "📍");
    graph.addNode(42, "Chợ Hàn (Trần Phú - Hùng Vương)", QPointF(610, 275), "poi", "🛍️");
    graph.addNode(43, "Giáo Xứ Chính Tòa (Nhà Thờ Con Gà)", QPointF(610, 320), "poi", "⛪");
    graph.addNode(44, "Trần Phú - Hoàng Văn Thụ", QPointF(610, 365), "junction", "📍");
    graph.addNode(45, "Trần Phú - Nguyễn Văn Linh", QPointF(610, 440), "junction", "📍");
    graph.addNode(46, "Bạch Đằng - Như Nguyệt / Quang Trung", QPointF(660, 80), "junction", "📍");
    graph.addNode(47, "Đầu Cầu Sông Hàn (Bờ Tây Bạch Đằng)", QPointF(660, 150), "bridge", "🌉");
    graph.addNode(48, "Bạch Đằng - Hùng Vương (Bến Du Thuyền)", QPointF(660, 275), "junction", "⚓");
    graph.addNode(49, "Đầu Cầu Rồng (Bờ Tây Bạch Đằng)", QPointF(660, 380), "bridge", "🌉");
    graph.addNode(50, "Công Viên APEC (2 Tháng 9)", QPointF(625, 410), "park", "🌳");
    graph.addNode(51, "Đường 2 Tháng 9 - Bình Minh 1", QPointF(625, 490), "junction", "📍");
    graph.addNode(52, "Đầu Cầu Trần Thị Lý (Bờ Tây 2/9)", QPointF(635, 610), "bridge", "🌉");
    graph.addNode(53, "Hòa Cường (Đường 2/9 - Duy Tân)", QPointF(580, 610), "junction", "📍");

    // 3. CÁC ĐIỂM TRÊN CẦU QUA SÔNG HÀN
    graph.addNode(54, "Cầu Sông Hàn (Giữa Cầu Quay)", QPointF(717, 150), "bridge", "🌉");
    graph.addNode(55, "Cầu Rồng (Vòm Rồng Vàng)", QPointF(717, 380), "bridge", "🐉");
    graph.addNode(56, "Cầu Trần Thị Lý (Trụ Dây Văng)", QPointF(705, 610), "bridge", "🌉");

    // 4. KHU VỰC BỜ ĐÔNG (SƠN TRÀ & NGŨ HÀNH SƠN, BÃI BIỂN MỸ KHÊ)
    graph.addNode(57, "Đầu Cầu Sông Hàn (Bờ Đông Trần Hưng Đạo)", QPointF(775, 150), "bridge", "🌉");
    graph.addNode(58, "Vincom Plaza Đà Nẵng (Sơn Trà)", QPointF(800, 150), "poi", "🛍️");
    graph.addNode(59, "Trần Hưng Đạo - Cầu Sông Hàn Bắc", QPointF(775, 80), "junction", "📍");
    graph.addNode(60, "Đầu Cầu Rồng (Bờ Đông Trần Hưng Đạo)", QPointF(775, 380), "bridge", "🌉");
    graph.addNode(61, "Trần Hưng Đạo - Cầu Rồng Bắc", QPointF(775, 270), "junction", "📍");
    graph.addNode(62, "Trần Hưng Đạo - Cầu Trần Thị Lý", QPointF(775, 610), "junction", "📍");
    graph.addNode(63, "Ngô Quyền - Phạm Văn Đồng (AH17)", QPointF(830, 150), "junction", "📍");
    graph.addNode(64, "Ngô Quyền - Võ Văn Kiệt (AH17)", QPointF(830, 380), "junction", "📍");
    graph.addNode(65, "Ngô Quyền - Trần Thị Lý (AH17)", QPointF(830, 610), "junction", "📍");
    graph.addNode(66, "Phạm Cự Lượng - Võ Văn Kiệt", QPointF(890, 380), "junction", "📍");
    graph.addNode(67, "Phạm Cự Lượng - An Hải Bắc", QPointF(890, 270), "junction", "📍");
    graph.addNode(68, "Nguyễn Thiện Kế - Võ Văn Kiệt", QPointF(940, 380), "junction", "📍");
    graph.addNode(69, "Nguyễn Thiện Kế - Lê Hữu Trác", QPointF(940, 460), "junction", "📍");
    graph.addNode(70, "Hồ Nghinh - Phạm Văn Đồng", QPointF(1015, 150), "junction", "📍");
    graph.addNode(71, "Hồ Nghinh - Võ Văn Kiệt", QPointF(1015, 380), "junction", "📍");
    graph.addNode(72, "Hà Bổng - Võ Văn Kiệt", QPointF(1050, 380), "junction", "📍");
    graph.addNode(73, "Hà Bổng - Lê Hữu Trác", QPointF(1050, 460), "junction", "📍");
    graph.addNode(74, "Bãi Tắm Phạm Văn Đồng", QPointF(1085, 150), "beach", "🏖️");
    graph.addNode(75, "Bãi Biển Mỹ Khê (Võ Văn Kiệt)", QPointF(1085, 380), "beach", "🏖️");
    graph.addNode(76, "Võ Nguyên Giáp - Lê Hữu Trác", QPointF(1085, 460), "beach", "🏖️");
    graph.addNode(77, "Võ Nguyên Giáp - Nguyễn Văn Thoại", QPointF(1085, 565), "beach", "🏖️");
    graph.addNode(78, "Ngũ Hành Sơn - Đỗ Bá (AH17)", QPointF(830, 650), "junction", "📍");
    graph.addNode(79, "Châu Thị Vĩnh Tế - Đỗ Bá", QPointF(940, 650), "junction", "📍");
    graph.addNode(80, "Đỗ Bá - Khu Đô Thị Bắc Mỹ Phú", QPointF(1010, 650), "poi", "🏘️");

    // 5. CÁC NÚT MỞ RỘNG TẠI THANH KHÊ & ĐƯỜNG NGUYỄN TẤT THÀNH
    graph.addNode(81, "Điện Biên Phủ - Hà Huy Tập", QPointF(60, 270), "junction", "📍");
    graph.addNode(82, "Điện Biên Phủ - Thái Thị Bôi", QPointF(120, 270), "junction", "📍");
    graph.addNode(83, "Điện Biên Phủ - Lê Duy Đình", QPointF(240, 270), "junction", "📍");
    graph.addNode(84, "Nguyễn Tất Thành - Hà Huy Tập", QPointF(60, 60), "junction", "🌊");
    graph.addNode(85, "Nguyễn Tất Thành - Thái Thị Bôi Bắc", QPointF(160, 48), "junction", "🌊");
    graph.addNode(86, "Nguyễn Tất Thành - Đống Đa", QPointF(360, 26), "junction", "🌊");
    graph.addNode(87, "Nguyễn Tất Thành - Nguyễn Du", QPointF(530, 20), "junction", "🌊");
    graph.addNode(88, "Nguyễn Tất Thành - Như Nguyệt", QPointF(660, 20), "junction", "🌊");
    graph.addNode(89, "Trần Cao Vân - Cửa Ngõ Tây", QPointF(-20, 140), "junction", "📍");
    graph.addNode(90, "Trần Cao Vân - Hà Huy Tập", QPointF(60, 135), "junction", "📍");
    graph.addNode(91, "Trần Cao Vân - Thanh Khê Đông", QPointF(230, 128), "junction", "📍");
    graph.addNode(92, "Trần Cao Vân - Tam Thuận", QPointF(330, 125), "junction", "📍");

    // 6. CÁC NÚT BỔ SUNG KHU VỰC SÂN BAY & THANH KHÊ TÂY
    graph.addNode(93, "Nguyễn Tất Thành - Cửa Ngõ Cảng", QPointF(485, 21), "junction", "🌊");
    graph.addNode(94, "Xô Viết Nghệ Tĩnh - Điện Biên Phủ", QPointF(360, 270), "junction", "📍");
    // ⚠️ Node 95: Không có edge kết nối — dùng node 86 thay thế cho vị trí tương đương
    graph.addNode(95, "Xô Viết Nghệ Tĩnh - Nguyễn Tất Thành", QPointF(360, 26), "junction", "🌊");
    graph.addNode(96, "Điện Biên Phủ - Xô Viết Nghệ Tĩnh Đông", QPointF(400, 270), "junction", "📍");
    graph.addNode(97, "Hà Huy Tập - Cửa Ngõ Nam", QPointF(60, 390), "junction", "📍");
    graph.addNode(98, "Nguyễn Hữu Dật - Điện Biên Phủ", QPointF(310, 270), "junction", "📍");
    graph.addNode(99, "Điện Biên Phủ - Ngã Tư Trường Chinh", QPointF(450, 270), "junction", "📍");
    graph.addNode(100, "Trần Cao Vân - Lê Độ", QPointF(400, 118), "junction", "📍");
    graph.addNode(101, "Lê Độ - Ông Ích Khiêm", QPointF(415, 150), "junction", "📍");
    graph.addNode(102, "Lê Độ - Hải Phòng Đông", QPointF(415, 175), "junction", "📍");
    graph.addNode(103, "Quang Trung - Bạch Đằng", QPointF(660, 80), "junction", "📍");
    graph.addNode(104, "Quang Trung - Trần Phú", QPointF(610, 80), "junction", "📍");
    graph.addNode(105, "Như Nguyệt - Phạm Văn Đồng Bắc", QPointF(775, 22), "junction", "📍");
    graph.addNode(106, "Phạm Văn Đồng - Trần Hưng Đạo Bắc", QPointF(830, 22), "junction", "📍");
    graph.addNode(107, "Phạm Văn Đồng - Đầu Bắc", QPointF(900, 22), "junction", "📍");
    graph.addNode(108, "An Hải Bắc - Đông Hưng Thuận", QPointF(890, 150), "junction", "📍");
    graph.addNode(109, "Trần Hưng Đạo - Phía Bắc Sơn Trà", QPointF(775, 45), "junction", "📍");
    graph.addNode(110, "Ngô Quyền Bắc - Mỹ Khê Bắc", QPointF(830, 60), "junction", "📍");
    graph.addNode(111, "Nguyễn Văn Thoại - Nam Kỳ Khởi Nghĩa", QPointF(1010, 565), "junction", "📍");
    graph.addNode(112, "Võ Nguyên Giáp - Võ Văn Kiệt", QPointF(1085, 380), "junction", "📍");

    // 7. CÁC NÚT MỞ RỘNG MẠNG LƯỚI ĐÔ THỊ BỜ ĐÔNG (SƠN TRÀ & NGŨ HÀNH SƠN)
    // --- Tuyến đường MORRISON (Y = 210) ---
    graph.addNode(113, "Trần Hưng Đạo - Morrison", QPointF(775, 210), "junction", "📍");
    graph.addNode(114, "Ngô Quyền - Morrison (AH17)", QPointF(830, 210), "junction", "📍");
    graph.addNode(115, "Phạm Cự Lượng - Morrison", QPointF(890, 210), "junction", "📍");
    graph.addNode(116, "Chính Hữu - Morrison", QPointF(975, 210), "junction", "📍");
    graph.addNode(117, "Hồ Nghinh - Morrison", QPointF(1015, 210), "junction", "📍");
    graph.addNode(118, "Hà Nghinh - Morrison", QPointF(1050, 210), "junction", "📍");
    graph.addNode(119, "Võ Nguyên Giáp - Morrison", QPointF(1085, 210), "beach", "🏖️");

    // --- Tuyến đường AN HẢI BẮC / LƯU HỮU PHƯỚC (Y = 270) ---
    graph.addNode(120, "Trần Hưng Đạo - An Hải Bắc", QPointF(775, 270), "junction", "📍");
    graph.addNode(121, "Ngô Quyền - An Hải Bắc (AH17)", QPointF(830, 270), "junction", "📍");
    graph.addNode(122, "Chính Hữu - An Hải Bắc", QPointF(975, 270), "junction", "📍");
    graph.addNode(123, "Hồ Nghinh - An Hải Bắc", QPointF(1015, 270), "junction", "📍");
    graph.addNode(124, "Võ Nguyên Giáp - An Hải Bắc", QPointF(1085, 270), "beach", "🏖️");

    // --- Tuyến đường NGUYỄN CÔNG TRỨ (Y = 330) ---
    graph.addNode(125, "Trần Hưng Đạo - Nguyễn Công Trứ", QPointF(775, 330), "junction", "📍");
    graph.addNode(126, "Ngô Quyền - Nguyễn Công Trứ (AH17)", QPointF(830, 330), "junction", "📍");
    graph.addNode(127, "Phạm Cự Lượng - Nguyễn Công Trứ", QPointF(890, 330), "junction", "📍");
    graph.addNode(128, "Nguyễn Thiện Kế - Nguyễn Công Trứ", QPointF(940, 330), "junction", "📍");
    graph.addNode(129, "Chính Hữu - Nguyễn Công Trứ", QPointF(975, 330), "junction", "📍");
    graph.addNode(130, "Hồ Nghinh - Nguyễn Công Trứ", QPointF(1015, 330), "junction", "📍");
    graph.addNode(131, "Võ Nguyên Giáp - Nguyễn Công Trứ", QPointF(1085, 330), "beach", "🏖️");

    // --- Tuyến VÕ VĂN KIỆT bổ sung (Y = 380) ---
    graph.addNode(132, "Chính Hữu - Võ Văn Kiệt", QPointF(975, 380), "junction", "📍");

    // --- Tuyến LÊ HỮU TRÁC bổ sung (Y = 460) ---
    graph.addNode(133, "Ngô Quyền - Lê Hữu Trác (AH17)", QPointF(830, 460), "junction", "📍");
    graph.addNode(134, "Phạm Cự Lượng - Lê Hữu Trác", QPointF(890, 460), "junction", "📍");

    // --- Tuyến NGUYỄN DUY HIỆU (Y = 510) ---
    graph.addNode(135, "Ngô Quyền - Nguyễn Duy Hiệu (AH17)", QPointF(830, 510), "junction", "📍");
    graph.addNode(136, "Phạm Cự Lượng - Nguyễn Duy Hiệu", QPointF(890, 510), "junction", "📍");
    graph.addNode(137, "Nguyễn Thiện Kế - Nguyễn Duy Hiệu", QPointF(940, 510), "junction", "📍");
    graph.addNode(138, "Hà Bổng - Nguyễn Duy Hiệu", QPointF(1050, 510), "junction", "📍");
    graph.addNode(139, "Võ Nguyên Giáp - Nguyễn Duy Hiệu", QPointF(1085, 510), "beach", "🏖️");

    // --- Đại lộ NGUYỄN VĂN THOẠI (Y = 565) ---
    graph.addNode(140, "Ngũ Hành Sơn - Nguyễn Văn Thoại (AH17)", QPointF(830, 565), "junction", "📍");
    graph.addNode(141, "Phạm Cự Lượng - Nguyễn Văn Thoại", QPointF(890, 565), "junction", "📍");
    graph.addNode(142, "Nguyễn Thiện Kế - Nguyễn Văn Thoại", QPointF(940, 565), "junction", "📍");
    graph.addNode(143, "Hà Bổng - Nguyễn Văn Thoại", QPointF(1050, 565), "junction", "📍");

    // --- Ven sông bờ Đông phía Nam: Đ. CHƯƠNG DƯƠNG (X = 765) ---
    graph.addNode(144, "Chương Dương - Cầu Trần Thị Lý", QPointF(765, 610), "junction", "📍");
    graph.addNode(145, "Chương Dương - Đỗ Bá", QPointF(765, 650), "junction", "📍");
    graph.addNode(146, "Chương Dương - Dương Khuê", QPointF(765, 710), "junction", "📍");

    // --- Tuyến ĐỖ BÁ & DƯƠNG KHUÊ kéo dài đến biển ---
    graph.addNode(147, "Võ Nguyên Giáp - Đỗ Bá", QPointF(1085, 650), "beach", "🏖️");
    graph.addNode(148, "Ngũ Hành Sơn - Dương Khuê (AH17)", QPointF(830, 710), "junction", "📍");
    graph.addNode(149, "Châu Thị Vĩnh Tế - Dương Khuê", QPointF(940, 710), "junction", "📍");
    graph.addNode(150, "Võ Nguyên Giáp - Dương Khuê", QPointF(1085, 710), "beach", "🏖️");

    // --- Ven sông TRẦN HƯNG ĐẠO (đoạn giữa Cầu Rồng và Cầu Trần Thị Lý) ---
    graph.addNode(151, "Trần Hưng Đạo - Mai Hắc Đế", QPointF(775, 430), "junction", "📍");
    graph.addNode(152, "Trần Hưng Đạo - Hà Thị Thân", QPointF(775, 490), "junction", "📍");
    graph.addNode(153, "Trần Hưng Đạo - Bùi Thị Xuân", QPointF(775, 550), "junction", "📍");

    // --- Các nút trên trục PHẠM VĂN ĐỒNG bổ sung ---
    graph.addNode(154, "Chính Hữu - Phạm Văn Đồng", QPointF(975, 150), "junction", "📍");
    graph.addNode(155, "Hà Nghinh - Phạm Văn Đồng", QPointF(1050, 150), "junction", "📍");

    // --- POIs / Địa danh nổi bật bờ Đông (clickable & routeable) ---
    graph.addNode(156, "Tượng Cá Chép Hoá Rồng & Cầu Tình Yêu", QPointF(760, 350), "poi", "⛲");
    graph.addNode(157, "Chợ Đêm Sơn Trà (Mai Hắc Đế)", QPointF(795, 430), "poi", "🏮");
    graph.addNode(158, "Mộc Quán Seafood Đà Nẵng", QPointF(980, 365), "poi", "🦀");
    graph.addNode(159, "Cửa Ngõ Café (Nguyễn Thiện Kế)", QPointF(955, 395), "poi", "☕");
    graph.addNode(160, "Bếp Cuốn Đà Nẵng - Michelin Selected", QPointF(920, 435), "poi", "🥢");
    graph.addNode(161, "B&B Bakery Đà Nẵng", QPointF(920, 475), "poi", "🥐");
    graph.addNode(162, "Khách sạn Mường Thanh Luxury Đà Nẵng", QPointF(1070, 565), "poi", "🏨");
    graph.addNode(163, "Bikini Bottom Express Đà Nẵng", QPointF(1070, 650), "poi", "🍹");
    graph.addNode(164, "Công Viên Biển Đông (Quảng Trường Hòa Bình)", QPointF(1065, 130), "park", "🕊️");
    graph.addNode(165, "Herbal Spa Đà Nẵng", QPointF(855, 270), "poi", "🌿");
    graph.addNode(166, "Pizza 4P's Indochina Đà Nẵng", QPointF(795, 170), "poi", "🍕");
    graph.addNode(167, "Four Points by Sheraton Danang", QPointF(1070, 80), "poi", "🏨");

    auto dist = [](QPointF a, QPointF b) {
      return std::hypot(a.x() - b.x(), a.y() - b.y());
    };
    auto connect = [&](int u, int v, double traffic = 1.0, const QString &st = "",
                       bool bridge = false, bool major = true) {
      graph.addEdge(u, v, dist(graph.nodes[u].pos, graph.nodes[v].pos),
                    traffic, st, bridge, major);
    };

    // --- CÁC TUYẾN ĐƯỜNG BỜ TÂY & THANH KHÊ (LIÊN TỤC KHÔNG CỤT) ---
    // Tuyến đường bờ biển Nguyễn Tất Thành (uốn lượn suốt bờ vịnh)
    connect(0, 84, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(84, 85, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(85, 1, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(1, 86, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(86, 17, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(17, 87, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(87, 88, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(88, 46, 1.2, "Đ. Như Nguyệt - Bạch Đằng", false, true);

    // Tuyến Trần Cao Vân
    connect(89, 90, 1.2, "Đ. Trần Cao Vân", false, true);
    connect(90, 2, 1.2, "Đ. Trần Cao Vân", false, true);
    connect(2, 91, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(91, 92, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(92, 16, 1.3, "Đ. Trần Cao Vân", false, true);

    // Tuyến Hà Huy Tập
    connect(84, 90, 1.2, "Đ. Hà Huy Tập", false, true);
    connect(90, 81, 1.2, "Đ. Hà Huy Tập", false, true);

    // Tuyến Thái Thị Bôi
    connect(2, 82, 1.2, "Đ. Thái Thị Bôi", false, false);

    // Tuyến Điện Biên Phủ (kéo dài xuyên suốt Thanh Khê từ mạn Tây vào trung tâm)
    connect(3, 81, 1.2, "Đ. Điện Biên Phủ", false, true);
    connect(81, 82, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(82, 4, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(4, 83, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(83, 7, 1.4, "Đ. Điện Biên Phủ", false, true);
    connect(7, 14, 1.4, "Đ. Lý Thái Tổ", false, true);

    // Lê Duy Đình
    connect(83, 10, 1.1, "Đ. Lê Duy Đình", false, false);
    connect(10, 18, 1.1, "Đ. Lê Duy Đình", false, false);

    // Hải Phòng (qua Ga Đà Nẵng)
    connect(14, 15, 1.2, "Đ. Hải Phòng", false, true);
    connect(15, 33, 1.3, "Đ. Hải Phòng", false, true);
    connect(33, 40, 1.2, "Đ. Hải Phòng", false, true);

    // Ông Ích Khiêm
    connect(17, 16, 1.2, "Đ. Ông Ích Khiêm", false, true);
    connect(16, 15, 1.4, "Đ. Ông Ích Khiêm", false, true);
    connect(15, 25, 1.5, "Đ. Ông Ích Khiêm", false, true);
    connect(25, 24, 1.8, "Đ. Ông Ích Khiêm", false, true);

    // Lê Duẩn
    connect(14, 25, 1.4, "Đ. Lý Thái Tổ - Lê Duẩn", false, true);
    connect(25, 26, 1.6, "Đ. Lê Duẩn", false, true);
    connect(26, 34, 1.5, "Đ. Lê Duẩn", false, true);
    connect(34, 41, 1.4, "Đ. Lê Duẩn", false, true);
    connect(41, 47, 1.3, "Đ. Lê Duẩn", false, true);

    // Hùng Vương
    connect(14, 24, 1.5, "Đ. Hùng Vương", false, true);
    connect(24, 27, 1.9, "Đ. Hùng Vương", false, true);
    connect(27, 35, 1.7, "Đ. Hùng Vương", false, true);
    connect(35, 42, 1.8, "Đ. Hùng Vương", false, true);
    connect(42, 48, 1.5, "Đ. Hùng Vương", false, true);

    // Lê Lợi & Phan Châu Trinh
    connect(32, 33, 1.2, "Đ. Lê Lợi", false, true);
    connect(33, 34, 1.3, "Đ. Lê Lợi", false, true);
    connect(34, 35, 1.4, "Đ. Phan Châu Trinh", false, true);
    connect(35, 36, 1.4, "Đ. Phan Châu Trinh", false, true);
    connect(36, 37, 1.5, "Đ. Phan Châu Trinh", false, true);
    connect(37, 38, 1.3, "Đ. Phan Châu Trinh", false, true);

    // Hoàng Diệu & Hoàng Văn Thụ
    connect(26, 28, 1.5, "Đ. Hoàng Diệu", false, true);
    connect(28, 29, 1.6, "Đ. Hoàng Diệu", false, true);
    connect(29, 30, 1.4, "Đ. Hoàng Diệu", false, true);
    connect(28, 36, 1.3, "Đ. Hoàng Văn Thụ", false, false);
    connect(36, 44, 1.3, "Đ. Hoàng Văn Thụ", false, false);
    connect(44, 49, 1.4, "Đ. Hoàng Văn Thụ", false, false);

    // Trần Phú (trục 1 chiều Bắc - Nam)
    connect(39, 40, 1.3, "Đ. Trần Phú", false, true);
    connect(40, 41, 1.4, "Đ. Trần Phú", false, true);
    connect(41, 42, 1.6, "Đ. Trần Phú", false, true);
    connect(42, 43, 1.5, "Đ. Trần Phú", false, true);
    connect(43, 44, 1.4, "Đ. Trần Phú", false, true);
    connect(44, 45, 1.5, "Đ. Trần Phú", false, true);
    connect(45, 50, 1.3, "Đ. Trần Phú", false, true);

    // Bạch Đằng (trục ven sông bờ Tây tại X = 660, KHÔNG đè lên sông)
    connect(46, 47, 1.2, "Đ. Bạch Đằng", false, true);
    connect(47, 48, 1.3, "Đ. Bạch Đằng", false, true);
    connect(48, 49, 1.4, "Đ. Bạch Đằng", false, true);
    connect(49, 50, 1.2, "Đ. Bạch Đằng Nối Dài", false, true);
    connect(50, 51, 1.3, "Đ. 2 Tháng 9", false, true);
    connect(51, 52, 1.2, "Đ. 2 Tháng 9", false, true);
    connect(52, 53, 1.2, "Đ. 2 Tháng 9", false, true);

    // Đường ven Công viên 29/3 và Sân bay
    connect(7, 8, 1.1, "Đ. Nguyễn Tri Phương", false, true);
    connect(8, 10, 1.2, "Đ. Nguyễn Tri Phương", false, true);
    connect(10, 9, 1.2, "Đ. Nguyễn Tri Phương", false, true);
    connect(9, 11, 1.3, "Đ. Nguyễn Tri Phương", false, true);
    connect(18, 20, 1.2, "Đ. Phan Thanh", false, false);
    connect(18, 19, 1.1, "Đường ven Hồ Hàm Nghi", false, false);
    connect(19, 21, 1.2, "Đ. Hàm Nghi", false, false);

    // Đại lộ Nguyễn Văn Linh (trục lớn nối Cổng Sân Bay ↔ Cầu Rồng)
    connect(5, 11, 1.3, "Đ. Nguyễn Văn Linh", false, true);
    connect(11, 20, 1.5, "Đ. Nguyễn Văn Linh", false, true);
    connect(20, 21, 1.6, "Đ. Nguyễn Văn Linh", false, true);
    connect(21, 22, 1.6, "Đ. Nguyễn Văn Linh", false, true);
    connect(22, 31, 1.7, "Đ. Nguyễn Văn Linh", false, true);
    connect(31, 29, 1.8, "Đ. Nguyễn Văn Linh", false, true);
    connect(29, 37, 1.9, "Đ. Nguyễn Văn Linh", false, true);
    connect(37, 45, 1.8, "Đ. Nguyễn Văn Linh", false, true);
    connect(45, 49, 1.5, "Đ. Nguyễn Văn Linh", false, true);

    // Duy Tân
    connect(11, 12, 1.4, "Đ. Nguyễn Hữu Thọ", false, true);
    connect(13, 12, 1.2, "Đ. Duy Tân", false, true);
    connect(12, 23, 1.3, "Đ. Duy Tân", false, true);
    connect(23, 30, 1.3, "Đ. Duy Tân", false, true);
    connect(30, 53, 1.2, "Đ. Duy Tân", false, true);
    connect(53, 52, 1.2, "Đ. Duy Tân", false, true);
    connect(22, 23, 1.3, "Đ. Lê Đình Lý", false, false);

    // --- CÁC CÂY CẦU VƯỢT SÔNG HÀN (NỐI TỪ X=660 SANG X=775) ---
    connect(47, 54, 1.2, "Cầu Sông Hàn", true, true);
    connect(54, 57, 1.2, "Cầu Sông Hàn", true, true);
    connect(49, 55, 2.1, "Cầu Rồng", true, true);
    connect(55, 60, 2.0, "Cầu Rồng", true, true);
    connect(52, 56, 1.3, "Cầu Trần Thị Lý", true, true);
    connect(56, 62, 1.3, "Cầu Trần Thị Lý", true, true);

    // --- CÁC ĐƯỜNG TRỤC BỜ ĐÔNG (SƠN TRÀ & NGŨ HÀNH SƠN - MẠNG LƯỚI ĐÔ THỊ ĐẦY ĐỦ) ---
    // 1. Tuyến ven sông Đ. Trần Hưng Đạo (X = 775)
    connect(105, 109, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(109, 59, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(59, 57, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(57, 113, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(113, 120, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(120, 61, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(61, 125, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(125, 60, 1.4, "Đ. Trần Hưng Đạo", false, true);
    connect(60, 151, 1.4, "Đ. Trần Hưng Đạo", false, true);
    connect(151, 152, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(152, 153, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(153, 62, 1.3, "Đ. Trần Hưng Đạo", false, true);

    // 2. Tuyến ven sông phía Nam Đ. Chương Dương (X = 765)
    connect(62, 144, 1.2, "Đ. Chương Dương", false, true);
    connect(144, 145, 1.2, "Đ. Chương Dương", false, true);
    connect(145, 146, 1.2, "Đ. Chương Dương", false, true);

    // 3. Đại lộ Ngô Quyền (AH17) / Ngũ Hành Sơn (X = 830)
    connect(106, 110, 1.2, "Đ. Ngô Quyền (AH17)", false, true);
    connect(110, 63, 1.3, "Đ. Ngô Quyền (AH17)", false, true);
    connect(63, 114, 1.3, "Đ. Ngô Quyền (AH17)", false, true);
    connect(114, 121, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(121, 126, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(126, 64, 1.5, "Đ. Ngô Quyền (AH17)", false, true);
    connect(64, 133, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(133, 135, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(135, 140, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(140, 65, 1.3, "Đ. Ngũ Hành Sơn (AH17)", false, true);
    connect(65, 78, 1.3, "Đ. Ngũ Hành Sơn (AH17)", false, true);
    connect(78, 148, 1.2, "Đ. Ngũ Hành Sơn (AH17)", false, true);

    // 4. Tuyến Đ. Phạm Cự Lượng (X = 890)
    connect(108, 115, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(115, 67, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(67, 127, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(127, 66, 1.3, "Đ. Phạm Cự Lượng", false, false);
    connect(66, 134, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(134, 136, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(136, 141, 1.2, "Đ. Phạm Cự Lượng", false, false);

    // 5. Tuyến Đ. Nguyễn Thiện Kế & Đ. Châu Thị Vĩnh Tế (X = 940)
    connect(128, 68, 1.2, "Đ. Nguyễn Thiện Kế", false, false);
    connect(68, 69, 1.2, "Đ. Nguyễn Thiện Kế", false, false);
    connect(69, 137, 1.2, "Đ. Nguyễn Thiện Kế", false, false);
    connect(137, 142, 1.2, "Đ. Nguyễn Thiện Kế", false, false);
    connect(142, 79, 1.2, "Đ. Châu Thị Vĩnh Tế", false, false);
    connect(79, 149, 1.2, "Đ. Châu Thị Vĩnh Tế", false, false);

    // 6. Tuyến Đ. Chính Hữu (X = 975)
    connect(154, 116, 1.2, "Đ. Chính Hữu", false, false);
    connect(116, 122, 1.2, "Đ. Chính Hữu", false, false);
    connect(122, 129, 1.2, "Đ. Chính Hữu", false, false);
    connect(129, 132, 1.2, "Đ. Chính Hữu", false, false);

    // 7. Tuyến Đ. Hồ Nghinh (X = 1015)
    connect(70, 117, 1.2, "Đ. Hồ Nghinh", false, false);
    connect(117, 123, 1.2, "Đ. Hồ Nghinh", false, false);
    connect(123, 130, 1.2, "Đ. Hồ Nghinh", false, false);
    connect(130, 71, 1.2, "Đ. Hồ Nghinh", false, false);

    // 8. Tuyến Đ. Hà Bổng / Hà Nghinh (X = 1050)
    connect(155, 118, 1.2, "Đ. Hà Nghinh", false, false);
    connect(118, 72, 1.2, "Đ. Hà Bổng", false, false);
    connect(72, 73, 1.2, "Đ. Hà Bổng", false, false);
    connect(73, 138, 1.2, "Đ. Hà Bổng", false, false);
    connect(138, 143, 1.2, "Đ. Hà Bổng", false, false);

    // 9. Đại lộ ven biển Võ Nguyên Giáp (X = 1085)
    connect(167, 74, 1.2, "Đ. Võ Nguyên Giáp", false, true);
    connect(74, 119, 1.2, "Đ. Võ Nguyên Giáp", false, true);
    connect(119, 124, 1.2, "Đ. Võ Nguyên Giáp", false, true);
    connect(124, 131, 1.2, "Đ. Võ Nguyên Giáp", false, true);
    connect(131, 75, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(75, 76, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(76, 139, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(139, 77, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(77, 147, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(147, 150, 1.2, "Đ. Võ Nguyên Giáp", false, true);

    // 10. Đại lộ Phạm Văn Đồng (Y = 150)
    connect(57, 58, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(58, 63, 1.3, "Đ. Phạm Văn Đồng", false, true);
    connect(63, 108, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(108, 154, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(154, 70, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(70, 155, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(155, 74, 1.2, "Đ. Phạm Văn Đồng", false, true);

    // 11. Tuyến Đ. Morrison (Y = 210)
    connect(113, 114, 1.2, "Đ. Morrison", false, false);
    connect(114, 115, 1.2, "Đ. Morrison", false, false);
    connect(115, 116, 1.2, "Đ. Morrison", false, false);
    connect(116, 117, 1.2, "Đ. Morrison", false, false);
    connect(117, 118, 1.2, "Đ. Morrison", false, false);
    connect(118, 119, 1.2, "Đ. Morrison", false, false);

    // 12. Tuyến Đ. An Hải Bắc (Y = 270)
    connect(120, 121, 1.2, "Đ. An Hải Bắc", false, false);
    connect(121, 67, 1.2, "Đ. An Hải Bắc", false, false);
    connect(67, 122, 1.2, "Đ. An Hải Bắc", false, false);
    connect(122, 123, 1.2, "Đ. An Hải Bắc", false, false);
    connect(123, 124, 1.2, "Đ. An Hải Bắc", false, false);

    // 13. Tuyến Đ. Nguyễn Công Trứ (Y = 330)
    connect(125, 126, 1.3, "Đ. Nguyễn Công Trứ", false, false);
    connect(126, 127, 1.3, "Đ. Nguyễn Công Trứ", false, false);
    connect(127, 128, 1.3, "Đ. Nguyễn Công Trứ", false, false);
    connect(128, 129, 1.3, "Đ. Nguyễn Công Trứ", false, false);
    connect(129, 130, 1.3, "Đ. Nguyễn Công Trứ", false, false);
    connect(130, 131, 1.2, "Đ. Nguyễn Công Trứ", false, false);

    // 14. Đại lộ Võ Văn Kiệt (Y = 380)
    connect(60, 64, 1.4, "Đ. Võ Văn Kiệt", false, true);
    connect(64, 66, 1.5, "Đ. Võ Văn Kiệt", false, true);
    connect(66, 68, 1.4, "Đ. Võ Văn Kiệt", false, true);
    connect(68, 132, 1.3, "Đ. Võ Văn Kiệt", false, true);
    connect(132, 71, 1.3, "Đ. Võ Văn Kiệt", false, true);
    connect(71, 72, 1.3, "Đ. Võ Văn Kiệt", false, true);
    connect(72, 75, 1.2, "Đ. Võ Văn Kiệt", false, true);

    // 15. Tuyến Đ. Lê Hữu Trác (Y = 460)
    connect(133, 134, 1.2, "Đ. Lê Hữu Trác", false, false);
    connect(134, 69, 1.2, "Đ. Lê Hữu Trác", false, false);
    connect(69, 73, 1.2, "Đ. Lê Hữu Trác", false, false);
    connect(73, 76, 1.2, "Đ. Lê Hữu Trác", false, false);

    // 16. Tuyến Đ. Nguyễn Duy Hiệu (Y = 510)
    connect(135, 136, 1.2, "Đ. Nguyễn Duy Hiệu", false, false);
    connect(136, 137, 1.2, "Đ. Nguyễn Duy Hiệu", false, false);
    connect(137, 138, 1.2, "Đ. Nguyễn Duy Hiệu", false, false);
    connect(138, 139, 1.2, "Đ. Nguyễn Duy Hiệu", false, false);

    // 17. Đại lộ Nguyễn Văn Thoại (Y = 565)
    connect(140, 141, 1.4, "Đ. Nguyễn Văn Thoại", false, true);
    connect(141, 142, 1.4, "Đ. Nguyễn Văn Thoại", false, true);
    connect(142, 143, 1.3, "Đ. Nguyễn Văn Thoại", false, true);
    connect(143, 77, 1.3, "Đ. Nguyễn Văn Thoại", false, true);

    // 18. Tuyến Đ. Trần Thị Lý Đông (Y = 610)
    connect(62, 65, 1.3, "Đ. Trần Thị Lý Đông", false, true);

    // 19. Tuyến Đ. Đỗ Bá (Y = 650)
    connect(145, 78, 1.2, "Đ. Đỗ Bá", false, false);
    connect(78, 79, 1.2, "Đ. Đỗ Bá", false, false);
    connect(79, 80, 1.2, "Đ. Đỗ Bá", false, false);
    connect(80, 147, 1.2, "Đ. Đỗ Bá", false, false);

    // 20. Tuyến Đ. Dương Khuê (Y = 710)
    connect(146, 148, 1.2, "Đ. Dương Khuê", false, false);
    connect(148, 149, 1.2, "Đ. Dương Khuê", false, false);
    connect(149, 150, 1.2, "Đ. Dương Khuê", false, false);

    // 21. Kết nối các điểm du lịch & POI vào mạng lưới
    connect(125, 156, 1.1, "Bến Du Thuyền DHC & Cầu Tình Yêu", false, false);
    connect(151, 157, 1.3, "Chợ Đêm Sơn Trà", false, false);
    connect(72, 158, 1.2, "Đ. Võ Văn Kiệt (Mộc Quán)", false, false);
    connect(128, 159, 1.1, "Đ. Nguyễn Thiện Kế (Cửa Ngõ Café)", false, false);
    connect(69, 160, 1.1, "Đ. Lê Hữu Trác (Bếp Cuốn Michelin)", false, false);
    connect(69, 161, 1.1, "Đ. Lê Hữu Trác (B&B Bakery)", false, false);
    connect(77, 162, 1.2, "Đ. Võ Nguyên Giáp (Mường Thanh)", false, false);
    connect(147, 163, 1.2, "Đ. Võ Nguyên Giáp (Bikini Bottom)", false, false);
    connect(74, 164, 1.1, "Công Viên Biển Đông", false, false);
    connect(121, 165, 1.2, "Herbal Spa", false, false);
    connect(58, 166, 1.2, "Pizza 4P's Indochina", false, false);
    connect(74, 167, 1.2, "Four Points by Sheraton", false, false);

    // --- CÁC KẾT NỐI BỔ SUNG CHO NODES 93-112 ---
    // Nguyễn Tất Thành bổ sung (node 93 giữa 17 và 87)
    connect(17, 93, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(93, 87, 1.1, "Đ. Nguyễn Tất Thành", false, true);

    // Xô Viết Nghệ Tĩnh (nối Điện Biên Phủ ↔ Nguyễn Tất Thành tại X=360)
    // node 95 ≈ node 86 (Nguyễn Tất Thành - Đống Đa) - dùng chung
    connect(94, 96, 1.3, "Đ. Điện Biên Phủ", false, true);   // mở rộng DBP
    connect(83, 94, 1.3, "Đ. Điện Biên Phủ", false, true);   // Lê Duy Đình → Xô Viết NTĩnh
    connect(94, 14, 1.4, "Đ. Xô Viết Nghệ Tĩnh", false, true); // Xô Viết NTĩnh ↔ Lý Thái Tổ
    connect(86, 94, 1.2, "Đ. Xô Viết Nghệ Tĩnh", false, true); // Nguyễn Tất Thành → DBP qua XVNTĩnh
    connect(96, 7, 1.3, "Đ. Điện Biên Phủ", false, true);    // DBP → Nguyễn Tri Phương
    connect(98, 94, 1.3, "Đ. Điện Biên Phủ", false, true);   // Nguyễn Hữu Dật → XVNTĩnh
    connect(83, 98, 1.2, "Đ. Điện Biên Phủ", false, true);
    connect(99, 96, 1.4, "Đ. Điện Biên Phủ", false, true);   // Trường Chinh → Xô Viết
    connect(99, 25, 1.5, "Đ. Điện Biên Phủ", false, true);   // → Lê Duẩn nút trung tâm

    // Hà Huy Tập kéo dài xuống Nam (đến cổng sân bay)
    connect(81, 97, 1.3, "Đ. Hà Huy Tập", false, true);
    connect(97, 5, 1.2, "Đ. Hà Huy Tập", false, true);

    // Trần Cao Vân - Lê Độ nhánh
    connect(92, 100, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(100, 16, 1.3, "Đ. Trần Cao Vân", false, true);   // nối vào Ông Ích Khiêm
    connect(100, 101, 1.2, "Đ. Lê Độ", false, false);
    connect(101, 15, 1.2, "Đ. Lê Độ", false, false);
    connect(101, 102, 1.2, "Đ. Lê Độ", false, false);
    connect(102, 15, 1.2, "Đ. Hải Phòng", false, true);

    // Quang Trung (nối Trần Phú ↔ Bạch Đằng theo chiều ngang)
    connect(104, 103, 1.3, "Đ. Quang Trung", false, true);
    connect(103, 46, 1.3, "Đ. Quang Trung", false, true);    // → Bạch Đằng
    connect(39, 104, 1.3, "Đ. Quang Trung", false, true);    // Trần Phú → Quang Trung
    connect(32, 104, 1.2, "Đ. Quang Trung", false, true);    // Lê Lợi → Quang Trung

    // Nối đường Như Nguyệt qua bờ Đông phía Bắc
    connect(88, 105, 1.2, "Đ. Như Nguyệt - Phạm Văn Đồng", false, true);
    connect(105, 106, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(106, 107, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(107, 108, 1.2, "Đ. Phạm Văn Đồng", false, false);
    connect(80, 111, 1.2, "Đ. Nguyễn Văn Thoại", false, false);
    connect(111, 77, 1.2, "Đ. Nguyễn Văn Thoại", false, true);
  }

  // ==================== VẼ BẢN ĐỒ CHI TIẾT (ĐỊA HÌNH, SÔNG, BIỂN, ĐƯỜNG XÃ) ====================
  void drawMap() {
    // 1. BIỂN ĐÔNG & BÃI BIỂN MỸ KHÊ (PHÍA ĐÔNG)
    QColor oceanColor = darkMode ? QColor(19, 32, 48) : QColor(168, 208, 224);
    auto *oceanRect = scene->addRect(1095, -80, 300, 880, QPen(Qt::NoPen), QBrush(oceanColor));
    oceanRect->setZValue(-32);

    // Bờ cát vàng óng ánh Bãi biển Mỹ Khê
    QColor sandColor = darkMode ? QColor(54, 48, 38) : QColor(252, 240, 206);
    auto *sandRect = scene->addRect(1075, -80, 20, 880, QPen(Qt::NoPen), QBrush(sandColor));
    sandRect->setZValue(-28);

    // Các đường gợn sóng biển Mỹ Khê
    QColor waveColor = darkMode ? QColor(28, 48, 72) : QColor(188, 222, 235);
    for (int wy = -40; wy < 780; wy += 80) {
      QPainterPath wave;
      wave.moveTo(1110, wy);
      wave.quadTo(1130, wy + 15, 1150, wy);
      wave.quadTo(1170, wy - 15, 1190, wy);
      auto *wItem = scene->addPath(wave, QPen(waveColor, 1.5, Qt::SolidLine, Qt::RoundCap));
      wItem->setZValue(-30);
    }

    auto *oceanText = scene->addText("B I Ể N   Đ Ô N G");
    QFont of("Segoe UI", 16, QFont::Bold);
    of.setLetterSpacing(QFont::AbsoluteSpacing, 6);
    oceanText->setFont(of);
    oceanText->setDefaultTextColor(darkMode ? QColor(55, 85, 120) : QColor(135, 180, 198));
    oceanText->setPos(1165, 230);
    oceanText->setRotation(90);
    oceanText->setZValue(-26);

    // 2. VỊNH ĐÀ NẴNG (MEN THEO CUNG ĐƯỜNG NGUYỄN TẤT THÀNH)
    QPainterPath bay;
    bay.moveTo(-40, -80);
    bay.lineTo(675, -80);
    bay.lineTo(675, 10);
    bay.cubicTo(560, 12, 445, 14, 360, 18);
    bay.cubicTo(280, 24, 185, 38, 100, 50);
    bay.cubicTo(50, 58, 0, 66, -40, 72);
    bay.closeSubpath();
    auto *bayItem = scene->addPath(bay, QPen(Qt::NoPen), QBrush(oceanColor));
    bayItem->setZValue(-32);

    auto *bayText = scene->addText("V Ị N H   Đ À   N Ẵ N G");
    QFont bf("Segoe UI", 12, QFont::Bold);
    bf.setLetterSpacing(QFont::AbsoluteSpacing, 5);
    bayText->setFont(bf);
    bayText->setDefaultTextColor(darkMode ? QColor(55, 85, 120) : QColor(135, 180, 198));
    bayText->setPos(100, -25);
    bayText->setZValue(-26);

    // 3. KHU VỰC ĐÔ THỊ BỜ ĐÔNG (QUẬN SƠN TRÀ & NGŨ HÀNH SƠN - CÁC KHỐI PHỐ BÀN CỜ)
    auto addUrbanBlock = [&](double bx, double by, double bw, double bh) {
      QPainterPath p;
      p.addRoundedRect(QRectF(bx, by, bw, bh), 3, 3);
      QColor blockFill = darkMode ? QColor(36, 38, 48) : QColor(235, 232, 225);
      QColor blockBdr  = darkMode ? QColor(48, 50, 62) : QColor(224, 220, 212);
      auto *item = scene->addPath(p, QPen(blockBdr, 0.8), QBrush(blockFill));
      item->setZValue(-24);
    };

    // Khối phố giữa Phạm Văn Đồng (150) và Morrison (210)
    addUrbanBlock(838, 158, 44, 44);
    addUrbanBlock(898, 158, 69, 44);
    addUrbanBlock(983, 158, 24, 44);
    addUrbanBlock(1023, 158, 21, 44);
    addUrbanBlock(1058, 158, 21, 44);

    // Khối phố giữa Morrison (210) và An Hải Bắc (270)
    addUrbanBlock(783, 218, 39, 44);
    addUrbanBlock(838, 218, 44, 44);
    addUrbanBlock(898, 218, 69, 44);
    addUrbanBlock(983, 218, 24, 44);
    addUrbanBlock(1023, 218, 21, 44);
    addUrbanBlock(1058, 218, 21, 44);

    // Khối phố giữa An Hải Bắc (270) và Nguyễn Công Trứ (330)
    addUrbanBlock(783, 278, 39, 44);
    addUrbanBlock(838, 278, 44, 44);
    addUrbanBlock(898, 278, 34, 44);
    addUrbanBlock(948, 278, 19, 44);
    addUrbanBlock(983, 278, 24, 44);
    addUrbanBlock(1023, 278, 54, 44);

    // Khối phố giữa Nguyễn Công Trứ (330) và Võ Văn Kiệt (380)
    addUrbanBlock(783, 338, 39, 34);
    addUrbanBlock(838, 338, 44, 34);
    addUrbanBlock(898, 338, 34, 34);
    addUrbanBlock(948, 338, 19, 34);
    addUrbanBlock(983, 338, 24, 34);
    addUrbanBlock(1023, 338, 54, 34);

    // Khối phố giữa Võ Văn Kiệt (380) và Lê Hữu Trác (460)
    addUrbanBlock(783, 388, 39, 64);
    addUrbanBlock(838, 388, 44, 64);
    addUrbanBlock(898, 388, 34, 64);
    addUrbanBlock(948, 388, 94, 64);
    addUrbanBlock(1058, 388, 21, 64);

    // Khối phố giữa Lê Hữu Trác (460) và Nguyễn Duy Hiệu (510)
    addUrbanBlock(783, 468, 39, 34);
    addUrbanBlock(838, 468, 44, 34);
    addUrbanBlock(898, 468, 34, 34);
    addUrbanBlock(948, 468, 94, 34);
    addUrbanBlock(1058, 468, 21, 34);

    // Khối phố giữa Nguyễn Duy Hiệu (510) và Nguyễn Văn Thoại (565)
    addUrbanBlock(783, 518, 39, 39);
    addUrbanBlock(838, 518, 44, 39);
    addUrbanBlock(898, 518, 34, 39);
    addUrbanBlock(948, 518, 94, 39);
    addUrbanBlock(1058, 518, 21, 39);

    // Khối phố Bắc Mỹ Phú: Nam Nguyễn Văn Thoại (565) đến Đỗ Bá (650) & Dương Khuê (710)
    addUrbanBlock(838, 575, 94, 65);
    addUrbanBlock(948, 575, 129, 65);
    addUrbanBlock(773, 658, 49, 44);
    addUrbanBlock(838, 658, 94, 44);
    addUrbanBlock(948, 658, 129, 44);

    // Dải công viên xanh ven sông bờ Đông (Trần Hưng Đạo & Chương Dương)
    QColor parkC = darkMode ? QColor(26, 52, 34) : QColor(204, 235, 205);
    auto *eastRiverPark1 = scene->addRect(764, 160, 8, 200, QPen(Qt::NoPen), QBrush(parkC));
    eastRiverPark1->setZValue(-23);
    auto *eastRiverPark2 = scene->addRect(764, 395, 8, 205, QPen(Qt::NoPen), QBrush(parkC));
    eastRiverPark2->setZValue(-23);
    auto *chuongDuongPark = scene->addRect(756, 615, 7, 95, QPen(Qt::NoPen), QBrush(parkC));
    chuongDuongPark->setZValue(-23);

    // 4. SÔNG HÀN — Hai bờ uốn lượn theo thực tế Google Maps
    // Tạo vùng khép kín cho sông (bờ Tây x≈660-680, bờ Đông x≈760-780)
    QPainterPath river;
    river.moveTo(678, -80);
    river.cubicTo(673, -20, 668,  30, 672,  80);
    river.cubicTo(676, 130, 670, 150, 668, 170);
    river.cubicTo(664, 215, 660, 265, 658, 310);
    river.cubicTo(655, 355, 650, 395, 645, 430);
    river.cubicTo(640, 475, 644, 515, 648, 555);
    river.cubicTo(652, 590, 650, 620, 648, 660);
    river.lineTo(648, 760);
    river.lineTo(780, 760);
    river.lineTo(780, 660);
    river.cubicTo(778, 625, 776, 595, 774, 560);
    river.cubicTo(772, 520, 774, 480, 776, 440);
    river.cubicTo(778, 395, 775, 355, 770, 315);
    river.cubicTo(765, 270, 762, 225, 763, 175);
    river.cubicTo(764, 145, 762, 115, 760,  80);
    river.cubicTo(758,  35, 756, -15, 760, -80);
    river.closeSubpath();

    QColor waterColor = darkMode ? QColor(22, 39, 64) : QColor(165, 208, 222);
    auto *riverItem = scene->addPath(river, QPen(Qt::NoPen), QBrush(waterColor));
    riverItem->setZValue(-25);

    auto *riverText = scene->addText("S   Ô   N   G      H   À   N");
    QFont rf("Segoe UI", 12, QFont::Bold);
    rf.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    riverText->setFont(rf);
    riverText->setDefaultTextColor(darkMode ? QColor(48, 80, 115) : QColor(125, 172, 190));
    riverText->setPos(710, 55);
    riverText->setRotation(78);
    riverText->setZValue(-24);

    // Đảo Xanh / Euro Village ven sông phía Nam
    QColor islandColor = darkMode ? QColor(36, 48, 38) : QColor(210, 235, 210);
    QPainterPath islandPath;
    islandPath.addRoundedRect(QRectF(690, 645, 48, 75), 14, 14);
    auto *island = scene->addPath(islandPath, QPen(Qt::NoPen), QBrush(islandColor));
    island->setZValue(-23);
    auto *islandText = scene->addText("Đảo Xanh");
    QFont isf("Segoe UI", 7, QFont::Bold);
    islandText->setFont(isf);
    islandText->setDefaultTextColor(darkMode ? QColor(120, 170, 120) : QColor(40, 95, 40));
    islandText->setPos(692, 676);
    islandText->setZValue(-22);

    // 5. CÔNG VIÊN 29/3 VÀ HỒ NƯỚC CẢNH QUAN
    QColor parkColor = darkMode ? QColor(26, 52, 34) : QColor(204, 235, 205);
    QPainterPath parkPath;
    parkPath.addRoundedRect(QRectF(230, 280, 80, 130), 16, 16);
    auto *park293 = scene->addPath(parkPath, QPen(Qt::NoPen), QBrush(parkColor));
    park293->setZValue(-22);

    QPainterPath lake293;
    lake293.moveTo(270, 295);
    lake293.cubicTo(245, 325, 245, 365, 265, 395);
    lake293.cubicTo(285, 400, 295, 375, 285, 345);
    lake293.cubicTo(278, 325, 290, 305, 270, 295);
    auto *lake293Item = scene->addPath(lake293, QPen(Qt::NoPen), QBrush(waterColor));
    lake293Item->setZValue(-21);

    auto *t293 = scene->addText("Hồ Công viên 29/3");
    QFont pf("Segoe UI", 7, QFont::DemiBold);
    t293->setFont(pf);
    t293->setDefaultTextColor(darkMode ? QColor(70, 110, 140) : QColor(80, 135, 150));
    t293->setPos(235, 335);
    t293->setZValue(-20);

    // Hồ Thạc Gián & Hồ Hàm Nghi
    QPainterPath tgPath;
    tgPath.addRoundedRect(QRectF(348, 310, 28, 48), 8, 8);
    auto *lakeThacGian = scene->addPath(tgPath, QPen(Qt::NoPen), QBrush(waterColor));
    lakeThacGian->setZValue(-21);

    QPainterPath hgPath;
    hgPath.addRoundedRect(QRectF(395, 310, 28, 48), 8, 8);
    auto *lakeHamNghi = scene->addPath(hgPath, QPen(Qt::NoPen), QBrush(waterColor));
    lakeHamNghi->setZValue(-21);

    auto *tHN = scene->addText("Hồ Hàm Nghi");
    tHN->setFont(pf);
    tHN->setDefaultTextColor(darkMode ? QColor(70, 110, 140) : QColor(80, 135, 150));
    tHN->setPos(370, 358);
    tHN->setZValue(-20);

    // Công viên APEC với Mái vòm "Cánh Diều Bay Cao"
    auto *apecPark = scene->addEllipse(615, 395, 40, 35, QPen(Qt::NoPen), QBrush(parkColor));
    apecPark->setZValue(-22);

    // Mái vòm uốn lượn trắng APEC
    QPainterPath kiteDome;
    kiteDome.moveTo(622, 412);
    kiteDome.cubicTo(630, 398, 642, 398, 650, 412);
    kiteDome.cubicTo(642, 420, 630, 420, 622, 412);
    auto *domeItem = scene->addPath(kiteDome, QPen(QColor(180, 180, 180), 1), QBrush(Qt::white));
    domeItem->setZValue(-21);

    // 6. CẢNG HÀNG KHÔNG QUỐC TẾ ĐÀ NẴNG (SÂN BAY CHI TIẾT)
    QColor airportBg = darkMode ? QColor(36, 38, 48) : QColor(228, 224, 216);
    auto *airportArea = scene->addRect(20, 390, 190, 290,
                                       QPen(QColor(180, 175, 165), 1),
                                       QBrush(airportBg));
    airportArea->setZValue(-20);

    // Sân đỗ máy bay (Apron)
    QColor apronColor = darkMode ? QColor(48, 50, 58) : QColor(210, 206, 198);
    auto *apron = scene->addRect(125, 435, 75, 195, QPen(Qt::NoPen), QBrush(apronColor));
    apron->setZValue(-19);

    // Đường băng 35R/17L
    auto *runway1 = scene->addRect(85, 410, 20, 250,
                                   QPen(Qt::NoPen),
                                   QBrush(darkMode ? QColor(60, 62, 70) : QColor(85, 90, 98)));
    runway1->setZValue(-19);
    auto *rLine1 = scene->addLine(95, 418, 95, 652, QPen(Qt::white, 1.5, Qt::DashLine));
    rLine1->setZValue(-18);

    // Số hiệu đường băng
    auto *rNum1 = scene->addText("35R");
    rNum1->setFont(QFont("Segoe UI", 6, QFont::Bold));
    rNum1->setDefaultTextColor(Qt::white);
    rNum1->setPos(87, 412);
    rNum1->setZValue(-17);

    // Đường băng 35L/17R
    auto *runway2 = scene->addRect(50, 430, 18, 210,
                                   QPen(Qt::NoPen),
                                   QBrush(darkMode ? QColor(55, 57, 65) : QColor(95, 100, 108)));
    runway2->setZValue(-19);
    auto *rLine2 = scene->addLine(59, 436, 59, 634, QPen(Qt::white, 1.5, Qt::DashLine));
    rLine2->setZValue(-18);

    // Các đường lăn taxiways
    QPen taxiPen(darkMode ? QColor(70, 72, 80) : QColor(130, 135, 142), 5, Qt::SolidLine, Qt::RoundCap);
    auto *taxi1 = scene->addLine(95, 475, 135, 475, taxiPen);
    taxi1->setZValue(-18);
    auto *taxi2 = scene->addLine(95, 545, 135, 545, taxiPen);
    taxi2->setZValue(-18);

    // Máy bay đỗ ở sân đỗ (Apron)
    auto *plane1 = scene->addText("✈️");
    plane1->setFont(QFont("Segoe UI", 11));
    plane1->setPos(140, 450);
    plane1->setZValue(-17);

    auto *plane2 = scene->addText("✈️");
    plane2->setFont(QFont("Segoe UI", 11));
    plane2->setPos(140, 520);
    plane2->setZValue(-17);

    // Nhà ga hành khách T1 (Quốc nội) & T2 (Quốc tế)
    QPainterPath termPath;
    termPath.addRoundedRect(QRectF(145, 580, 55, 50), 6, 6);
    auto *terminal = scene->addPath(termPath,
                                    QPen(QColor(150, 145, 135), 1),
                                    QBrush(darkMode ? QColor(50, 52, 60) : QColor(245, 245, 250)));
    terminal->setZValue(-18);

    auto *apText = scene->addText("Cảng hàng không QT Đà Nẵng\n(Ga T1 & T2)");
    QFont apf("Segoe UI", 8, QFont::Bold);
    apText->setFont(apf);
    apText->setDefaultTextColor(darkMode ? QColor(160, 165, 180) : QColor(100, 105, 115));
    apText->setPos(40, 665);
    apText->setZValue(-17);

    // 7. HỆ THỐNG MẶT ĐƯỜNG PHÂN CẤP CHUẨN GOOGLE MAPS (THẲNG VÀ BÁM SÁT MẠNG LƯỚI ĐÔ THỊ)
    // Lớp 1: Casing (Viền ngoài mặt đường)
    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u < e.to) {
          int outW = e.isMajor ? 16 : 10;
          auto *line = scene->addLine(QLineF(graph.nodes[u].pos, graph.nodes[e.to].pos),
              QPen(roadColor, outW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          line->setZValue(-10);
        }
      }
    }

    // Lớp 2: Fill (Đại lộ màu vàng Google Maps, đường nội bộ màu trắng)
    QColor majorFill = darkMode ? QColor(80, 72, 56) : QColor(254, 225, 153);
    QColor minorFill = darkMode ? QColor(48, 47, 44) : QColor(255, 255, 255);

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u < e.to) {
          int inW = e.isMajor ? 11 : 6;
          QColor fillC = e.isMajor ? majorFill : minorFill;
          auto *line = scene->addLine(QLineF(graph.nodes[u].pos, graph.nodes[e.to].pos),
              QPen(fillC, inW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          line->setZValue(-9);
        }
      }
    }

    // Lớp 3: Traffic Overlay (Lớp màu sắc mật độ giao thông phủ trên mặt đường toàn thành phố)
    trafficLinesGroup = scene->createItemGroup({});
    trafficOverlayLines.clear();

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u < e.to) {
          int lineW = e.isMajor ? 4 : 3;
          QColor c = trafficToColor(e.trafficFactor);
          auto *line = scene->addLine(QLineF(graph.nodes[u].pos, graph.nodes[e.to].pos),
              QPen(c, lineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          line->setZValue(-8);
          trafficLinesGroup->addToGroup(line);
          trafficOverlayLines.append({line, u, e.to, e.isMajor});
        }
      }
    }
    trafficLinesGroup->setVisible(showTrafficLayer);

    // 8. CÁC CÂY CẦU NGHỆ THUẬT DANH TIẾNG ĐÀ NẴNG
    // 8.A. CẦU THUẬN PHƯỚC (CẦU TREO DÂY VÕNG LỚN NHẤT VIỆT NAM Ở CỬA SÔNG HÀN)
    // 2 Trụ tháp cầu treo màu bạc
    QPen towerPen(darkMode ? QColor(180, 185, 200) : QColor(140, 150, 165), 3.5, Qt::SolidLine, Qt::SquareCap);
    auto *tower1 = scene->addLine(685, -16, 685, 18, towerPen);
    tower1->setZValue(-3);
    auto *tower2 = scene->addLine(748, -16, 748, 18, towerPen);
    tower2->setZValue(-3);

    // 2 Cáp võng chính parabol uốn lượn qua đỉnh tháp
    QPainterPath cable1;
    cable1.moveTo(660, 8);
    cable1.quadTo(685, -16, 716, 2);
    cable1.quadTo(748, -16, 770, 8);
    auto *cItem1 = scene->addPath(cable1, QPen(QColor(100, 181, 246), 2.2, Qt::SolidLine, Qt::RoundCap));
    cItem1->setZValue(-2);

    // Dây treo thẳng đứng
    for (int sx = 692; sx <= 742; sx += 8) {
      double t = (sx - 685.0) / (748.0 - 685.0);
      double cy = -16.0 + 4.0 * 18.0 * (t - 0.5) * (t - 0.5);
      auto *hanger = scene->addLine(sx, cy, sx, 8, QPen(QColor(140, 150, 165, 160), 1));
      hanger->setZValue(-3);
    }

    // 8.B. CẦU SÔNG HÀN (CẦU QUAY ĐẦU TIÊN TẠI VIỆT NAM)
    // Trụ xoay tròn ở chính giữa lòng sông
    auto *pivotIsland = scene->addEllipse(711, 138, 14, 14,
                                          QPen(darkMode ? QColor(120, 130, 145) : QColor(180, 185, 195), 1.5),
                                          QBrush(darkMode ? QColor(70, 75, 88) : QColor(220, 225, 232)));
    pivotIsland->setZValue(-3);

    // Giàn thép chịu lực vòm 2 bên trụ xoay
    QPainterPath swingArch1, swingArch2;
    swingArch1.moveTo(675, 145);
    swingArch1.quadTo(696, 130, 718, 145);
    swingArch2.moveTo(718, 145);
    swingArch2.quadTo(740, 130, 762, 145);
    auto *archItem1 = scene->addPath(swingArch1, QPen(QColor(66, 133, 244), 3, Qt::SolidLine, Qt::RoundCap));
    auto *archItem2 = scene->addPath(swingArch2, QPen(QColor(66, 133, 244), 3, Qt::SolidLine, Qt::RoundCap));
    archItem1->setZValue(-2);
    archItem2->setZValue(-2);

    // 8.C. CẦU RỒNG (BIỂU TƯỢNG VÀNG RỰC RỠ CỦA ĐÀ NẴNG)
    // Thân rồng vàng 3 nhịp vòm thép uốn sóng uyển chuyển
    QPainterPath dragonBody;
    dragonBody.moveTo(655, 375);
    dragonBody.quadTo(685, 342, 715, 375);
    dragonBody.quadTo(745, 345, 775, 375);
    auto *dragon = scene->addPath(dragonBody, QPen(QColor(255, 179, 0), 5.5, Qt::SolidLine, Qt::RoundCap));
    dragon->setZValue(-2);

    // Đầu rồng vươn cao ở mố phía Đông hướng ra Biển Đông
    QPainterPath dragonHead;
    dragonHead.moveTo(775, 375);
    dragonHead.lineTo(788, 366);
    dragonHead.lineTo(796, 368);
    dragonHead.lineTo(790, 376);
    dragonHead.closeSubpath();
    auto *headItem = scene->addPath(dragonHead, QPen(Qt::NoPen), QBrush(QColor(255, 179, 0)));
    headItem->setZValue(-1);

    // Vệt hào quang / lửa đầu rồng
    auto *dragonGlow = scene->addEllipse(790, 365, 8, 8, QPen(Qt::NoPen), QBrush(QColor(255, 87, 34, 180)));
    dragonGlow->setZValue(-1);

    // Đuôi rồng hình hoa sen cách điệu ở mố phía Tây
    QPainterPath dragonTail;
    dragonTail.moveTo(655, 375);
    dragonTail.lineTo(646, 368);
    dragonTail.lineTo(644, 375);
    dragonTail.lineTo(646, 382);
    dragonTail.closeSubpath();
    auto *tailItem = scene->addPath(dragonTail, QPen(Qt::NoPen), QBrush(QColor(255, 179, 0)));
    tailItem->setZValue(-1);

    // 8.D. CẦU TRẦN THỊ LÝ (TRỤ THÁP NGHIÊNG CÁNH BUỒM DÂY VĂNG)
    // Trụ tháp nghiêng chữ Y ngược cao vút nghiêng 12 độ
    QPen pylonPen(QColor(244, 81, 30), 4.5, Qt::SolidLine, Qt::RoundCap);
    auto *pylon = scene->addLine(710, 610, 712, 565, pylonPen);
    pylon->setZValue(-2);

    // Chùm dây văng nan quạt xòe như cánh buồm hướng ra biển
    QPen fanPen(QColor(255, 112, 67, 180), 1.2);
    for (int bx = 665; bx <= 755; bx += 10) {
      if (std::abs(bx - 710) > 6) {
        auto *fanLine = scene->addLine(712, 567, bx, 610, fanPen);
        fanLine->setZValue(-3);
      }
    }

    // 8.E. BẾN DU THUYỀN DHC MARINA, CẦU TÌNH YÊU & TƯỢNG CÁ CHÉP HOÁ RỒNG
    QPainterPath pierPath;
    pierPath.addRoundedRect(QRectF(744, 342, 28, 18), 3, 3);
    auto *pierItem = scene->addPath(pierPath, QPen(QColor(160, 140, 120), 1),
                                    QBrush(darkMode ? QColor(60, 55, 50) : QColor(230, 215, 200)));
    pierItem->setZValue(-2);

    for (int hx = 747; hx <= 766; hx += 7) {
      auto *heart = scene->addText("❤️");
      heart->setFont(QFont("Segoe UI", 6));
      heart->setPos(hx, 339);
      heart->setZValue(-1);
    }
    auto *carpText = scene->addText("⛲");
    carpText->setFont(QFont("Segoe UI", 9));
    carpText->setPos(736, 343);
    carpText->setZValue(-1);

    // 8.F. CÔNG VIÊN BIỂN ĐÔNG (EAST SEA PARK) VÀ QUẢNG TRƯỜNG HÒA BÌNH
    QPainterPath eastSeaPark;
    eastSeaPark.addRoundedRect(QRectF(1048, 112, 34, 32), 6, 6);
    auto *espItem = scene->addPath(eastSeaPark, QPen(Qt::NoPen), QBrush(parkC));
    espItem->setZValue(-22);

    auto *dove1 = scene->addText("🕊️");
    dove1->setFont(QFont("Segoe UI", 8));
    dove1->setPos(1054, 116);
    dove1->setZValue(-21);

    // 8.G. RẶNG DỪA & CHÒI NGHỈ VEN BÃI BIỂN MỸ KHÊ
    for (int py = 90; py <= 690; py += 75) {
      auto *palm = scene->addText("🌴");
      palm->setFont(QFont("Segoe UI", 8));
      palm->setPos(1074, py);
      palm->setZValue(-26);
    }
    for (int uy = 130; uy <= 670; uy += 90) {
      auto *umb = scene->addText("⛱️");
      umb->setFont(QFont("Segoe UI", 7));
      umb->setPos(1088, uy);
      umb->setZValue(-26);
    }

    // 9. NÚT GIAO LỘ (JUNCTION DOTS)
    for (const Node &n : graph.nodes) {
      QColor dotC = darkMode ? QColor(105, 100, 92) : QColor(185, 178, 165);
      auto *dot = scene->addEllipse(n.pos.x() - 2.5, n.pos.y() - 2.5, 5, 5,
                                    QPen(Qt::NoPen), QBrush(dotC));
      dot->setZValue(4);
    }

    // 10. ĐỊA DANH & POI NỔI BẬT
    poiGroup = scene->createItemGroup({});

    struct LandmarkItem {
      QString name;
      QString icon;
      QPointF pos;
      QColor badgeBg;
      QPointF offset;
    };
    QVector<LandmarkItem> pois = {
      // Bờ Tây & Trung tâm
      {"Cảng hàng không quốc tế Đà Nẵng", "✈️", QPointF(90, 520), QColor(26, 115, 232), QPointF(-40, 14)},
      {"Ga Đà Nẵng", "🚆", QPointF(370, 180), QColor(0, 137, 123), QPointF(10, -7)},
      {"Hồ Công viên 29/3", "🌳", QPointF(265, 340), QColor(46, 125, 50), QPointF(10, -7)},
      {"Hồ Hàm Nghi", "💧", QPointF(385, 330), QColor(3, 155, 229), QPointF(8, -7)},
      {"AEON MALL Đà Nẵng Thanh Khê", "🛍️", QPointF(175, 240), QColor(230, 81, 0), QPointF(10, -10)},
      {"Chợ Cồn", "🛍️", QPointF(450, 275), QColor(230, 81, 0), QPointF(10, -7)},
      {"Chợ Hàn", "🛍️", QPointF(610, 275), QColor(230, 81, 0), QPointF(10, -7)},
      {"Giáo Xứ Chính Toà Đà Nẵng", "⛪", QPointF(610, 320), QColor(94, 53, 177), QPointF(10, -8)},
      {"Công Viên APEC", "🌳", QPointF(625, 410), QColor(46, 125, 50), QPointF(10, 8)},
      {"Cầu Thuận Phước", "🌉", QPointF(716, 8), QColor(30, 136, 229), QPointF(-36, -20)},
      {"Cầu Sông Hàn", "🌉", QPointF(718, 145), QColor(245, 124, 0), QPointF(-32, -18)},
      {"Cầu Rồng", "🐉", QPointF(715, 375), QColor(245, 124, 0), QPointF(-26, -18)},
      {"Cầu Trần Thị Lý", "🌉", QPointF(710, 610), QColor(244, 81, 30), QPointF(-36, -18)},

      // Bờ Đông (Sơn Trà & Ngũ Hành Sơn - Nhà hàng, Khách sạn, Du lịch)
      {"Vincom Plaza Đà Nẵng", "🛍️", QPointF(800, 140), QColor(230, 81, 0), QPointF(10, -18)},
      {"Pizza 4P's Indochina", "🍕", QPointF(795, 172), QColor(230, 81, 0), QPointF(10, -7)},
      {"Herbal Spa", "🌿", QPointF(855, 270), QColor(46, 125, 50), QPointF(10, -7)},
      {"Tượng Cá Chép Hóa Rồng & Cầu Tình Yêu", "⛲", QPointF(758, 350), QColor(233, 30, 99), QPointF(-140, -7)},
      {"Chợ Đêm Sơn Trà", "🏮", QPointF(795, 430), QColor(230, 81, 0), QPointF(10, -7)},
      {"Mộc Quán Seafood Đà Nẵng", "🦀", QPointF(980, 365), QColor(230, 81, 0), QPointF(-110, -18)},
      {"Cửa Ngõ Café", "☕", QPointF(955, 395), QColor(141, 110, 99), QPointF(10, -7)},
      {"Bếp Cuốn Đà Nẵng - Michelin", "🥢", QPointF(920, 435), QColor(198, 40, 40), QPointF(10, -7)},
      {"B&B Bakery - Đà Nẵng", "🥐", QPointF(920, 475), QColor(239, 108, 0), QPointF(10, -7)},
      {"Khách sạn Mường Thanh Luxury", "🏨", QPointF(1070, 565), QColor(94, 53, 177), QPointF(-140, -16)},
      {"Four Points by Sheraton", "🏨", QPointF(1070, 80), QColor(94, 53, 177), QPointF(-115, -7)},
      {"Bikini Bottom Express", "🍹", QPointF(1070, 650), QColor(0, 151, 167), QPointF(-105, 8)},
      {"Maia Beach Bar", "🏖️", QPointF(1105, 610), QColor(0, 151, 167), QPointF(10, -7)},
      {"Công Viên Biển Đông", "🕊️", QPointF(1065, 130), QColor(46, 125, 50), QPointF(-110, -7)},
      {"Bãi tắm Phạm Văn Đồng", "🏖️", QPointF(1085, 155), QColor(0, 151, 167), QPointF(10, -7)},
      {"Bãi Biển Mỹ Khê", "🏖️", QPointF(1085, 380), QColor(0, 151, 167), QPointF(10, -7)}
    };
    for (const auto &pi : pois) {
      auto *badge = scene->addEllipse(pi.pos.x() - 6.5, pi.pos.y() - 6.5, 13, 13,
                                      QPen(Qt::white, 1.5), QBrush(pi.badgeBg));
      badge->setZValue(12);
      poiGroup->addToGroup(badge);

      auto *t = scene->addText(pi.icon + " " + pi.name);
      QFont pf("Segoe UI", 8, QFont::DemiBold);
      t->setFont(pf);
      t->setPos(pi.pos + pi.offset);
      t->setDefaultTextColor(darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36));
      t->setZValue(13);
      poiGroup->addToGroup(t);
    }
    poiGroup->setVisible(showLandmarksLayer);

    // 11. TÊN ĐƯỜNG PHỐ (STREET LABELS)
    streetLabelGroup = scene->createItemGroup({});

    struct StreetLabel {
      QString name;
      QPointF pos;
      double rotation;
      bool isMajor;
    };
    QVector<StreetLabel> streetLabels = {
      // Bờ Tây & Trung tâm
      {"Nguyễn Tất Thành", QPointF(130, 40), -8.0, true},
      {"Nguyễn Tất Thành", QPointF(290, 20), -6.0, true},
      {"Trần Cao Vân", QPointF(65, 126), 0.0, false},
      {"Trần Cao Vân", QPointF(320, 118), 0.0, false},
      {"Hải Phòng", QPointF(420, 166), 0.0, false},
      {"Điện Biên Phủ", QPointF(70, 252), 0.0, true},
      {"Lê Duy Đình", QPointF(175, 338), 0.0, false},
      {"Nguyễn Tri Phương", QPointF(280, 375), 90.0, true},
      {"Phan Thanh", QPointF(350, 400), 90.0, false},
      {"Hàm Nghi", QPointF(400, 400), 90.0, false},
      {"Lê Đình Lý", QPointF(440, 520), 90.0, false},
      {"Nguyễn Hữu Thọ", QPointF(330, 570), 90.0, true},
      {"Duy Tân", QPointF(270, 602), 0.0, true},
      {"Duy Tân", QPointF(415, 602), 0.0, true},
      {"Ông Ích Khiêm", QPointF(440, 95), 90.0, false},
      {"Hùng Vương", QPointF(335, 267), 0.0, true},
      {"Ng. Gia Tự", QPointF(480, 245), 90.0, false},
      {"Triệu Nữ Vương", QPointF(460, 390), 90.0, false},
      {"Hoàng Diệu", QPointF(480, 490), 90.0, true},
      {"Hoàng Văn Thụ", QPointF(520, 357), 0.0, false},
      {"Lê Lợi", QPointF(520, 130), 90.0, false},
      {"Phan Châu Trinh", QPointF(520, 310), 90.0, false},
      {"Phan Châu Trinh", QPointF(522, 490), 90.0, false},
      {"Lê Duẩn", QPointF(380, 212), 0.0, true},
      {"Trần Phú", QPointF(600, 210), 90.0, true},
      {"Trần Phú", QPointF(600, 335), 90.0, true},
      {"Bạch Đằng", QPointF(655, 210), 90.0, true},
      {"Bạch Đằng", QPointF(655, 335), 90.0, true},
      {"Nguyễn Văn Linh", QPointF(380, 432), 0.0, true},
      {"Đ. 2 Tháng 9", QPointF(615, 530), 90.0, true},
      {"Trần Thị Lý", QPointF(560, 602), 0.0, true},
      {"Quang Trung", QPointF(560, 72), 0.0, false},
      {"Xô Viết Nghệ Tĩnh", QPointF(350, 235), 90.0, true},
      {"Nguyễn Hữu Dật", QPointF(295, 252), 90.0, false},
      {"Lê Độ", QPointF(410, 135), 90.0, false},
      {"Nguyễn Tất Thành", QPointF(435, 8), -5.0, true},
      {"Như Nguyệt", QPointF(645, 5), 0.0, false},
      {"Hà Huy Tập", QPointF(50, 325), 90.0, false},

      // Bờ Đông (Sơn Trà & Ngũ Hành Sơn - Mạng lưới hoàn chỉnh)
      {"Đ. Trần Hưng Đạo", QPointF(755, 230), 90.0, true},
      {"Đ. Trần Hưng Đạo", QPointF(755, 480), 90.0, true},
      {"Ngô Quyền", QPointF(815, 95), 90.0, true},
      {"Ngô Quyền (AH17)", QPointF(815, 330), 90.0, true},
      {"Ngũ Hành Sơn", QPointF(815, 660), 90.0, true},
      {"Phạm Văn Đồng", QPointF(885, 138), 0.0, true},
      {"Morrison", QPointF(920, 202), 0.0, false},
      {"An Hải Bắc", QPointF(950, 262), 0.0, false},
      {"Nguyễn Công Trứ", QPointF(840, 322), 0.0, false},
      {"Nguyễn Công Trứ", QPointF(1020, 322), 0.0, false},
      {"Võ Văn Kiệt", QPointF(835, 372), 0.0, true},
      {"Võ Văn Kiệt", QPointF(980, 372), 0.0, true},
      {"Phạm Cự Lượng", QPointF(876, 235), 90.0, false},
      {"Phạm Cự Lượng", QPointF(876, 490), 90.0, false},
      {"Nguyễn Thiện Kế", QPointF(926, 350), 90.0, false},
      {"Nguyễn Thiện Kế", QPointF(926, 490), 90.0, false},
      {"Chính Hữu", QPointF(961, 235), 90.0, false},
      {"Chính Hữu", QPointF(961, 350), 90.0, false},
      {"Hồ Nghinh", QPointF(1001, 235), 90.0, false},
      {"Hồ Nghinh", QPointF(1001, 350), 90.0, false},
      {"Hà Bổng", QPointF(1036, 420), 90.0, false},
      {"Hà Nghinh", QPointF(1036, 235), 90.0, false},
      {"Lê Hữu Trác", QPointF(870, 452), 0.0, false},
      {"Nguyễn Duy Hiệu", QPointF(860, 502), 0.0, false},
      {"Nguyễn Văn Thoại", QPointF(850, 557), 0.0, true},
      {"Trần Thị Lý", QPointF(780, 602), 0.0, true},
      {"Chương Dương", QPointF(748, 660), 90.0, false},
      {"Châu Thị Vĩnh Tế", QPointF(926, 620), 90.0, false},
      {"Đỗ Bá", QPointF(860, 642), 0.0, false},
      {"Dương Khuê", QPointF(790, 702), 0.0, false},
      {"Dương Khuê", QPointF(990, 702), 0.0, false},
      {"Võ Nguyên Giáp", QPointF(1073, 235), 90.0, true},
      {"Võ Nguyên Giáp", QPointF(1073, 490), 90.0, true},
      {"Võ Nguyên Giáp", QPointF(1073, 670), 90.0, true}
    };
    for (const auto &sl : streetLabels) {
      auto *st = scene->addText(sl.name);
      QFont sf("Segoe UI", sl.isMajor ? 8 : 7, sl.isMajor ? QFont::Bold : QFont::DemiBold);
      st->setFont(sf);
      st->setDefaultTextColor(sl.isMajor
                                  ? (darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36))
                                  : (darkMode ? QColor(165, 170, 180) : QColor(95, 99, 104)));
      st->setPos(sl.pos);
      st->setRotation(sl.rotation);
      st->setZValue(7);
      streetLabelGroup->addToGroup(st);
    }

    // Biển số hiệu Quốc lộ AH17 (Highway Shield badges)
    auto drawShieldAH17 = [&](double sx, double sy) {
      QPainterPath sp;
      sp.addRoundedRect(QRectF(sx - 13, sy - 7, 26, 14), 3, 3);
      auto *bg = scene->addPath(sp,
                                QPen(darkMode ? QColor(80, 85, 95) : QColor(90, 95, 105), 1),
                                QBrush(darkMode ? QColor(40, 44, 52) : Qt::white));
      bg->setZValue(6);
      auto *t = scene->addText("AH17");
      QFont hf("Segoe UI", 6, QFont::Bold);
      t->setFont(hf);
      t->setDefaultTextColor(darkMode ? QColor(220, 225, 235) : QColor(32, 33, 36));
      t->setPos(sx - 12, sy - 9);
      t->setZValue(7);
      streetLabelGroup->addToGroup(bg);
      streetLabelGroup->addToGroup(t);
    };
    drawShieldAH17(830, 240);
    drawShieldAH17(830, 480);

    streetLabelGroup->setVisible(showStreetLabelsLayer);

    // 12. TÊN PHƯỜNG & KHU VỰC
    struct DistrictLabel {
      QString name;
      QPointF pos;
    };
    QVector<DistrictLabel> distLabels = {
      {"THANH KHÊ", QPointF(35, 235)},
      {"PHÚ GIA", QPointF(350, 90)},
      {"AN HẢI", QPointF(710, 315)},
      {"AN HẢI BẮC", QPointF(920, 115)},
      {"HÒA CƯƠNG", QPointF(475, 665)},
      {"BẮC MỸ PHÚ", QPointF(855, 665)},
      {"KHU NHÀ HÀNG VEN BIỂN", QPointF(1005, 335)},
      {"BÃI BIỂN MỸ KHÊ", QPointF(1020, 490)}
    };
    for (const auto &dl : distLabels) {
      auto *dt = scene->addText(dl.name);
      QFont df("Segoe UI", 8, QFont::DemiBold);
      df.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
      dt->setFont(df);
      dt->setDefaultTextColor(darkMode ? QColor(135, 140, 155) : QColor(120, 125, 135));
      dt->setPos(dl.pos);
      dt->setZValue(2);
    }

    // 13. GRID TỌA ĐỘ
    gridGroup = scene->createItemGroup({});
    QColor gridC = darkMode ? QColor(48, 48, 58) : QColor(215, 209, 198);
    QPen gridPen(gridC, 0.5);
    for (int x = -40; x < 1280; x += 60)
      gridGroup->addToGroup(scene->addLine(x, -80, x, 780, gridPen));
    for (int y = -80; y < 780; y += 60)
      gridGroup->addToGroup(scene->addLine(-40, y, 1280, y, gridPen));
    gridGroup->setZValue(-35);
    gridGroup->setVisible(gridVisible);

    // 14. BẢNG CHÚ THÍCH MẬT ĐỘ (LEGEND)
    legendGroup = scene->createItemGroup({});
    QColor legendBg  = darkMode ? QColor(32, 34, 48, 230) : QColor(255, 255, 255, 230);
    QColor legendBdr = darkMode ? QColor(68, 70, 90)       : QColor(218, 220, 224);
    auto *lRect = scene->addRect(14, 68, 215, 78, QPen(legendBdr, 1), QBrush(legendBg));
    lRect->setZValue(49);
    legendGroup->addToGroup(lRect);

    struct LI { QString label; QColor color; };
    QVector<LI> legendItems = {
      {"● Xanh: Thông thoáng (≤1.2x)",  QColor(15, 157, 88)},
      {"● Vàng/Cam: Đông vừa (≤1.9x)",  QColor(251, 188, 4)},
      {"● Đỏ: Ùn tắc (≥2.0x)",         QColor(234, 67, 53)},
    };
    int ly = 73;
    for (const auto &li : legendItems) {
      auto *t = scene->addText(li.label);
      QFont lf("Segoe UI", 8, QFont::DemiBold);
      t->setFont(lf);
      t->setPos(18, ly);
      t->setDefaultTextColor(li.color);
      t->setZValue(50);
      legendGroup->addToGroup(t);
      ly += 21;
    }
    legendGroup->setVisible(false);

    scene->setSceneRect(-40, -80, 1320, 860);
  }

  void applyTheme() {
    if (darkMode) {
      setBackgroundBrush(QColor(24, 26, 32));
      roadColor = QColor(64, 60, 54);
      textColor = QColor(218, 220, 225);
    } else {
      setBackgroundBrush(QColor(242, 239, 233));
      roadColor = QColor(196, 190, 176);
      textColor = QColor(60, 64, 67);
    }

    updateFloatingControlTheme();
    updateFloatingSearchBarTheme();
    updateFloatingCategoryBarTheme();
    updateFloatingRouteBannerTheme();
    updateFloatingTrafficTheme();

    stopAnimation();
    stopPathAnim();

    pathItems.clear();
    markerItems.clear();
    altPathItems.clear();
    secondaryPathItems.clear();
    pathAnimLines.clear();
    gridGroup = nullptr;
    poiGroup = nullptr;
    streetLabelGroup = nullptr;
    legendGroup = nullptr;
    trafficLinesGroup = nullptr;
    trafficOverlayLines.clear();

    scene->clear();
    drawMap();

    if (!startPoint.isNull() || !endPoint.isNull())
      redrawMarkers();
    if (!lastSteps.isEmpty() || !timeSteps.isEmpty() || !distSteps.isEmpty())
      recompute();

    viewport()->update();
    emit statusMessage(darkMode ? "Chế độ ban đêm (Dark Mode) bật" : "Chế độ ban ngày (Light Mode) bật");
  }

  void addMarker(QPointF pos, const QColor &color, const QString &label) {
    const double r = 11.0;
    QPainterPath pin;
    pin.addEllipse(QRectF(-r, -r * 2.6, r * 2.0, r * 2.0));
    pin.moveTo(-r * 0.5, -r * 0.65);
    pin.lineTo(r * 0.5, -r * 0.65);
    pin.lineTo(0.0, 0.0);
    pin.closeSubpath();

    auto *pinItem = scene->addPath(pin, QPen(color.darker(160), 1.5), QBrush(color));
    pinItem->setPos(pos);
    pinItem->setZValue(30);
    markerItems.append(pinItem);

    const double dotR = r * 0.38;
    auto *inner = scene->addEllipse(
        pos.x() - dotR, pos.y() - r * 1.6 - dotR,
        dotR * 2.0, dotR * 2.0,
        QPen(Qt::NoPen), QBrush(Qt::white));
    inner->setZValue(31);
    markerItems.append(inner);

    auto *t = scene->addSimpleText(label);
    QFont lf("Segoe UI", 10, QFont::Bold);
    t->setFont(lf);
    t->setBrush(color.darker(140));
    t->setPos(pos.x() - 4, pos.y() - r * 2.6 - 14);
    t->setZValue(32);
    markerItems.append(t);
  }

  void clearMarkers() {
    for (auto *i : markerItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    markerItems.clear();
  }

  void redrawMarkers() {
    if (!startPoint.isNull())
      addMarker(startPoint, colorA, "A");
    if (!endPoint.isNull())
      addMarker(endPoint, colorB, "B");
  }

  void clearPath() {
    stopPathAnim();
    stopAnimation();

    for (auto *i : pathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    pathItems.clear();

    for (auto *i : altPathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    altPathItems.clear();

    for (auto *i : secondaryPathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    secondaryPathItems.clear();
    pathAnimLines.clear();
  }

  int findNearestNode(QPointF p, double maxDist) const {
    int best = -1;
    double bestD = maxDist;
    for (const Node &n : graph.nodes) {
      double d = QLineF(p, n.pos).length();
      if (d < bestD) {
        bestD = d;
        best = n.id;
      }
    }
    return best;
  }

  std::tuple<int, int, QPointF> findNearestPointOnEdge(QPointF click, double maxDist = 14.0) {
    double bestDist = maxDist;
    int bestU = -1, bestV = -1;
    QPointF bestProj;

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u >= e.to)
          continue;
        QPointF A = graph.nodes[u].pos, B = graph.nodes[e.to].pos;
        QPointF AP = click - A, AB = B - A;
        double ab2 = QPointF::dotProduct(AB, AB);
        if (ab2 < 1e-6)
          continue;
        double t = std::clamp(QPointF::dotProduct(AP, AB) / ab2, 0.0, 1.0);
        QPointF proj = A + t * AB;
        double d = QLineF(click, proj).length();
        if (d < bestDist) {
          bestDist = d;
          bestU = u;
          bestV = e.to;
          bestProj = proj;
        }
      }
    }
    return {bestU, bestV, bestProj};
  }

  double trafficOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return 1.0;
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.trafficFactor;
    return 1.0;
  }

  QString streetOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return "";
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.streetName;
    return "";
  }

  bool isBridgeOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return false;
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.isBridge;
    return false;
  }

  void setEndpointFromNode(int nid) {
    if (startPoint.isNull()) {
      startPoint = graph.nodes[nid].pos;
      startEdgeU = nid;
      startEdgeV = -1;
      clearPath();
      clearMarkers();
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(nid, -1);
    } else if (endPoint.isNull()) {
      endPoint = graph.nodes[nid].pos;
      endEdgeU = nid;
      endEdgeV = -1;
      addMarker(endPoint, colorB, "B");
      recompute();
      emit endpointsChanged(startEdgeU, nid);
    } else {
      clearPath();
      clearMarkers();
      stopAnimation();
      stopPathAnim();
      startPoint = graph.nodes[nid].pos;
      startEdgeU = nid;
      startEdgeV = -1;
      endPoint = QPointF();
      endEdgeU = endEdgeV = -1;
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(nid, -1);
    }
    emit routeChanged();
  }

  void setEndpointFromEdge(int edgeU, int edgeV, QPointF projected) {
    if (startPoint.isNull()) {
      startPoint = projected;
      startEdgeU = edgeU;
      startEdgeV = edgeV;
      clearPath();
      clearMarkers();
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(edgeU, -1);
    } else if (endPoint.isNull()) {
      endPoint = projected;
      endEdgeU = edgeU;
      endEdgeV = edgeV;
      addMarker(endPoint, colorB, "B");
      recompute();
      emit endpointsChanged(startEdgeU, edgeU);
    } else {
      clearPath();
      clearMarkers();
      stopAnimation();
      stopPathAnim();
      startPoint = projected;
      startEdgeU = edgeU;
      startEdgeV = edgeV;
      endPoint = QPointF();
      endEdgeU = endEdgeV = -1;
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(edgeU, -1);
    }
    emit routeChanged();
  }

  void handleRightClick(QMouseEvent *e) {
    QPointF sp = mapToScene(e->pos());
    int nearestNode = findNearestNode(sp, 20.0);
    auto [u, v, proj] = findNearestPointOnEdge(sp, 14.0);

    QMenu menu(this);
    menu.setStyleSheet(darkMode ?
      "QMenu { background: #242630; color: #E8EAED; border: 1px solid #5F6368; border-radius: 8px; padding: 4px; }"
      "QMenu::item { padding: 6px 20px 6px 10px; border-radius: 4px; font-size: 12px; }"
      "QMenu::item:selected { background: #3C4043; color: #8AB4F8; }"
      "QMenu::separator { height: 1px; background: #4D5156; margin: 4px 6px; }"
      :
      "QMenu { background: #FFFFFF; color: #202124; border: 1px solid #DADCE0; border-radius: 8px; padding: 4px; }"
      "QMenu::item { padding: 6px 20px 6px 10px; border-radius: 4px; font-size: 12px; }"
      "QMenu::item:selected { background: #E8F0FE; color: #1A73E8; }"
      "QMenu::separator { height: 1px; background: #E8EAED; margin: 4px 6px; }"
    );

    // TH1: Click chuột phải trúng Nút giao / Địa điểm
    if (nearestNode >= 0) {
      const Node &n = graph.nodes[nearestNode];
      menu.addSection(QString("%1 %2").arg(n.icon, n.name));
      QAction *actSetA = menu.addAction("📍 Chọn làm điểm xuất phát (A)");
      QAction *actSetB = menu.addAction("🏁 Chọn làm điểm đến (B)");
      menu.addSeparator();

      bool isBanned = bannedNodes.contains(nearestNode);
      QAction *actBan = menu.addAction(isBanned ? "✅ Bỏ cấm nút giao này" : "🚫 Cấm đi qua nút giao này");

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      QAction *chosen = menu.exec(e->globalPosition().toPoint());
#else
      QAction *chosen = menu.exec(e->globalPos());
#endif
      if (chosen == actSetA) {
        setEndpointFromNode(nearestNode);
      } else if (chosen == actSetB) {
        if (startPoint.isNull()) {
          setEndpointFromNode(nearestNode);
        } else {
          endPoint = n.pos;
          endEdgeU = nearestNode;
          endEdgeV = -1;
          addMarker(endPoint, colorB, "B");
          recompute();
          emit endpointsChanged(startEdgeU, nearestNode);
          emit routeChanged();
        }
      } else if (chosen == actBan) {
        if (isBanned) bannedNodes.remove(nearestNode);
        else bannedNodes.insert(nearestNode);
        recompute();
      }
      return;
    }

    // TH2: Click chuột phải trúng Đoạn đường
    if (u != -1) {
      double cur = trafficOf(graph, u, v);
      QString street = streetOf(graph, u, v);
      if (street.isEmpty())
        street = QString("%1 ↔ %2").arg(graph.nodes[u].name, graph.nodes[v].name);

      menu.addSection(QString("🛣️ %1 (Mật độ %2x)").arg(street).arg(cur, 0, 'f', 1));
      QAction *actSetA = menu.addAction("📍 Chọn điểm này làm xuất phát (A)");
      QAction *actSetB = menu.addAction("🏁 Chọn điểm này làm điểm đến (B)");
      menu.addSeparator();

      QMenu *subTraffic = menu.addMenu("🚦 Điều chỉnh mật độ giao thông");
      QAction *a1 = subTraffic->addAction("🟢 Đặt mật độ 1.0 (Thông thoáng)");
      QAction *a2 = subTraffic->addAction("🟡 Đặt mật độ 1.4 (Đông vừa)");
      QAction *a3 = subTraffic->addAction("🟠 Đặt mật độ 1.8 (Ùn ứ)");
      QAction *a4 = subTraffic->addAction("🔴 Đặt mật độ 2.5 (Tắc nghẽn)");
      subTraffic->addSeparator();
      QAction *aResetT = subTraffic->addAction("⚡ Khôi phục mật độ gốc ban đầu");

      menu.addSeparator();
      bool isEdgeBanned = bannedEdges.contains({u, v}) || bannedEdges.contains({v, u});
      QAction *aBan = menu.addAction(isEdgeBanned ? "✅ Bỏ cấm đoạn đường này" : "🚫 Cấm đoạn đường này");

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      QAction *chosen = menu.exec(e->globalPosition().toPoint());
#else
      QAction *chosen = menu.exec(e->globalPos());
#endif
      if (chosen == actSetA) {
        setEndpointFromEdge(u, v, proj);
      } else if (chosen == actSetB) {
        if (startPoint.isNull()) {
          setEndpointFromEdge(u, v, proj);
        } else {
          endPoint = proj;
          endEdgeU = u;
          endEdgeV = v;
          addMarker(endPoint, colorB, "B");
          recompute();
          emit endpointsChanged(startEdgeU, u);
          emit routeChanged();
        }
      } else if (chosen == a1 || chosen == a2 || chosen == a3 || chosen == a4 || chosen == aResetT) {
        double newT = 1.0;
        if (chosen == a1) newT = 1.0;
        else if (chosen == a2) newT = 1.4;
        else if (chosen == a3) newT = 1.8;
        else if (chosen == a4) newT = 2.5;

        for (auto &ed : graph.adj[u]) {
          if (ed.to == v) {
            ed.trafficFactor = (chosen == aResetT) ? ed.baseTraffic : newT;
            if (chosen != aResetT) ed.baseTraffic = newT;
          }
        }
        for (auto &ed : graph.adj[v]) {
          if (ed.to == u) {
            ed.trafficFactor = (chosen == aResetT) ? ed.baseTraffic : newT;
            if (chosen != aResetT) ed.baseTraffic = newT;
          }
        }

        updateCityTrafficOverlay();
        if (!startPoint.isNull() && !endPoint.isNull()) {
          recompute();
        } else {
          viewport()->update();
        }
        emit statusMessage(QString("Đã cập nhật mật độ %1: %2x").arg(street).arg(newT, 0, 'f', 1));
      } else if (chosen == aBan) {
        if (isEdgeBanned) {
          bannedEdges.remove({u, v});
          bannedEdges.remove({v, u});
        } else {
          bannedEdges.insert({u, v});
        }
        recompute();
      }
      return;
    }

    // TH3: Click chuột phải vào khoảng trống bản đồ
    QAction *aSwap = menu.addAction("🔄 Đảo chiều lộ trình (A ↔ B)");
    QAction *aClr = menu.addAction("🗑️ Xoá tất cả lộ trình (Esc)");
    menu.addSeparator();
    QAction *aUnban = menu.addAction("🔓 Bỏ cấm tất cả nút & đường");
    QAction *aReset = menu.addAction("🎯 Căn lại giữa bản đồ");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAction *chosen = menu.exec(e->globalPosition().toPoint());
#else
    QAction *chosen = menu.exec(e->globalPos());
#endif
    if (chosen == aClr)
      clearAll();
    else if (chosen == aSwap)
      swapAB();
    else if (chosen == aUnban) {
      bannedEdges.clear();
      bannedNodes.clear();
      recompute();
    } else if (chosen == aReset) {
      resetView();
    }
  }

  void computeRouteInfo(const Graph &g, const QVector<int> &path,
                        QVector<RouteStep> &steps, double &outMeters,
                        int &outSeconds) {
    steps.clear();
    outMeters = 0;
    outSeconds = 0;

    for (int i = 0; i < path.size() - 1; ++i) {
      int u = path[i], v = path[i + 1];
      double traf = trafficOf(g, u, v);
      double effT = traf * peakMul;
      QColor c = trafficToColor(traf);
      QPointF p1 = g.nodes[u].pos, p2 = g.nodes[v].pos;
      double distPx = QLineF(p1, p2).length();
      double h = Graph::travelTimeOf(distPx, effT, v0);

      RouteStep s;
      s.from = g.nodes[u].name;
      s.to = g.nodes[v].name;
      s.meters = distPx * 0.005 * 1000.0;
      s.seconds = static_cast<int>(h * 3600);
      s.traffic = traf;
      s.color = c;
      s.street = streetOf(g, u, v);

      bool bridge = isBridgeOf(g, u, v);
      if (bridge) {
        s.instruction = QString("Qua %1").arg(s.street.isEmpty() ? s.to : s.street);
        s.icon = "🌉";
      } else if (i == 0) {
        s.instruction = QString("Khởi hành từ %1, đi vào %2")
                            .arg(s.from, s.street.isEmpty() ? s.to : s.street);
        s.icon = "🏁";
      } else {
        QPointF p0 = g.nodes[path[i - 1]].pos;
        QPointF v1 = p1 - p0;
        QPointF v2 = p2 - p1;
        double cross = v1.x() * v2.y() - v1.y() * v2.x();
        double dot = v1.x() * v2.x() + v1.y() * v2.y();
        double l1 = std::hypot(v1.x(), v1.y());
        double l2 = std::hypot(v2.x(), v2.y());
        double angleRad = std::acos(std::clamp(dot / (l1 * l2 + 1e-6), -1.0, 1.0));
        double angleDeg = angleRad * 180.0 / 3.14159265;

        if (angleDeg < 25.0) {
          s.instruction = QString("Đi thẳng tiếp vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "⬆️";
        } else if (cross > 0) {
          s.instruction = QString("Rẽ phải vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "↱";
        } else {
          s.instruction = QString("Rẽ trái vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "↰";
        }
      }

      steps.append(s);
      outMeters += s.meters;
      outSeconds += s.seconds;
    }
  }

  void drawRoute(const Graph &g, const QVector<int> &path,
                 QVector<QGraphicsItem *> &items, bool isPrimary,
                 QColor overrideColor = QColor()) {
    if (isPrimary)
      pathAnimLines.clear();

    for (int i = 0; i < path.size() - 1; ++i) {
      int u = path[i], v = path[i + 1];
      QPointF p1 = g.nodes[u].pos, p2 = g.nodes[v].pos;

      if (isPrimary) {
        double traf = trafficOf(g, u, v);
        double effT = traf * peakMul;
        // Hiển thị màu sắc mật độ giao thông (Xanh/Vàng/Cam/Đỏ) khi bật, hoặc màu xanh Google Maps chuẩn khi tắt
        QColor c = showTrafficLayer ? trafficToColor(traf) : QColor(26, 115, 232);
        int w = 8 + (effT >= 1.8 ? 2 : 0) + (effT >= 2.2 ? 2 : 0);

        auto *shadow = scene->addLine(QLineF(p1, p2),
                                      QPen(darkMode ? QColor(30, 30, 35) : Qt::white, w + 4,
                                           Qt::SolidLine, Qt::RoundCap,
                                           Qt::RoundJoin));
        shadow->setZValue(18);
        items.append(shadow);

        auto *line = scene->addLine(QLineF(p1, p2),
                                    QPen(c, w, Qt::SolidLine,
                                         Qt::RoundCap, Qt::RoundJoin));
        line->setZValue(20);
        items.append(line);
        pathAnimLines.append(static_cast<QGraphicsLineItem *>(line));

        double len = QLineF(p1, p2).length();
        if (len > 35.0) {
          QPointF dir = (p2 - p1) / len;
          QPointF norm(-dir.y(), dir.x());
          int steps = static_cast<int>(len / 45.0);
          for (int s = 1; s <= steps; ++s) {
            double t = static_cast<double>(s) / (steps + 1);
            QPointF pt = p1 + t * (p2 - p1);

            QPainterPath arr;
            arr.moveTo(pt - dir * 4.0 + norm * 3.5);
            arr.lineTo(pt + dir * 3.0);
            arr.lineTo(pt - dir * 4.0 - norm * 3.5);

            auto *arrItem = scene->addPath(arr, QPen(QColor(255, 255, 255, 220), 2.0,
                                                     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            arrItem->setZValue(21);
            items.append(arrItem);
          }
        }
      } else {
        QPen pen(overrideColor, 6, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        auto *line = scene->addLine(QLineF(p1, p2), pen);
        line->setZValue(17);
        items.append(line);
      }
    }
  }

  void recompute() {
    if (startPoint.isNull() || endPoint.isNull())
      return;
    clearPath();
    stopAnimation();
    stopPathAnim();
    lastSteps.clear();
    totalMeters = 0;
    totalSeconds = 0;
    altPaths.clear();
    timeSteps.clear();
    distSteps.clear();
    timeTotalMeters = distTotalMeters = 0;
    timeTotalSeconds = distTotalSeconds = 0;
    dualRoutesAvailable = false;
    dualRoutesSame = false;
    primaryPoints.clear();

    QSet<QPair<int, int>> localBanned = bannedEdges;
    if (avoidCongested) {
      for (int u = 0; u < graph.nodes.size(); ++u)
        for (const Edge &e : graph.adj[u])
          if (u < e.to && e.trafficFactor * peakMul >= 2.0)
            localBanned.insert({u, e.to});
    }

    bool sameEdge = (startEdgeU == endEdgeU && startEdgeV == endEdgeV) ||
                    (startEdgeU == endEdgeV && startEdgeV == endEdgeU);
    if (sameEdge && startEdgeV != -1) {
      double traf = trafficOf(graph, startEdgeU, startEdgeV) * peakMul;
      QColor c = showTrafficLayer ? trafficToColor(traf / peakMul) : QColor(26, 115, 232);
      QPen pen(c, 10, Qt::SolidLine, Qt::RoundCap);
      auto *line = scene->addLine(QLineF(startPoint, endPoint), pen);
      line->setZValue(20);
      pathItems.append(line);

      double distPx = QLineF(startPoint, endPoint).length();
      double h = Graph::travelTimeOf(distPx, traf, v0);
      totalMeters = distPx * 0.005 * 1000.0;
      totalSeconds = static_cast<int>(h * 3600);

      RouteStep s{"A", "B", totalMeters, totalSeconds, traf / peakMul,
                  pen.color(), "Đi thẳng đến đích", "", "🏁"};
      lastSteps.append(s);
      primaryPoints = {startPoint, endPoint};
      emit routeChanged();
      emit statusMessage(QString("Đi thẳng: %1 • %2s")
                             .arg(fmtDist(totalMeters))
                             .arg(totalSeconds));
      startAnimation(primaryPoints);
      return;
    }

    Graph temp = graph;
    int startId = temp.nodes.size();
    int endId = startId + 1;
    temp.addNode(startId, "A", startPoint);
    temp.addNode(endId, "B", endPoint);

    auto attach = [&](int id, QPointF pt, int eu, int ev) {
      if (ev == -1) {
        temp.addEdge(id, eu, 0.01, 1.0);
      } else {
        double t = trafficOf(graph, eu, ev);
        temp.addEdge(id, eu, QLineF(pt, temp.nodes[eu].pos).length(), t);
        temp.addEdge(id, ev, QLineF(pt, temp.nodes[ev].pos).length(), t);
      }
    };
    attach(startId, startPoint, startEdgeU, startEdgeV);
    attach(endId, endPoint, endEdgeU, endEdgeV);

    QVector<int> timePath =
        temp.dijkstra(startId, endId, bannedNodes, localBanned, peakMul, v0);
    QVector<int> distPath =
        temp.dijkstraByDistance(startId, endId, bannedNodes, localBanned);

    if (timePath.size() < 2 && distPath.size() < 2) {
      emit statusMessage("Không tìm thấy đường đi!");
      emit routeChanged();
      return;
    }

    if (timePath.size() >= 2)
      computeRouteInfo(temp, timePath, timeSteps, timeTotalMeters,
                       timeTotalSeconds);
    if (distPath.size() >= 2)
      computeRouteInfo(temp, distPath, distSteps, distTotalMeters,
                       distTotalSeconds);

    dualRoutesSame = (timePath == distPath);
    dualRoutesAvailable = (timePath.size() >= 2 && distPath.size() >= 2);

    QVector<int> *primaryPtr = nullptr;
    QVector<int> *secondaryPtr = nullptr;

    if (routeMode == DistRoute && distPath.size() >= 2) {
      primaryPtr = &distPath;
      secondaryPtr = &timePath;
    } else {
      primaryPtr = &timePath;
      secondaryPtr = &distPath;
    }

    if (primaryPtr->size() < 2 && secondaryPtr->size() >= 2)
      std::swap(primaryPtr, secondaryPtr);

    QVector<int> &primaryPath = *primaryPtr;
    QVector<int> &secondaryPath = *secondaryPtr;

    bool drawSecondary = dualRoutesAvailable && !dualRoutesSame &&
                         secondaryPath.size() >= 2 && primaryPath.size() >= 2;

    if (drawSecondary) {
      QColor secColor =
          (routeMode == DistRoute) ? QColor(255, 140, 0) : QColor(100, 150, 200);
      drawRoute(temp, secondaryPath, secondaryPathItems, false, secColor);
    }

    for (int i = 0; i < primaryPath.size(); ++i)
      primaryPoints.append(temp.nodes[primaryPath[i]].pos);

    drawRoute(temp, primaryPath, pathItems, true);

    if (routeMode == DistRoute && distPath.size() >= 2) {
      lastSteps = distSteps;
      totalMeters = distTotalMeters;
      totalSeconds = distTotalSeconds;
    } else {
      lastSteps = timeSteps;
      totalMeters = timeTotalMeters;
      totalSeconds = timeTotalSeconds;
    }

    if (showAlternatives) {
      QVector<QPair<int, int>> mainEdges;
      for (int i = 0; i < primaryPath.size() - 1; ++i) {
        int u = primaryPath[i], v = primaryPath[i + 1];
        if (u < startId && v < startId)
          mainEdges.append({qMin(u, v), qMax(u, v)});
      }
      for (int k = 0; k < qMin(2, mainEdges.size()); ++k) {
        QSet<QPair<int, int>> extra = localBanned;
        extra.insert(mainEdges[k]);
        QVector<int> alt =
            temp.dijkstra(startId, endId, bannedNodes, extra, peakMul, v0);
        if (alt.size() >= 2) {
          QVector<QPointF> ap;
          QColor ac = (k == 0) ? QColor(0, 150, 255) : QColor(255, 140, 0);
          for (int i = 0; i < alt.size() - 1; ++i) {
            auto *line = scene->addLine(
                QLineF(temp.nodes[alt[i]].pos, temp.nodes[alt[i + 1]].pos),
                QPen(ac, 6, Qt::DashLine));
            line->setZValue(18);
            altPathItems.append(line);
            ap.append(temp.nodes[alt[i]].pos);
          }
          ap.append(temp.nodes[alt.last()].pos);
          altPaths.append(ap);
        }
      }
    }

    emit routeChanged();
    QString modeStr = (routeMode == DistRoute) ? "Quãng đường ngắn nhất"
                                               : "Thời gian nhanh nhất";
    QString msg = QString("[%1] %2 • %3 ph %4 s")
                      .arg(modeStr)
                      .arg(fmtDist(totalMeters))
                      .arg(totalSeconds / 60)
                      .arg(totalSeconds % 60);
    emit statusMessage(msg);

    updateRouteBanner();
    startAnimation(primaryPoints);
    startPathAnimation();
  }

  QString fmtDist(double meters) const {
    if (useMeters)
      return QString("%1 m").arg(meters, 0, 'f', 0);
    return QString("%1 km").arg(meters / 1000.0, 0, 'f', 2);
  }

  void startAnimation(const QVector<QPointF> &pts) {
    stopAnimation();
    if (pts.size() < 2)
      return;

    animSegments.clear();
    animTotalDist = 0.0;
    for (int i = 0; i < pts.size() - 1; ++i) {
      QPointF p0 = pts[i], p1 = pts[i + 1];
      double len = QLineF(p0, p1).length();
      if (len < 1e-4) continue;
      double angle = std::atan2(p1.y() - p0.y(), p1.x() - p0.x()) * 180.0 / 3.14159265;
      animTotalDist += len;
      animSegments.append({p0, p1, len, animTotalDist, angle});
    }
    if (animSegments.isEmpty()) return;

    animCurrentDist = 0.0;
    animSpeed = (transportMode == Car) ? 190.0 : ((transportMode == Motorbike) ? 150.0 : 70.0);

    mover = scene->createItemGroup({});

    // Hào quang tỏa sáng xung quanh xe
    auto *outerHalo = scene->addEllipse(-18, -18, 36, 36,
                                        QPen(Qt::NoPen),
                                        QBrush(QColor(66, 133, 244, 55)));
    mover->addToGroup(outerHalo);

    // Vòng tròn nền
    auto *whiteRing = scene->addEllipse(-12, -12, 24, 24,
                                        QPen(Qt::white, 2.0),
                                        QBrush(QColor(26, 115, 232)));
    mover->addToGroup(whiteRing);

    if (transportMode == Car) {
      // Tia đèn pha chiếu sáng phía trước
      QPolygonF headlight;
      headlight << QPointF(-4, -8) << QPointF(-14, -28) << QPointF(14, -28) << QPointF(4, -8);
      auto *hlItem = scene->addPolygon(headlight, QPen(Qt::NoPen), QBrush(QColor(255, 238, 88, 80)));
      mover->addToGroup(hlItem);

      // Thân xe ô tô sedan
      QPainterPath carPath;
      carPath.addRoundedRect(QRectF(-6, -9, 12, 18), 3, 3);
      auto *carBody = scene->addPath(carPath, QPen(Qt::white, 1), QBrush(QColor(245, 245, 255)));
      mover->addToGroup(carBody);

      // Kính chắn gió
      auto *windshield = scene->addRect(-4, -4, 8, 4, QPen(Qt::NoPen), QBrush(QColor(40, 50, 70)));
      mover->addToGroup(windshield);

      // Đèn hậu đỏ
      auto *tl1 = scene->addRect(-5, 7, 3, 2, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      auto *tl2 = scene->addRect(2, 7, 3, 2, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      mover->addToGroup(tl1);
      mover->addToGroup(tl2);
    } else if (transportMode == Motorbike) {
      // Đèn xe máy
      QPolygonF headlight;
      headlight << QPointF(0, -6) << QPointF(-8, -22) << QPointF(8, -22);
      auto *hlItem = scene->addPolygon(headlight, QPen(Qt::NoPen), QBrush(QColor(255, 238, 88, 90)));
      mover->addToGroup(hlItem);

      // Thân xe máy + người lái
      auto *bikeBody = scene->addRect(-2, -7, 4, 14, QPen(Qt::white, 1), QBrush(QColor(251, 188, 4)));
      mover->addToGroup(bikeBody);

      auto *helmet = scene->addEllipse(-3.5, -2, 7, 7, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      mover->addToGroup(helmet);
    } else {
      // Đi bộ (Walker)
      QPolygonF arrow;
      arrow << QPointF(0, -7) << QPointF(-4, 3) << QPointF(0, 1) << QPointF(4, 3);
      auto *arrowItem = scene->addPolygon(arrow, QPen(Qt::NoPen), QBrush(Qt::white));
      mover->addToGroup(arrowItem);
    }

    mover->setZValue(42);
    mover->setPos(pts[0]);
    mover->setRotation(animSegments[0].angleDeg + 90.0);

    smoothAnimTimer->start(35);
  }

  void stopAnimation() {
    if (smoothAnimTimer)
      smoothAnimTimer->stop();
    if (mover) {
      if (mover->scene())
        scene->removeItem(mover);
      delete mover;
      mover = nullptr;
    }
    animSegments.clear();
    animCurrentDist = 0.0;
  }

  void startPathAnimation() {
    pathAnimIndex = 0;
    for (auto *l : pathAnimLines)
      if (l && l->scene())
        l->setVisible(false);
    pathAnimTimer->start(70);
  }

  void onPathAnimTick() {
    if (pathAnimIndex >= pathAnimLines.size()) {
      pathAnimTimer->stop();
      return;
    }
    auto *line = pathAnimLines[pathAnimIndex++];
    if (line && line->scene())
      line->setVisible(true);
  }

  void stopPathAnim() {
    if (pathAnimTimer)
      pathAnimTimer->stop();
    pathAnimIndex = 0;
  }

  void onTrafficSimTick() {
    if (isTypicalMode) return;

    if (!trafficSimEnabled) {
      applyLiveTraffic();
      return;
    }

    simMinute += 15;
    if (simMinute >= 60) {
      simMinute = 0;
      simHour++;
      if (simHour > 22)
        simHour = 6;
    }

    applyTypicalTraffic(selectedDayIndex, simHour, simMinute);
  }
};

// ==================== GIAO DIỆN CHÍNH MAIN WINDOW ====================
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow() {
    setWindowTitle("Bản đồ Đà Nẵng — Dẫn đường thông minh Google Maps");
    resize(1460, 900);

    setStyleSheet(R"(
      QMainWindow { background: #F8F9FA; }
      QToolBar {
        background: #FFFFFF;
        border-bottom: 1px solid #DADCE0;
        spacing: 6px;
        padding: 4px 8px;
      }
      QToolBar QToolButton {
        color: #3C4043;
        font-family: 'Segoe UI';
        font-size: 12px;
        font-weight: 500;
        padding: 5px 10px;
        border-radius: 4px;
        border: none;
        background: transparent;
      }
      QToolBar QToolButton:hover  { background: #F1F3F4; }
      QToolBar QToolButton:pressed { background: #E8EAED; }
      QToolBar QLabel {
        color: #5F6368;
        font-family: 'Segoe UI';
        font-size: 12px;
        font-weight: 500;
      }
      QToolBar QCheckBox {
        color: #3C4043;
        font-family: 'Segoe UI';
        font-size: 12px;
        spacing: 6px;
      }
      QToolBar QCheckBox::indicator {
        width: 15px; height: 15px;
        border: 2px solid #9AA0A6;
        border-radius: 3px;
        background: white;
      }
      QToolBar QCheckBox::indicator:checked {
        background: #4285F4;
        border-color: #4285F4;
      }
      QDockWidget {
        color: #202124;
        font-family: 'Segoe UI';
        font-size: 13px;
      }
      QDockWidget::title {
        background: #FFFFFF;
        padding: 10px 14px;
        border-bottom: 1px solid #DADCE0;
        font-weight: bold;
        font-size: 14px;
        color: #202124;
      }
      QListWidget {
        background: #FFFFFF;
        border: none;
        font-family: 'Segoe UI';
        font-size: 12px;
        color: #3C4043;
        outline: none;
      }
      QListWidget::item {
        padding: 8px 10px;
        border-bottom: 1px solid #F1F3F4;
        min-height: 36px;
      }
      QListWidget::item:hover   { background: #F8F9FA; }
      QListWidget::item:selected { background: #E8F0FE; color: #1A73E8; }
      QComboBox {
        border: 1px solid #DADCE0;
        border-radius: 6px;
        padding: 5px 12px;
        background: #FFFFFF;
        font-family: 'Segoe UI';
        font-size: 12px;
        color: #3C4043;
        min-height: 24px;
      }
      QComboBox:hover    { border-color: #4285F4; }
      QComboBox:focus    { border: 2px solid #4285F4; }
      QComboBox::drop-down { border: none; width: 22px; }
      QStatusBar {
        background: #FFFFFF;
        color: #5F6368;
        font-family: 'Segoe UI';
        font-size: 11px;
        border-top: 1px solid #DADCE0;
      }
      QSlider::groove:horizontal {
        height: 4px;
        background: #DADCE0;
        border-radius: 2px;
      }
      QSlider::handle:horizontal {
        background: #4285F4;
        width: 14px; height: 14px;
        margin: -5px 0;
        border-radius: 7px;
        border: none;
      }
      QSlider::sub-page:horizontal {
        background: #4285F4;
        border-radius: 2px;
      }
    )");

    view = new MapView(this);
    setCentralWidget(view);

    buildTopToolbar();
    buildDirectionsDock();
    buildBottomToolbar();

    connect(view, &MapView::routeChanged, this, &MainWindow::onRouteUpdated);
    connect(view, &MapView::statusMessage, this,
            [this](const QString &m) { statusBar()->showMessage(m, 4000); });
    connect(view, &MapView::mousePosChanged, this, [this](QPointF p) {
      coordLabel->setText(
          QString("(%1, %2)").arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0));
    });
    connect(view, &MapView::endpointsChanged, this, &MainWindow::onEndpointsUpdated);

    statusBar()->showMessage(
        QString("Sẵn sàng — Bản đồ đầy đủ %1 giao lộ Đà Nẵng | Click chọn điểm A → B").arg(view->nodeCount()));
    onRouteUpdated();
    view->setEndpoints(5, 55);
  }

private slots:
  void onEndpointsUpdated(int startId, int endId) {
    disconnect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    disconnect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);

    startCombo->setCurrentIndex(startId >= 0 ? startId + 1 : 0);
    destCombo->setCurrentIndex(endId >= 0 ? endId + 1 : 0);

    connect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    connect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
  }

  void onComboChanged(int) {
    int sIdx = startCombo->currentIndex() - 1;
    int dIdx = destCombo->currentIndex() - 1;
    if (sIdx >= 0 && dIdx >= 0 && sIdx != dIdx) {
      view->setEndpoints(sIdx, dIdx);
    } else if (sIdx >= 0 && dIdx < 0) {
      view->setEndpoints(sIdx, sIdx);
    }
  }

  void onRouteUpdated() {
    if (view->lastRoute().isEmpty() && !view->hasDualRoutes()) {
      etaLabel->setText("-- phút");
      etaLabel->setStyleSheet("color: #70757A; font-size: 26px; font-weight: bold;");
      distEtaLabel->setText("Chưa có tuyến đường được chọn");
      trafficChip->setText("● Chọn điểm đi & điểm đến");
      trafficChip->setStyleSheet("background: #F1F3F4; color: #5F6368; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
    } else {
      int totalSecs = view->lastTotalSeconds();
      int mins = (totalSecs + 30) / 60;
      if (mins < 1) mins = 1;
      etaLabel->setText(QString("%1 phút").arg(mins));

      QTime etaTime = QTime::currentTime().addSecs(totalSecs);
      distEtaLabel->setText(QString("%1 • Dự kiến đến %2")
                                .arg(unitStr(view->lastTotalMeters()))
                                .arg(etaTime.toString("HH:mm")));

      bool congested = false;
      for (const auto &s : view->lastRoute()) {
        if (s.traffic >= 1.8) {
          congested = true;
          break;
        }
      }
      if (congested) {
        etaLabel->setStyleSheet("color: #D93025; font-size: 28px; font-weight: bold;");
        trafficChip->setText("● Có đoạn ùn ứ");
        trafficChip->setStyleSheet("background: #FCE8E6; color: #C5221F; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
      } else {
        etaLabel->setStyleSheet("color: #188038; font-size: 28px; font-weight: bold;");
        trafficChip->setText("● Tuyến nhanh nhất • Đường thông thoáng");
        trafficChip->setStyleSheet("background: #E6F4EA; color: #137333; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
      }
    }

    if (view->hasDualRoutes() && !view->areDualRoutesSame()) {
      dualRouteWidget->setVisible(true);
      int tSec = view->timeRouteTotalSeconds();
      int dSec = view->distRouteTotalSeconds();
      timeRouteBtn->setText(QString("⏱ Nhanh nhất: %1 ph (%2)")
                               .arg((tSec + 30) / 60)
                               .arg(unitStr(view->timeRouteTotalMeters())));
      distRouteBtn->setText(QString("📏 Ngắn nhất: %1 ph (%2)")
                               .arg((dSec + 30) / 60)
                               .arg(unitStr(view->distRouteTotalMeters())));

      if (view->currentRouteMode() == MapView::TimeRoute) {
        timeRouteBtn->setStyleSheet("QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 6px; font-weight: bold; padding: 6px; }");
        distRouteBtn->setStyleSheet("QPushButton { background: #FFFFFF; color: #5F6368; border: 1px solid #DADCE0; border-radius: 6px; padding: 6px; }");
      } else {
        distRouteBtn->setStyleSheet("QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 6px; font-weight: bold; padding: 6px; }");
        timeRouteBtn->setStyleSheet("QPushButton { background: #FFFFFF; color: #5F6368; border: 1px solid #DADCE0; border-radius: 6px; padding: 6px; }");
      }
    } else {
      dualRouteWidget->setVisible(false);
    }

    stepList->clear();
    const auto &steps = view->lastRoute();
    if (steps.isEmpty()) {
      auto *emptyItem = new QListWidgetItem("Hãy nhấp chọn điểm A và B trên bản đồ");
      emptyItem->setTextAlignment(Qt::AlignCenter);
      emptyItem->setForeground(QColor(128, 134, 139));
      stepList->addItem(emptyItem);
    } else {
      for (int i = 0; i < steps.size(); ++i) {
        const auto &s = steps[i];
        QString txt = QString("%1  %2\n     %3 • %4 giây")
                          .arg(s.icon, s.instruction)
                          .arg(unitStr(s.meters))
                          .arg(s.seconds);
        auto *it = new QListWidgetItem(txt);
        QPixmap pm(10, 10);
        pm.fill(s.color);
        it->setIcon(QIcon(pm));
        stepList->addItem(it);
      }

      auto *finish = new QListWidgetItem(QString("🏁  Đến điểm hẹn: %1")
                                             .arg(steps.last().to));
      QFont ff = finish->font();
      ff.setBold(true);
      finish->setFont(ff);
      finish->setForeground(QColor(234, 67, 53));
      stepList->addItem(finish);
    }

    statsLabel->setText(QString("Giao lộ: %1 | Tuyến đường: %2")
                            .arg(view->nodeCount())
                            .arg(view->edgeCount()));
  }

  void copyRoute() {
    const auto &steps = view->lastRoute();
    if (steps.isEmpty())
      return;
    QString t = "===== LỘ TRÌNH GOOGLE MAPS ĐÀ NẴNG =====\n";
    for (int i = 0; i < steps.size(); ++i) {
      t += QString("%1. %2 (%3, %4s, mật độ %5)\n")
               .arg(i + 1)
               .arg(steps[i].instruction)
               .arg(unitStr(steps[i].meters))
               .arg(steps[i].seconds)
               .arg(steps[i].traffic, 0, 'f', 1);
    }
    t += QString("TỔNG CỘNG: %1 | %2 phút %3 giây\n")
             .arg(unitStr(view->lastTotalMeters()))
             .arg(view->lastTotalSeconds() / 60)
             .arg(view->lastTotalSeconds() % 60);
    QApplication::clipboard()->setText(t);
    statusBar()->showMessage("Đã sao chép lộ trình chỉ đường", 2500);
  }

  void savePng() {
    QString fn = QFileDialog::getSaveFileName(this, "Lưu ảnh bản đồ",
                                              "GoogleMaps_DaNang_Full.png", "PNG (*.png)");
    if (fn.isEmpty())
      return;
    if (!fn.endsWith(".png", Qt::CaseInsensitive))
      fn += ".png";
    view->savePng(fn);
  }

  void showNodeTable() {
    QDialog dlg(this);
    dlg.setWindowTitle("Danh mục các giao lộ & địa điểm Đà Nẵng");
    dlg.resize(560, 480);
    dlg.setStyleSheet("background: #FFFFFF; font-family: 'Segoe UI';");

    auto *tw = new QTableWidget(&dlg);
    tw->setColumnCount(4);
    tw->setHorizontalHeaderLabels({"ID", "Địa điểm / Tuyến đường", "Tọa độ", "Số nhánh"});
    tw->setRowCount(view->nodeCount());
    auto names = view->nodeNames();
    for (int i = 0; i < names.size(); ++i) {
      tw->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
      tw->setItem(i, 1, new QTableWidgetItem(names[i]));
      QPointF p = view->nodePos(i);
      tw->setItem(
          i, 2,
          new QTableWidgetItem(
              QString("(%1, %2)").arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0)));
      tw->setItem(i, 3,
                  new QTableWidgetItem(QString::number(view->nodeDegree(i))));
    }
    tw->horizontalHeader()->setStretchLastSection(true);
    tw->verticalHeader()->setVisible(false);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addWidget(tw);
    dlg.exec();
  }

private:
  MapView *view = nullptr;
  QListWidget *stepList = nullptr;
  QLabel *coordLabel = nullptr;
  QLabel *statsLabel = nullptr;

  QComboBox *startCombo = nullptr;
  QComboBox *destCombo = nullptr;
  QLabel *etaLabel = nullptr;
  QLabel *distEtaLabel = nullptr;
  QLabel *trafficChip = nullptr;
  QLabel *simTimeLabel = nullptr;
  QWidget *dualRouteWidget = nullptr;
  QPushButton *timeRouteBtn = nullptr;
  QPushButton *distRouteBtn = nullptr;

  QWidget *dockContentWidget = nullptr;
  QFrame *planCardWidget = nullptr;
  QFrame *etaCardWidget = nullptr;
  QLabel *stepsTitleLabel = nullptr;

  bool useMetersFlag = true;

  QString unitStr(double m) const {
    if (useMetersFlag)
      return QString("%1 m").arg(m, 0, 'f', 0);
    return QString("%1 km").arg(m / 1000.0, 0, 'f', 2);
  }

  // ── Dark / Light theme sync ──────────────────────────────────────
  void applyAppTheme(bool dark) {
    // ── Main window / toolbar / dock stylesheet ──
    if (dark) {
      setStyleSheet(R"(
        QMainWindow { background: #1A1A2E; }
        QToolBar {
          background: #16213E;
          border-bottom: 1px solid #0F3460;
          spacing: 6px;
          padding: 4px 8px;
        }
        QToolBar QToolButton {
          color: #E0E0E0;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 5px 10px;
          border-radius: 4px;
          border: none;
          background: transparent;
        }
        QToolBar QToolButton:hover   { background: #0F3460; }
        QToolBar QToolButton:pressed { background: #533483; }
        QToolBar QLabel {
          color: #A8B2C1;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
        }
        QToolBar QCheckBox {
          color: #C8D0DC;
          font-family: 'Segoe UI';
          font-size: 12px;
          spacing: 6px;
        }
        QToolBar QCheckBox::indicator {
          width: 15px; height: 15px;
          border: 2px solid #555E6D;
          border-radius: 3px;
          background: #2A2D3E;
        }
        QToolBar QCheckBox::indicator:checked {
          background: #4285F4;
          border-color: #4285F4;
        }
        QDockWidget {
          color: #E0E0E0;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QDockWidget::title {
          background: #16213E;
          padding: 10px 14px;
          border-bottom: 1px solid #0F3460;
          font-weight: bold;
          font-size: 14px;
          color: #FFFFFF;
        }
        QListWidget {
          background: #1E2235;
          border: none;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #C8D0DC;
          outline: none;
        }
        QListWidget::item {
          padding: 8px 10px;
          border-bottom: 1px solid #2A2D3E;
          min-height: 36px;
        }
        QListWidget::item:hover    { background: #252840; }
        QListWidget::item:selected { background: #1A3A5C; color: #7AB3FF; }
        QComboBox {
          border: 1px solid #3A4560;
          border-radius: 6px;
          padding: 5px 12px;
          background: #1E2235;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #C8D0DC;
          min-height: 24px;
        }
        QComboBox:hover  { border-color: #4285F4; }
        QComboBox:focus  { border: 2px solid #4285F4; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
          background: #1E2235;
          color: #C8D0DC;
          selection-background-color: #1A3A5C;
        }
        QStatusBar {
          background: #16213E;
          color: #8892A4;
          font-family: 'Segoe UI';
          font-size: 11px;
          border-top: 1px solid #0F3460;
        }
        QSlider::groove:horizontal {
          height: 4px;
          background: #3A4560;
          border-radius: 2px;
        }
        QSlider::handle:horizontal {
          background: #4285F4;
          width: 14px; height: 14px;
          margin: -5px 0;
          border-radius: 7px;
          border: none;
        }
        QSlider::sub-page:horizontal {
          background: #4285F4;
          border-radius: 2px;
        }
      )");
    } else {
      setStyleSheet(R"(
        QMainWindow { background: #F8F9FA; }
        QToolBar {
          background: #FFFFFF;
          border-bottom: 1px solid #DADCE0;
          spacing: 6px;
          padding: 4px 8px;
        }
        QToolBar QToolButton {
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 5px 10px;
          border-radius: 4px;
          border: none;
          background: transparent;
        }
        QToolBar QToolButton:hover  { background: #F1F3F4; }
        QToolBar QToolButton:pressed { background: #E8EAED; }
        QToolBar QLabel {
          color: #5F6368;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
        }
        QToolBar QCheckBox {
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 12px;
          spacing: 6px;
        }
        QToolBar QCheckBox::indicator {
          width: 15px; height: 15px;
          border: 2px solid #9AA0A6;
          border-radius: 3px;
          background: white;
        }
        QToolBar QCheckBox::indicator:checked {
          background: #4285F4;
          border-color: #4285F4;
        }
        QDockWidget {
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QDockWidget::title {
          background: #FFFFFF;
          padding: 10px 14px;
          border-bottom: 1px solid #DADCE0;
          font-weight: bold;
          font-size: 14px;
          color: #202124;
        }
        QListWidget {
          background: #FFFFFF;
          border: none;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #3C4043;
          outline: none;
        }
        QListWidget::item {
          padding: 8px 10px;
          border-bottom: 1px solid #F1F3F4;
          min-height: 36px;
        }
        QListWidget::item:hover   { background: #F8F9FA; }
        QListWidget::item:selected { background: #E8F0FE; color: #1A73E8; }
        QComboBox {
          border: 1px solid #DADCE0;
          border-radius: 6px;
          padding: 5px 12px;
          background: #FFFFFF;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #3C4043;
          min-height: 24px;
        }
        QComboBox:hover    { border-color: #4285F4; }
        QComboBox:focus    { border: 2px solid #4285F4; }
        QComboBox::drop-down { border: none; width: 22px; }
        QStatusBar {
          background: #FFFFFF;
          color: #5F6368;
          font-family: 'Segoe UI';
          font-size: 11px;
          border-top: 1px solid #DADCE0;
        }
        QSlider::groove:horizontal {
          height: 4px;
          background: #DADCE0;
          border-radius: 2px;
        }
        QSlider::handle:horizontal {
          background: #4285F4;
          width: 14px; height: 14px;
          margin: -5px 0;
          border-radius: 7px;
          border: none;
        }
        QSlider::sub-page:horizontal {
          background: #4285F4;
          border-radius: 2px;
        }
      )");
    }

    // ── Per-widget fine-grained dark style overrides ──
    if (dockContentWidget) {
      dockContentWidget->setStyleSheet(
          dark ? "background: #1A1A2E;" : "background: #FFFFFF;");
    }
    if (planCardWidget) {
      planCardWidget->setStyleSheet(
          dark ? "QFrame { background:#1E2235; border:1px solid #3A4560; border-radius:10px; padding:8px; }"
               : "QFrame { background:#FFFFFF; border:1px solid #DADCE0; border-radius:10px; padding:8px; }");
    }
    if (etaCardWidget) {
      etaCardWidget->setStyleSheet(
          dark ? "QFrame { background:#1E2235; border:1px solid #3A4560; border-radius:10px; padding:10px; }"
               : "QFrame { background:#FFFFFF; border:1px solid #DADCE0; border-radius:10px; padding:10px; }");
    }
    if (stepsTitleLabel) {
      stepsTitleLabel->setStyleSheet(
          dark ? "font-weight:bold; color:#A8B2C1; font-size:13px;"
               : "font-weight:bold; color:#202124; font-size:13px;");
    }
    if (distEtaLabel) {
      distEtaLabel->setStyleSheet(
          dark ? "color:#8892A4; font-size:13px;"
               : "color:#5F6368; font-size:13px;");
    }
    // Refresh the route display so ETA label colour resets properly
    onRouteUpdated();
  }

  void buildTopToolbar() {
    QToolBar *tb = addToolBar("Chính");
    tb->setMovable(false);

    QAction *actClear = tb->addAction("✕ Đặt lại", view, &MapView::clearAll);
    actClear->setToolTip("Xoá điểm chọn hiện tại (Phím Esc)");

    QAction *actSwap = tb->addAction("⇅ Đảo A↔B", view, &MapView::swapAB);
    actSwap->setToolTip("Đảo vị trí điểm đi và điểm đến");

    QAction *actTraffic = tb->addAction("🚦 Mật độ GT", this, [this]() {
      view->setTrafficLayerVisible(!view->isTrafficLayerVisible());
    });
    actTraffic->setCheckable(true);
    actTraffic->setChecked(view->isTrafficLayerVisible());
    actTraffic->setToolTip("Bật/Tắt màu sắc thể hiện mật độ giao thông trên tuyến đường A → B");
    connect(view, &MapView::trafficLayerVisibilityChanged, actTraffic, [actTraffic](bool v) {
      actTraffic->setChecked(v);
    });

    tb->addSeparator();

    tb->addAction("⌖ Về giữa", view, &MapView::resetView);
    tb->addAction("⛶ Toàn cảnh", view, &MapView::fitView);

    tb->addSeparator();

    tb->addAction("▶ Mô phỏng GPS", view, &MapView::playNavigationSimulation);
    tb->addAction("📋 Copy lộ trình", this, &MainWindow::copyRoute);
    tb->addAction("📷 Lưu ảnh", this, &MainWindow::savePng);
    tb->addAction(QString("📍 Danh mục địa điểm (%1)").arg(view->nodeCount()), this, &MainWindow::showNodeTable);

    tb->addSeparator();

    QCheckBox *simBox = new QCheckBox("Mô phỏng GT động");
    tb->addWidget(simBox);

    simTimeLabel = new QLabel("🟢 08:00 – Cao điểm sáng");
    simTimeLabel->setStyleSheet(
        "color: #1A73E8; font-weight: bold; padding: 4px 10px; font-size: 13px; "
        "background: #E8F0FE; border-radius: 6px; border: 1.5px solid #1A73E8; margin-left: 4px;");
    tb->addWidget(simTimeLabel);

    connect(view, &MapView::trafficSimTimeChanged, this, [this](int, const QString &desc) {
      if (simTimeLabel) {
        simTimeLabel->setText(QString("🟢 %1").arg(desc));
      }
    });

    connect(simBox, &QCheckBox::toggled, this, [this](bool b) {
      view->setTrafficSimulation(b);
      if (!b && simTimeLabel) {
        simTimeLabel->setText("⚪ Tạm dừng mô phỏng");
      }
    });

    simBox->setChecked(false);
  }

  void buildDirectionsDock() {
    QDockWidget *dock = new QDockWidget("Chỉ đường Google Maps", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    QWidget *w = new QWidget;
    w->setStyleSheet("background: #FFFFFF;");
    dockContentWidget = w;
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(12);

    // Form chọn điểm đi & đến
    QFrame *planCard = new QFrame;
    planCardWidget = planCard;
    planCard->setStyleSheet(R"(
      QFrame {
        background: #FFFFFF;
        border: 1px solid #DADCE0;
        border-radius: 10px;
        padding: 8px;
      }
    )");
    QVBoxLayout *cardLay = new QVBoxLayout(planCard);
    cardLay->setContentsMargins(6, 6, 6, 6);
    cardLay->setSpacing(8);

    QHBoxLayout *rA = new QHBoxLayout;
    QLabel *dotA = new QLabel("🟢");
    dotA->setFixedWidth(22);
    startCombo = new QComboBox;
    startCombo->addItem("— Chọn điểm đi (A) —");
    startCombo->addItems(view->nodeNames());
    connect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    rA->addWidget(dotA);
    rA->addWidget(startCombo, 1);
    cardLay->addLayout(rA);

    QHBoxLayout *rB = new QHBoxLayout;
    QLabel *dotB = new QLabel("🔴");
    dotB->setFixedWidth(22);
    destCombo = new QComboBox;
    destCombo->addItem("— Chọn điểm đến (B) —");
    destCombo->addItems(view->nodeNames());
    connect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    rB->addWidget(dotB);
    rB->addWidget(destCombo, 1);
    cardLay->addLayout(rB);

    // Chọn phương tiện
    QHBoxLayout *modeLay = new QHBoxLayout;
    modeLay->setSpacing(6);
    QPushButton *btnCar = new QPushButton("🚗 Ô tô");
    QPushButton *btnBike = new QPushButton("🛵 Xe máy");
    QPushButton *btnWalk = new QPushButton("🚶 Đi bộ");

    QString modeActiveStyle = "QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 14px; font-weight: bold; padding: 5px 10px; font-size: 11px; }";
    QString modeNormalStyle = "QPushButton { background: #F1F3F4; color: #5F6368; border: 1px solid transparent; border-radius: 14px; padding: 5px 10px; font-size: 11px; } QPushButton:hover { background: #E8EAED; }";

    btnCar->setStyleSheet(modeActiveStyle);
    btnBike->setStyleSheet(modeNormalStyle);
    btnWalk->setStyleSheet(modeNormalStyle);

    connect(btnCar, &QPushButton::clicked, this, [=]() {
      btnCar->setStyleSheet(modeActiveStyle);
      btnBike->setStyleSheet(modeNormalStyle);
      btnWalk->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Car);
    });
    connect(btnBike, &QPushButton::clicked, this, [=]() {
      btnBike->setStyleSheet(modeActiveStyle);
      btnCar->setStyleSheet(modeNormalStyle);
      btnWalk->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Motorbike);
    });
    connect(btnWalk, &QPushButton::clicked, this, [=]() {
      btnWalk->setStyleSheet(modeActiveStyle);
      btnCar->setStyleSheet(modeNormalStyle);
      btnBike->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Walking);
    });

    modeLay->addWidget(btnCar);
    modeLay->addWidget(btnBike);
    modeLay->addWidget(btnWalk);
    cardLay->addLayout(modeLay);

    lay->addWidget(planCard);

    // Thẻ tổng quan ETA
    QFrame *etaCard = new QFrame;
    etaCardWidget = etaCard;
    etaCard->setStyleSheet(R"(
      QFrame {
        background: #FFFFFF;
        border: 1px solid #DADCE0;
        border-radius: 10px;
        padding: 10px;
      }
    )");
    QVBoxLayout *eLay = new QVBoxLayout(etaCard);
    eLay->setContentsMargins(8, 8, 8, 8);
    eLay->setSpacing(4);

    etaLabel = new QLabel("-- phút");
    etaLabel->setStyleSheet("color: #188038; font-size: 28px; font-weight: bold;");
    eLay->addWidget(etaLabel);

    distEtaLabel = new QLabel("Chưa có tuyến đường");
    distEtaLabel->setStyleSheet("color: #5F6368; font-size: 13px;");
    eLay->addWidget(distEtaLabel);

    trafficChip = new QLabel("● Tuyến nhanh nhất");
    trafficChip->setStyleSheet("background: #E6F4EA; color: #137333; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
    eLay->addWidget(trafficChip);

    dualRouteWidget = new QWidget;
    QHBoxLayout *drLay = new QHBoxLayout(dualRouteWidget);
    drLay->setContentsMargins(0, 6, 0, 0);
    drLay->setSpacing(6);
    timeRouteBtn = new QPushButton("⏱ Nhanh nhất");
    distRouteBtn = new QPushButton("📏 Ngắn nhất");
    connect(timeRouteBtn, &QPushButton::clicked, this, [this]() {
      view->setRouteMode(MapView::TimeRoute);
    });
    connect(distRouteBtn, &QPushButton::clicked, this, [this]() {
      view->setRouteMode(MapView::DistRoute);
    });
    drLay->addWidget(timeRouteBtn);
    drLay->addWidget(distRouteBtn);
    dualRouteWidget->setVisible(false);
    eLay->addWidget(dualRouteWidget);

    lay->addWidget(etaCard);

    QLabel *stepsTitle = new QLabel("Chi tiết các chặng di chuyển:");
    stepsTitleLabel = stepsTitle;
    stepsTitle->setStyleSheet("font-weight: bold; color: #202124; font-size: 13px;");
    lay->addWidget(stepsTitle);

    stepList = new QListWidget;
    stepList->setWordWrap(true);
    lay->addWidget(stepList, 1);

    dock->setWidget(w);
    dock->setMinimumWidth(360);
    addDockWidget(Qt::RightDockWidgetArea, dock);
  }

  void buildBottomToolbar() {
    QToolBar *tb = addToolBar("Tùy chọn");
    tb->setMovable(false);

    QCheckBox *unitBox = new QCheckBox("Dùng mét");
    unitBox->setChecked(true);
    connect(unitBox, &QCheckBox::toggled, this, [this](bool b) {
      useMetersFlag = b;
      view->setUseMeters(b);
      onRouteUpdated();
    });
    tb->addWidget(unitBox);

    tb->addSeparator();

    QCheckBox *darkBox = new QCheckBox("Chế độ đêm (Dark)");
    connect(darkBox, &QCheckBox::toggled, this, [this](bool b) {
      view->setDarkMode(b);
      applyAppTheme(b);
    });
    tb->addWidget(darkBox);

    QCheckBox *trafficBox = new QCheckBox("Mật độ GT (A → B)");
    trafficBox->setChecked(view->isTrafficLayerVisible());
    trafficBox->setToolTip("Bật/Tắt màu sắc mật độ giao thông trên tuyến đường A → B");
    connect(trafficBox, &QCheckBox::toggled, view, &MapView::setTrafficLayerVisible);
    connect(view, &MapView::trafficLayerVisibilityChanged, trafficBox, [trafficBox](bool v) {
      trafficBox->setChecked(v);
    });
    tb->addWidget(trafficBox);

    QCheckBox *gridBox = new QCheckBox("Lưới tọa độ");
    connect(gridBox, &QCheckBox::toggled, view, &MapView::setGridVisible);
    tb->addWidget(gridBox);

    tb->addSeparator();

    QCheckBox *banBox = new QCheckBox("Tránh Cầu Rồng");
    connect(banBox, &QCheckBox::toggled, view, &MapView::setBanLongBridge);
    tb->addWidget(banBox);

    QCheckBox *conBox = new QCheckBox("Tránh điểm kẹt xe");
    connect(conBox, &QCheckBox::toggled, view, &MapView::setAvoidCongested);
    tb->addWidget(conBox);

    QCheckBox *altBox = new QCheckBox("So sánh lộ trình");
    connect(altBox, &QCheckBox::toggled, view, &MapView::setShowAlternatives);
    tb->addWidget(altBox);

    tb->addSeparator();

    coordLabel = new QLabel("(—, —)");
    coordLabel->setMinimumWidth(85);
    tb->addWidget(new QLabel(" Tọa độ: "));
    tb->addWidget(coordLabel);

    statsLabel = new QLabel("Giao lộ: 0 | Tuyến đường: 0");
    statsLabel->setMinimumWidth(150);
    tb->addWidget(statsLabel);
  }

protected:
  void showEvent(QShowEvent *e) override {
    QMainWindow::showEvent(e);
    QTimer::singleShot(60, this, [this]() {
      view->resetView();
    });
  }
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow w;
  w.show();

  for (int i = 1; i < argc; ++i) {
    if (QString(argv[i]) == "--screenshot" && i + 1 < argc) {
      QString outFn = argv[i + 1];
      QTimer::singleShot(800, [&w, outFn]() {
        QPixmap pix = w.grab();
        pix.save(outFn);
        QCoreApplication::quit();
      });
      break;
    }
  }

  return app.exec();
}
