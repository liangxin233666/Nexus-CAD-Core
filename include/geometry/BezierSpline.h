#pragma once
#include <vector>
#include "math/GeoMath.h"

// 节点连续性类型定义
enum class ContinuityMode {
    Free,     // C0: 两侧手柄独立
    Aligned,  // C1: 两侧手柄共线但长度可不同
    Mirrored  // C2: 两侧手柄共线且长度相等
};

class BezierSpline {
public:
    std::vector<Vec2> points;               // 控制点数组，每 3 个点定义一个分段
    std::vector<ContinuityMode> modes;      // 锚点的连续性状态

    // 初始化一段基础曲线
    void init(Vec2 center) {
        points.clear();
        modes.clear();
        points.reserve(32);
        modes.reserve(10);

        // 初始生成包含 4 个点的单个贝塞尔线段 (1个锚点, 2个手柄, 1个锚点)
        points.push_back(center - Vec2(200, 0));
        points.push_back(center - Vec2(100, -150));
        points.push_back(center + Vec2(100, 150));
        points.push_back(center + Vec2(200, 0));
        modes.push_back(ContinuityMode::Mirrored);
        modes.push_back(ContinuityMode::Mirrored);
    }

    // 在曲线末尾追加一个新的线段
    void addSegment(Vec2 anchorPos) {
        const Vec2& lastAnchor = points[points.size() - 1];
        const Vec2& lastHandle = points[points.size() - 2];

        // 根据前一个手柄的位置推算新的起始手柄，保持连续性
        Vec2 newHandle1 = lastAnchor + (lastAnchor - lastHandle);
        Vec2 newHandle2 = anchorPos + (newHandle1 - anchorPos) * 0.5;

        points.push_back(newHandle1);
        points.push_back(newHandle2);
        points.push_back(anchorPos);
        modes.push_back(ContinuityMode::Mirrored);

        enforceContinuity(points.size() - 4, points.size() - 5, points.size() - 3);
    }

    // 获取曲线的分段数量
    [[nodiscard]] inline int getSegmentCount() const {
        return static_cast<int>((points.size() - 1) / 3);
    }

    // 计算全局参数 t 对应的曲线点坐标 (t 范围 0 到 getSegmentCount)
    [[nodiscard]] Vec2 getPoint(double t) const {
        int seg; double localT;
        getSegAndT(t, seg, localT);
        int idx = seg * 3;
        double u = 1.0 - localT;

        // 标准的三次贝塞尔多项式展开
        return points[idx] * (u * u * u) +
            points[idx + 1] * (3 * u * u * localT) +
            points[idx + 2] * (3 * u * localT * localT) +
            points[idx + 3] * (localT * localT * localT);
    }

    // 计算参数 t 处的一阶导数（切线向量）
    [[nodiscard]] Vec2 getDerivative(double t) const {
        int seg; double localT;
        getSegAndT(t, seg, localT);
        int idx = seg * 3;
        double u = 1.0 - localT;
        return (points[idx + 1] - points[idx]) * (3 * u * u) +
            (points[idx + 2] - points[idx + 1]) * (6 * u * localT) +
            (points[idx + 3] - points[idx + 2]) * (3 * localT * localT);
    }

    // 计算参数 t 处的二阶导数（加速度向量）
    [[nodiscard]] Vec2 getSecondDerivative(double t) const {
        int seg; double localT;
        getSegAndT(t, seg, localT);
        int idx = seg * 3;
        double u = 1.0 - localT;
        return (points[idx + 2] - points[idx + 1] * 2.0 + points[idx]) * (6 * u) +
            (points[idx + 3] - points[idx + 2] * 2.0 + points[idx + 1]) * (6 * localT);
    }

    // 计算参数 t 处的曲率 (公式: (x'y'' - y'x'') / (x'^2 + y'^2)^(3/2))
    [[nodiscard]] double getCurvature(double t) const {
        Vec2 d1 = getDerivative(t), d2 = getSecondDerivative(t);
        double lenSq = d1.lengthSquared();
        if (lenSq < 1e-8) return 0.0;
        return d1.cross(d2) / (lenSq * std::sqrt(lenSq));
    }

    // 通过离散采样近似计算曲线总长度
    [[nodiscard]] double calculateLength(int res = 50) const {
        double len = 0.0;
        int segs = getSegmentCount();
        double step = 1.0 / res;
        for (double t = 0; t < segs; t += step) {
            len += getPoint(t).distanceTo(getPoint(t + step));
        }
        return len;
    }

    // 使用格林公式通过离散线段近似计算曲线闭合区域面积
    [[nodiscard]] double calculateArea(int res = 100) const {
        double area = 0.0;
        int segs = getSegmentCount();
        double step = 1.0 / res;
        for (double t = 0; t < segs; t += step) {
            Vec2 p1 = getPoint(t), p2 = getPoint(t + step);
            area += (p1.x * p2.y - p2.x * p1.y);
        }
        return std::abs(area * 0.5);
    }

