#pragma once
#include "geometry/BezierSpline.h"

// 负责管理交互状态与渲染参数的类
class CurveEditor {
public:
    BezierSpline spline;          // 核心样条数据

    // 交互状态
    int selectedPointIndex = -1;  // 当前选中的控制点索引
    int draggingPointIndex = -1;  // 正在拖拽的控制点索引
    double hitRadius = 15.0;      // 鼠标点击的有效判定半径

    // UI 显示模式状态
    bool isLightMode = false;

    // 分析功能开关
    bool showCurvature = false;         // 显示曲率梳
    bool showOffset = false;            // 显示等距偏移
    bool showConvexHull = false;        // 显示凸包
    bool showInflections = true;        // 显示拐点
    bool enableSnapping = true;         // 启用对齐投影
    bool showOsculatingCircle = true;   // 显示密切圆与法向标记
    bool showHodograph = false;         // 显示速矢图
    bool showHeatmapCurve = true;       // 使用曲率颜色渲染曲线

    // 用户调节参数
    float offsetDistance = 30.0f;       // 偏移距离
    float curvatureScale = 50000.0f;    // 曲率梳显示缩放比例

    // 事件处理接口
    void init();
    void onMouseDown(double x, double y, bool isRightClick = false);
    void onMouseDrag(double x, double y);
    void onMouseUp();

    void toggleContinuityForSelection(); // 更改选中点的连续性
    void subdivideAtSelected();          // 对选中点所在的线段进行细分
};