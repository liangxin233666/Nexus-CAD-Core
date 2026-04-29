#include "editor/CurveEditor.h"

// 初始化，生成默认样条曲线
void CurveEditor::init() {
    spline.init(Vec2(600, 450));
}

// 鼠标按下事件
void CurveEditor::onMouseDown(double x, double y, bool isRightClick) {
    Vec2 mousePos(x, y);
    int hit = -1;
    double minDistance = hitRadius;

    // 遍历所有控制点，找出距离鼠标最近且在判定半径内的点
    for (size_t i = 0; i < spline.points.size(); ++i) {
        double dist = spline.points[i].distanceTo(mousePos);
        if (dist < minDistance) {
            minDistance = dist;
            hit = static_cast<int>(i);
        }
    }

    if (hit != -1) {
        // 如果点中已有控制点，更新选中和拖拽状态
        selectedPointIndex = hit;
        draggingPointIndex = hit;
    }
    else if (!isRightClick) {
        // 左键点击空白处，添加新线段
        spline.addSegment(mousePos);
        selectedPointIndex = static_cast<int>(spline.points.size() - 1);
        draggingPointIndex = selectedPointIndex;
    }
    else {
        // 右键点击空白处，取消选中
        selectedPointIndex = -1;
    }
}

// 鼠标拖拽事件：更新被拖拽控制点的位置
void CurveEditor::onMouseDrag(double x, double y) {
    if (draggingPointIndex != -1) {
        spline.setControlPoint(draggingPointIndex, Vec2(x, y));
    }
}

// 鼠标松开事件：重置拖拽状态
void CurveEditor::onMouseUp() {
    draggingPointIndex = -1;
}

// 触发选中控制点的连续性切换
void CurveEditor::toggleContinuityForSelection() {
    if (selectedPointIndex != -1) {
        // 如果选中了手柄点，需要先找到其归属的锚点索引
        int anchorIdx = (selectedPointIndex % 3 == 0) ? selectedPointIndex :
            ((selectedPointIndex + 1) % 3 == 0 ? selectedPointIndex + 1 : selectedPointIndex - 1);
        spline.toggleContinuity(anchorIdx);
    }
}

// 对当前选中的控制点所属的线段执行细分操作
void CurveEditor::subdivideAtSelected() {
    if (selectedPointIndex != -1) {
        int segIdx = selectedPointIndex / 3;
        // 处理选中末尾点的情况
        if (segIdx >= spline.getSegmentCount()) {
            segIdx = spline.getSegmentCount() - 1;
        }
        spline.subdivideSegment(segIdx, 0.5);
        selectedPointIndex = -1; // 细分后重置选中状态
    }
}