    // 拉普拉斯平滑：将内部控制点向相邻点的均值位置靠拢，以平滑整条曲线
    void fairCurve(int iterations = 3) {
        std::vector<Vec2> temp = points;
        for (int iter = 0; iter < iterations; ++iter) {
            for (size_t i = 1; i < points.size() - 1; ++i) {
                // 锚点和手柄分配不同的移动权重
                double weight = (i % 3 == 0) ? 0.1 : 0.4;
                temp[i] = points[i] * (1.0 - weight) + (points[i - 1] + points[i + 1]) * (weight * 0.5);
            }
            points = temp;
        }
        // 平滑结束后，重新强制对齐手柄以维持连续性设置
        for (size_t i = 0; i < points.size(); i += 3) {
            if (i >= 1 && i + 1 < points.size()) enforceContinuity(i, i - 1, i + 1);
        }
    }

    // 在指定线段的参数 t 处将其拆分为两段 (De Casteljau 算法)
    void subdivideSegment(int segIndex, double t = 0.5) {
        if (segIndex < 0 || segIndex >= getSegmentCount()) return;
        int idx = segIndex * 3;

        // 计算插值点
        Vec2 p01 = points[idx] + (points[idx + 1] - points[idx]) * t;
        Vec2 p12 = points[idx + 1] + (points[idx + 2] - points[idx + 1]) * t;
        Vec2 p23 = points[idx + 2] + (points[idx + 3] - points[idx + 2]) * t;

        Vec2 p012 = p01 + (p12 - p01) * t;
        Vec2 p123 = p12 + (p23 - p12) * t;

        Vec2 p0123 = p012 + (p123 - p012) * t;

        // 插入新的控制点
        points.insert(points.begin() + idx + 3, p0123);
        points.insert(points.begin() + idx + 4, p123);
        points.insert(points.begin() + idx + 5, p23);

        points[idx + 1] = p01;
        points[idx + 2] = p012;

        // 新增锚点默认设为 Aligned
        modes.insert(modes.begin() + segIndex + 1, ContinuityMode::Aligned);
    }

    // 移动控制点，并处理联动的拓扑约束
    void setControlPoint(int index, Vec2 pos) {
        Vec2 delta = pos - points[index];
        points[index] = pos;

        if (index % 3 == 0) { // 如果移动的是锚点，跟随移动两个手柄
            if (index - 1 >= 0) points[index - 1] += delta;
            if (index + 1 < points.size()) points[index + 1] += delta;
        }
        else { // 如果移动的是手柄，根据连续性规则调整对侧手柄
            int anchorIdx = ((index + 1) % 3 == 0) ? index + 1 : index - 1;
            int siblingIdx = ((index + 1) % 3 == 0) ? index + 2 : index - 2;
            if (siblingIdx >= 0 && siblingIdx < points.size()) {
                enforceContinuity(anchorIdx, index, siblingIdx);
            }
        }
    }

    // 切换锚点的连续性模式 (Free -> Aligned -> Mirrored -> Free)
    void toggleContinuity(int anchorIndex) {
        int modeIdx = anchorIndex / 3;
        modes[modeIdx] = (modes[modeIdx] == ContinuityMode::Free) ? ContinuityMode::Aligned :
            (modes[modeIdx] == ContinuityMode::Aligned) ? ContinuityMode::Mirrored : ContinuityMode::Free;

        if (anchorIndex - 1 >= 0 && anchorIndex + 1 < points.size()) {
            enforceContinuity(anchorIndex, anchorIndex - 1, anchorIndex + 1);
        }
    }

private:
    // 将全局参数 t 拆分为所在的线段索引和该线段内的局部参数 localT
    void getSegAndT(double t, int& seg, double& localT) const {
        int maxSeg = getSegmentCount();
        if (t >= maxSeg) { seg = maxSeg - 1; localT = 1.0; return; }
        if (t < 0) { seg = 0; localT = 0.0; return; }
        seg = static_cast<int>(t);
        localT = t - seg;
    }

    // 强制执行节点两侧的连续性约束
    void enforceContinuity(int anchorIdx, int movedIdx, int siblingIdx) {
        if (modes[anchorIdx / 3] == ContinuityMode::Free) return;

        Vec2 anchor = points[anchorIdx];
        Vec2 movedDir = points[movedIdx] - anchor;
        double siblingDist = (points[siblingIdx] - anchor).length();

        if (modes[anchorIdx / 3] == ContinuityMode::Aligned && movedDir.length() > 1e-6) {
            // C1: 保持方向共线，长度维持原状
            points[siblingIdx] = anchor - movedDir.normalize() * siblingDist;
        }
        else if (modes[anchorIdx / 3] == ContinuityMode::Mirrored) {
            // C2: 方向共线，且长度强制对等
            points[siblingIdx] = anchor - movedDir;
        }
    }
};