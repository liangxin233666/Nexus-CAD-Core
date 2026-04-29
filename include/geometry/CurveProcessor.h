#pragma once
#include "BezierSpline.h"
#include <algorithm>

// 用于渲染线段的基础结构
struct RenderLine { Vec2 start, end; };

// 带有曲率信息的曲线离散点
struct CurvaturePoint { Vec2 pos; double kappa; };

class CurveProcessor {
public:
    // 将曲线离散化，并附带计算每个采样点的曲率
    static std::vector<CurvaturePoint> discretizeCurveWithCurvature(const BezierSpline& spline, int resolution = 80) {
        std::vector<CurvaturePoint> pts;
        double tMax = spline.getSegmentCount();
        pts.reserve(static_cast<size_t>(tMax * resolution) + 2);

        double step = 1.0 / resolution;
        for (double t = 0; t <= tMax; t += step) {
            pts.push_back({ spline.getPoint(t), spline.getCurvature(t) });
        }
        return pts;
    }

    // 生成曲率梳：在线段上绘制沿法线方向延伸的线段，长度与当前曲率成正比
    static std::vector<RenderLine> generateCurvatureComb(const BezierSpline& spline, double scale, int res = 80) {
        std::vector<RenderLine> combs;
        double tMax = spline.getSegmentCount();
        combs.reserve(static_cast<size_t>(tMax * res) + 2);

        double step = 1.0 / res;
        for (double t = 0; t <= tMax; t += step) {
            Vec2 p = spline.getPoint(t);
            Vec2 normal = spline.getDerivative(t).perpendicular().normalize();
            combs.push_back({ p, p + normal * (spline.getCurvature(t) * scale) });
        }
        return combs;
    }

    // 生成偏移曲线：将曲线沿法线方向平移指定距离
    static std::vector<Vec2> generateOffsetCurve(const BezierSpline& spline, double distance, int res = 80) {
        std::vector<Vec2> pts;
        double tMax = spline.getSegmentCount();
        pts.reserve(static_cast<size_t>(tMax * res) + 2);

        double step = 1.0 / res;
        for (double t = 0; t <= tMax; t += step) {
            Vec2 normal = spline.getDerivative(t).perpendicular().normalize();
            pts.push_back(spline.getPoint(t) + normal * distance);
        }
        return pts;
    }

    // 使用 Andrew 算法（单调链算法）计算控制点集合的凸包
    static std::vector<Vec2> computeConvexHull(const std::vector<Vec2>& inPts) {
        if (inPts.size() <= 3) return inPts;
        std::vector<Vec2> pts = inPts;

        // 按 X 坐标排序，若相同则按 Y 排序
        std::sort(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
            });

        std::vector<Vec2> hull;
        hull.reserve(pts.size());

        // 构建下半部分
        for (const auto& p : pts) {
            while (hull.size() >= 2 && crossProduct(hull[hull.size() - 2], hull.back(), p) <= 0)
                hull.pop_back();
            hull.push_back(p);
        }

        // 构建上半部分
        size_t t = hull.size() + 1;
        for (int i = static_cast<int>(pts.size()) - 2; i >= 0; i--) {
            while (hull.size() >= t && crossProduct(hull[hull.size() - 2], hull.back(), pts[i]) <= 0)
                hull.pop_back();
            hull.push_back(pts[i]);
        }
        hull.pop_back();
        return hull;
    }

    // 查找曲率正负号发生变化的拐点（使用二分查找逼近）
    static std::vector<Vec2> findInflectionPoints(const BezierSpline& spline, int resPerSeg = 30) {
        std::vector<Vec2> inflections;
        for (int s = 0; s < spline.getSegmentCount(); s++) {
            // 使用一阶导和二阶导的叉乘表示相对曲率的符号
            double prevCross = spline.getDerivative(s).cross(spline.getSecondDerivative(s));

            for (int i = 1; i <= resPerSeg; i++) {
                double t = s + static_cast<double>(i) / resPerSeg;
                double currCross = spline.getDerivative(t).cross(spline.getSecondDerivative(t));

                // 如果发现变号，说明存在拐点
                if (prevCross * currCross < 0) {
                    double t0 = t - 1.0 / resPerSeg, t1 = t;
                    // 在此区间内进行二分迭代寻找精确解
                    for (int k = 0; k < 10; k++) {
                        double tm = (t0 + t1) * 0.5;
                        if (prevCross * spline.getDerivative(tm).cross(spline.getSecondDerivative(tm)) < 0)
                            t1 = tm; else t0 = tm;
                    }
                    inflections.push_back(spline.getPoint((t0 + t1) * 0.5));
                }
                prevCross = currCross;
            }
        }
        return inflections;
    }

    // 计算给定坐标点到曲线上的最短距离点，返回对应的参数 T
    static double getClosestPointT(const BezierSpline& spline, Vec2 mousePos) {
        double bestT = 0, minDistSq = 1e9;
        int samples = spline.getSegmentCount() * 50;
        double stepBase = spline.getSegmentCount() / static_cast<double>(samples);

        // 步骤1：全局粗略采样，找到近似最近点
        for (int i = 0; i <= samples; i++) {
            double t = i * stepBase;
            double dSq = spline.getPoint(t).distanceTo(mousePos);
            if (dSq < minDistSq) { minDistSq = dSq; bestT = t; }
        }

        // 步骤2：局部二分搜索微调，提升精度
        double step = 0.5 / spline.getSegmentCount();
        for (int k = 0; k < 12; k++) {
            double tL = std::max(0.0, bestT - step);
            double tR = std::min(static_cast<double>(spline.getSegmentCount()), bestT + step);
            double dL = spline.getPoint(tL).distanceTo(mousePos);
            double dR = spline.getPoint(tR).distanceTo(mousePos);

            if (dL < dR && dL < minDistSq) { bestT = tL; minDistSq = dL; }
            else if (dR < dL && dR < minDistSq) { bestT = tR; minDistSq = dR; }
            step *= 0.5;
        }
        return bestT;
    }

    // 生成速矢图 (Hodograph)：将曲线的导数向量作为一个独立的图形映射出来
    static std::vector<Vec2> generateHodograph(const BezierSpline& spline, Vec2 origin, double scale, int res = 80) {
        std::vector<Vec2> hodoPts;
        double tMax = spline.getSegmentCount();
        hodoPts.reserve(static_cast<size_t>(tMax * res) + 2);

        double step = 1.0 / res;
        for (double t = 0; t <= tMax; t += step) {
            hodoPts.push_back(origin + spline.getDerivative(t) * scale);
        }
        return hodoPts;
    }
};